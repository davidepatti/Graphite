#define MAX_THREAD 65

int pertubations;
int perturbed_bytes;

void approxikit_init()
{
    pertubations = 0;
    perturbed_bytes = 0;
}

void approxikit_status()
{
    FILE * fp;
    printf("writing approxikit status \n");
    fp = fopen ("/sim/graphite/results/latest/APPROXIKIT_perturbations.txt", "a+");
    fprintf(fp,"APPROXIKIT total pertubation call = %d\n",pertubations);
    fprintf(fp,"APPROXIKIT total bytes involved = %d\n",perturbed_bytes);
    fclose(fp);
}

char BitFlip(char v, int b)
{
    printf("\nAPPROXIKIT: AZZACCAGNAI!");
      int bit_b = v & (1 << b);
        if (bit_b == 0)
	        return v | (1 << b);
	  else
	          return v & ~(1 << b);
}
// ---------------------------------------------------------------------------

void Perturb(void *addr, unsigned int size, double ber)
{
    //printf("APPROXIKIT: Pertubating address %x, size %d, ber %lf\n",addr,size,ber);
    pertubations++;
    char *data = (char*)addr;

    for (int i=0; i<size; i++)
    {
	if ((double)rand() / RAND_MAX <= ber*8)
	    *(data+i) = BitFlip(*(data+i), rand() % 8); // flip a random bit in a byte
    }

    perturbed_bytes+=size;
}
// ---------------------------------------------------------------------------
// Note: using a single common file it's ok since annotation is done
// before thread spawning
void annotate_address_home(FILE* fp, char * label,long unsigned int address, int size)
{
    int granularity = 1;
    int n_controllers = 2;
    int id = (address>>granularity)%n_controllers;
    //printf("[DATA_LABEL: %s],  ahl %d, n_controllers %d, ADDRESS %lx, TILE ID %d (+%d bytes)\n", label,granularity,n_controllers,A,id,length);
    fprintf(fp,"LABEL:%s g:%d nc:%d addr:%lx size:%d\n",label,granularity,n_controllers,address,size);
    //printf("Annotating region [LABEL:%s](%lx-%lx,%d)\n",label,address,address+size,size);
}
