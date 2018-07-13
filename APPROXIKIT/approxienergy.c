#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_REGIONS 200000
#define MAX_THREADS 65
#define NOT_VALID 0

#define EPBWL 1.47e-12
#define EPBWL_APPROX 9.98e-13
#define EPBWR 0.527e-12
#define EPBWR_APPROX 0.152e-12

#define DIST_MAX 5


typedef struct {
    long unsigned address;
    int size;
    char label[16];
} Region;

int communications[MAX_THREADS][MAX_THREADS];

Region regions[MAX_REGIONS];

void id2Coord(int id,int * x, int * y)
{
    *x = id%8;
    *y = id/8;
}

int dist(int a,int b)
{
    int ax,bx,ay,by;
    id2Coord(a,&ax,&ay);
    id2Coord(b,&bx,&by);
    return abs(ax-bx)+abs(ay-by);
}


int main(int argc, char *argv[])
{
    int bufSize = 256;
    char label[100];
    int granularity,n_controllers,size,src,dst; 
    long unsigned address;

    int current_region = 0;
    int total_regions = 0;

    int i,j;

    for (i=0;i<MAX_THREADS;i++)
        for(j=0;j<MAX_THREADS;j++)
            communications[i][j]=0;

    for (i=0;i<MAX_REGIONS;i++)
    {
        regions[i].address = NOT_VALID;
        regions[i].size = NOT_VALID;
    }

    FILE* fp;
    char buf[bufSize];
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <annotated regions> <memory refs>\n", argv[0]);
        return 1;
    }
    if ((fp = fopen(argv[1], "r")) == NULL)
    {
        perror("fopen source-file");
        return 1;
    }

    while (fgets(buf, sizeof(buf), fp) != NULL)
    {
        buf[strlen(buf) - 1] = '\0'; // eat the newline fgets() stores
        sscanf(buf,"LABEL:%s g:%d nc:%d addr:%lx size:%d\n",label,&granularity,&n_controllers,&address,&size);

        printf("Loading region [%s](%lx-%lx,%d)\n",label,address,address+size,size);

        strcpy(regions[current_region].label,label);
        regions[current_region].address = address;
        regions[current_region].size = size;
        current_region++;
    }

    fclose(fp);
    total_regions = current_region;
    printf("Total regions loaded %d\n",total_regions);

    if ((fp = fopen(argv[2], "r")) == NULL)
    {
        perror("Cannot open annotated file");
        return 1;
    }

    int c = 0;
    double energy = 0.0;
    double energy_approx = 0.0;

    int dst_x,dst_y;
    int src_x,src_y;


    int hops_wireless;
    int hops_wired;

    int total_wired_hops = 0;

    // no winoc
    int total_noc_hops = 0;

    int only_wired_comms = 0;
    int wireless_comms = 0;


    while (fgets(buf, sizeof(buf), fp) != NULL)
    {
	bool approxiref;
	
	c++;
        buf[strlen(buf) - 1] = '\0'; // eat the newline fgets() stores
        sscanf(buf,"DRAM addr:%lx size:%d src:%d dst:%d\n",&address,&size,&src,&dst);

	if (c%10000==0)
	    printf("%d mem ref entries processed...\n",c);

        for (i = 0; i <total_regions;i++)
        {
            int region_start = regions[i].address;
            int region_end = regions[i].address+regions[i].size-1;
            int address_end = address + size;
            int address_start = address;

	    approxiref = false;

            if ( address_start <= region_end && address_end >= region_start)
            {
		approxiref = true;
            }
        }

	id2Coord(src,&src_x,&src_y);
	id2Coord(dst,&dst_x,&dst_y);

	int dis = dist(src,dst);

	total_noc_hops+=dis;

	if (dis >DIST_MAX)
	{
	    if (src_x>3)
	    {
		if (src_y>3)
		    dst = 63;
		else
		    dst = 7;
	    }
	    else
	    {
		if (src_y>3)
		    dst = 56;
		else
		    dst = 0;
	    }

	    wireless_comms++;
	    hops_wireless = 1;
	    id2Coord(dst,&dst_x,&dst_y);
	}
	else
	{
	    only_wired_comms++;
	    hops_wireless = 0;
	}


	hops_wired = abs(src_x-dst_x) + abs(src_y-dst_y);
	total_wired_hops+=hops_wired;

	energy_approx+=hops_wired*((approxiref)?EPBWR_APPROX:EPBWR)*64*8+
	        hops_wireless*((approxiref)?EPBWL_APPROX:EPBWL)*64*8;

	energy+=hops_wired*(EPBWR)*64*8+
	        hops_wireless*(EPBWL)*64*8;
    }
    fclose(fp);

    int total_bytes = 0;
    double savings = (energy-energy_approx)/energy;
    /*
    printf("----------------------------------------\n");
    printf(" energy: %lf \n",energy);
    printf(" energy_approx: %lf \n",energy_approx);
    printf(" savings: %lf \n",savings);
    printf("----------------------------------------\n");
    */
   
    printf("WiNoCwired only communications: %d\n",only_wired_comms);
    printf("WiNoC wireless communications: %d\n",wireless_comms);
    printf("WiNoC fraction wireless communications: %lf\n",(double)wireless_comms/(wireless_comms+only_wired_comms));
    printf("WiNoC avg wired hops: %lf\n",(double)total_wired_hops/(only_wired_comms+wireless_comms));
    printf("NoC avg hops: %lf\n",(double)total_noc_hops/(only_wired_comms+wireless_comms));

}
