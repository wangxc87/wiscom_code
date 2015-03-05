#include <stdio.h>

int main(void)
{
    unsigned int a = 0;
    int c,b=0;
    for(a=0;a<0xfffff;a++){
        c = a%2;
        if(c !=b){
            printf("%d:",b);
            b = c;
            printf("%d   ",b);
        }
        if(a%100 == 0)
            printf ("\n");
        usleep(1000);
    }
    return 1;

}
