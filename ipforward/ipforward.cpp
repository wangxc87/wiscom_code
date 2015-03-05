
extern "C"
{
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <signal.h>
}

#include <string>

using namespace std;

int done = 1;
unsigned long recvFrameCount = 0;

#define trace()     printf("%d\n", __LINE__)

int usage()
{
    printf("ipforward --proto udp/tcp --recv-port port --send-ip ip --send-port sendport\n");
    return 0;
}

int ipforware_udp(unsigned short recvport, const char *sendip, unsigned short sendport)
{
    int s = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    int i = 0;
    if ( -1 == s )
        return -1;

    int opt = 1;
    if ( -1 == setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (void*)&opt, sizeof(opt)) )
    {
        close(s);
        return -1;
    }

    struct sockaddr_in  addr;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(recvport);
    if ( -1 == bind(s, (struct sockaddr *) &addr, (socklen_t)sizeof(addr)) )
    {
        close(s);
        return -1;
    }

    char data[2048];
    socklen_t socklen = sizeof(addr);
    int size;

    while ( true )
    {
        size = recvfrom(s, data, sizeof(data), 0, (struct sockaddr *)&addr, (socklen_t*)&socklen);
        if ( size <= 0 )
            continue;
        //        printf("recv frameID %u..\n",i++);
        recvFrameCount ++;
        memset(&addr, 0, sizeof(addr));
        addr.sin_addr.s_addr = inet_addr(sendip);
        addr.sin_port   = htons(sendport);
        addr.sin_family = AF_INET;

        size = sendto(s, data, size, 0, (struct sockaddr *)&addr, (socklen_t)sizeof(addr));
    }

    close(s);

    return -1;
}
void signal_handle(int iSignNo)
{
    switch(iSignNo){
    case SIGINT:
        //        done = 1;
        fprintf(stdout,"SIGINT (Ctrl + c) capture..\n");
        break;
    case SIGQUIT:
        fprintf(stdout,"SIGQUIT (Ctrl + \\)Capture....\n");
        fprintf(stdout,"Recieve Total FrameNum is %ld..\n",recvFrameCount);
        break;
    default:
        break;
    }
}

int main(int argc, char**argv)
{
    string proto;
    string recvport;
    string sendip;
    string sendport;

    if ( argc != 9 )
        return usage();

    signal(SIGQUIT,signal_handle);//ctrl+\ quit

    for (int i = 1; i < argc; i+=2)
    {
        string opt = argv[i];

        if ( !opt.compare("--proto") )
        {
            proto = string(argv[i+1]);
        }

        if ( !opt.compare("--recv-port") )
        {
            recvport = string(argv[i+1]);
        }

        if ( !opt.compare("--send-ip") )
        {
            sendip = string(argv[i+1]);
        }

        if ( !opt.compare("--send-port") )
        {
            sendport = string(argv[i+1]);
        }
    }

    if ( proto.empty() || recvport.empty()
        || sendip.empty() || sendport.empty() )
    {
        return usage();
    }

    if ( !proto.compare("udp") )
    {
        return ipforware_udp((unsigned short)atoi(recvport.c_str()), sendip.c_str(), (unsigned short)atoi(sendport.c_str()));
    }
    else
    {
        printf("not support\n");
    }

    return 0;
}
