#include <stdio.h>
#define MAX_REGIONS 10000
#define MAX_THREADS 65
#define NOT_VALID 0

typedef struct {
    long unsigned address;
    int size;
    char label[16];
} Region;

int communications[MAX_THREADS][MAX_THREADS];

Region regions[MAX_REGIONS];

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
        //printf("Reading-> %s\n", buf);
        sscanf(buf,"LABEL:%s g:%d nc:%d addr:%lx size:%d\n",label,&granularity,&n_controllers,&address,&size);

        printf("Loading region [%s](%lx-%lx,%d)\n",label,address,address+size,size);

        strcpy(regions[current_region].label,label);
        regions[current_region].address = address;
        regions[current_region].size = size;
        current_region++;
    }
    total_regions = current_region;
    fclose(fp);

    if ((fp = fopen(argv[2], "r")) == NULL)
    {
        perror("Cannot open annotated file");
        return 1;
    }

    while (fgets(buf, sizeof(buf), fp) != NULL)
    {
        buf[strlen(buf) - 1] = '\0'; // eat the newline fgets() stores
        //printf("Reading-> %s\n", buf);
        sscanf(buf,"DRAM addr:%lx size:%d src:%d dst:%d\n",&address,&size,&src,&dst);
        //printf("Reading access addr:%lx size:%d src:%d dst:%d\n",address,size,src,dst);

        for (i = 0; i <total_regions;i++)
        {
            int region_start = regions[i].address;
            int region_end = regions[i].address+regions[i].size;
            int address_end = address + size;
            int address_start = address;

            if ( address_start <= region_end && address_end >= region_start)
            {
                communications[src][dst]++;
            }
        }

    }
    fclose(fp);

    int total_bytes = 0;
    printf("----------------------------------------\n");
    printf(" approximate communication list \n");
    printf("----------------------------------------\n");
   
    for (i=0;i<MAX_THREADS;i++)
        for(j=0;j<MAX_THREADS;j++)
        {
            // assuming send size of cacheline 64 bytes
            if (communications[i][j]!=0)
            {
                printf("src: %d dst: %d occurences: %d total bytes: %d\n",i,j,communications[i][j],communications[i][j]*64);
                total_bytes+=communications[i][j]*64;
            }

        }

    printf("** Total bytes in this communication list: %d\n",total_bytes);
}
