#include "probe.h"


#define THRESHOLD 10000
/* stupid codes to be monitored */ 
void computation_mult()
{
   double tmp=1.0;
   int i=1;
   for( i = 1; i < THRESHOLD; i++ )
   {
      tmp = tmp*i;
   }
}

/* stupid codes to be monitored */ 
void computation_add()
{
   int tmp = 0;
   int i=0;

   for( i = 0; i < THRESHOLD; i++ )
   {
      tmp = tmp + i;
   }

}


int main() {
    Probe* probe = new Probe();
    probe->init();
    probe->start();

    /* Your code goes here*/
    computation_add();

    probe->stop();
    probe->print();
    exit(0); 
}
