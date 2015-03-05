#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main()
{
    int fd;
    int res;
    fd_set rdfds;
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    char ch;
    char buf[100];
	
    while(1) {
       FD_ZERO(&rdfds);
        FD_SET(0, &rdfds); 
        res = select(1, &rdfds, NULL, NULL, &tv);    
        printf("res = %d \n", res);
        if (res > 0 && FD_ISSET(0, &rdfds)){
            ch = getchar();
            printf("ch = %c\n", ch);
            if (ch == 'q')
                return 0;
            read(0, buf, sizeof(buf[100]));
        }
 //       printf("hello world!\n");
        sleep(1);
    }
    return 0;
}
