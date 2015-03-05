#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rs232.h"
main(int argc, char *argv[])
{

	int ret,portno,nWritten,nRead;
	char buf[256];
    char dev_name[256];
    snprintf(dev_name, 256, "/dev/%s", argv[1]);
    printf("Test Port %s.\n",dev_name);
    
	portno=0;
	while(1)
	{
		ret=OpenCom(portno,dev_name,115200);
		if(ret==-1)
		{
                printf("The %s open error.",dev_name);
			exit(1);
		}
		nWritten=ComWrt(portno,"abcdsdf",7);
		printf("\n%s has send %d chars!\n",dev_name,nWritten);
		printf("\nRecieving data!***\n");
		fflush(stdout);
/*
		nRead=ComRd(0,buf,256,3000);
		if(nRead>0)
		{
			printf("*****OK\n");
		}
		else
			printf("Timeout\n");
*/
		if((ret=CloseCom(portno)==-1))
		{
			perror("Close com");
			exit(1);
		}
		printf("\n\n");
        usleep(10000);
        
	}
	printf("Exit now.\n");
	return;
}
