
char BitFlip(char v, int b)
{
      int bit_b = v & (1 << b);
        if (bit_b == 0)
	        return v | (1 << b);
	  else
	          return v & ~(1 << b);
}
// ---------------------------------------------------------------------------

void Perturb(void *addr, unsigned int size, double ber)
{
    char *data = (char*)addr;

    for (int i=0; i<size; i++)
    {
	if ((double)rand() / RAND_MAX <= ber*8)
	    *(data+i) = BitFlip(*(data+i), rand() % 8); // flip a random bit in a byte
    }
}
// ---------------------------------------------------------------------------
void annotate_address_home(char * label,long unsigned int A, int length)
{
    int granularity = 1;
    int n_controllers = 2;
    int id = (A>>granularity)%n_controllers;
    //printf("[DATA_LABEL: %s],  ahl %d, n_controllers %d, ADDRESS %lx, TILE ID %d (+%d bytes)\n", label,granularity,n_controllers,A,id,length);
    printf("%s,%d,%d,%lx,%d\n", label,granularity,n_controllers,A,length);
}
