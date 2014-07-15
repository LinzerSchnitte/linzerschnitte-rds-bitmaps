//Flint -LinzerSchnitte Entangled Sparks Transmitter Sender
//Josh Gardiner
//2014 06 05
//Copyright (C) Josh Gardiner 2014
/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include <unistd.h>
#include <poll.h>
#include <sys/timerfd.h>
#include <sched.h>

#include "util.h"
#include "esp.h"
#include "esp_time.h"
#include "udp.h"

#define BUFFERSIZE 32
#define VESRION 1.10
#define PACKET_INTERVAL 180000000 //190ms

void print_flint_help(void);
void print_greating(void);

int main(int argc, char**argv)
{
	fprintf(stderr, "Flint:Entangled Sparks: Frame Contoller for Entagngled Sparks Version %1.2f\n", VESRION);
	fprintf(stderr, "Copyright (C)2014 Josh Gardiner josh@zool.com\n");
	fprintf(stderr, "\nThis program comes with ABSOLUTELY NO WARRANTY; for details type -w'\n");
	fprintf(stderr, "This is free software, and you are welcome to redistribute it\n");
	fprintf(stderr, "under certain conditions; type -c for details.\n");
	fprintf(stderr, "\ntype -h for help\n\n");

    // Create Packet types
    espDataPacket sendpacket; // We Send espData Packets
    espAckPacket recvpacket;  // We Recive espAck Packets

    // Setup Options
    int cmd_option;
    int port = 1535;
    char *server = "127.0.0.1";
    // Parse cmd line options
    while((cmd_option=getopt(argc, argv, "hp:s:wc")) != EOF)
    switch(cmd_option)
    {
        default:
        case 'h': print_flint_help();
        case 'p': port=atoi(optarg); break;
        case 's': server=(optarg); break;
        case 'w': print_warranty();
        case 'c': print_conditions();
    }


    struct sockaddr_in myaddr;
    struct sockaddr_in remaddr;

    int socket1;
    int slen=sizeof(remaddr);

    int recvlen;

    if ((socket1=socket(AF_INET, SOCK_DGRAM, 0))==-1)
        printf("socket created\n");

    memset((char *)&myaddr, 0, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr.sin_port = htons(0);

    int optval=1;
    setsockopt(socket1,SOL_SOCKET,SO_BROADCAST,&optval,sizeof optval); // enables bcast

    // Not Required if using polling
    //int nonBlocking = 1;
    //if ( fcntl( socket1, F_SETFL, O_NONBLOCK, nonBlocking ) == -1 )
    //{
    //  printf( "failed to set non-blocking\n" );
    //  return -1;
    //}


    if (bind(socket1, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
        perror("bind failed");
        return 0;
    }  
    
    memset((char *) &remaddr, 0, sizeof(remaddr));
    remaddr.sin_family = AF_INET;
    remaddr.sin_port = htons(port);
    if (inet_aton(server, &remaddr.sin_addr)==0) {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }


    struct sched_param schedparm;
    memset(&schedparm, 0, sizeof(schedparm));
    schedparm.sched_priority = 1; // lowest rt priority
    sched_setscheduler(0, SCHED_FIFO, &schedparm);

    struct itimerspec timspec;
    bzero(&timspec, sizeof(timspec));
    timspec.it_interval.tv_sec = 0;
    timspec.it_interval.tv_nsec = PACKET_INTERVAL;
    //timspec.it_value.tv_sec = 0;
    timspec.it_value.tv_nsec =1;

    int timerfd = timerfd_create(CLOCK_MONOTONIC,0);
    timerfd_settime(timerfd, 0, &timspec, 0);
    //timerfd_settime()
    //fprintf(stderr,"timer1=%d\n",timer1);

    struct pollfd ufds[2];
    ufds[0].fd = timerfd;
    ufds[0].events = POLLIN; //| POLLPRI; // check for normal or out-of-band

    ufds[1].fd = socket1;
    ufds[1].events = POLLIN; //| POLLPRI; // check for normal or out-of-band

    int rv;
    uint64_t loopcount=0;
    uint32_t frameid=0;
    //uint32_t acks=0;

    Clock flintClock;
    Clock flintOffsetClock;
    flintOffsetClock=initClock();
    flintClock=getProgramClock(flintOffsetClock);


//    uint32_t testdata=0XABCDEF01;
    uint32_t testdata = 0x00000001;

    for(;;)
    {
        rv = poll(ufds, 2, 1);
        //testloopcount++;
        if (rv == -1) 
        {
                perror("poll"); // error occurred in poll()
        } else if (rv == 0) 
        {
        // DO Nothing
        
        } else 
        {
            if (ufds[0].revents & POLLIN) // Timer
            {
                // Used to Send Packets at Regular Intervals
                read(timerfd, &loopcount, sizeof(uint64_t)); //reset Timer

                // Ensure to re write every field of espDataPacket or
                // do sendpacket=data_ntoh(sendpacket) first.
                flintClock=getProgramClock(flintOffsetClock);

                sendpacket.prot_header   = 0;
                sendpacket.frameid       = frameid;
                sendpacket.cmd           = 20;
                sendpacket.data          = testdata;
                sendpacket.ptime_sec     = flintClock.seconds;
                sendpacket.ptime_usec    = flintClock.useconds;
                sendpacket.clockadj_usec = 0;

                //acks=0;

                printf("SENT Packet FrameID=  %d\n",sendpacket.frameid);

                sendpacket=data_hton(sendpacket);
                if (sendto(socket1, &sendpacket, sizeof(espDataPacket), 0, (struct sockaddr *)&remaddr, slen)==-1) 
                {
                    perror("sendto");
                    exit(1);
                }
                frameid++;
                testdata ^= 0x00000001;

                }
            if (ufds[1].revents & POLLIN) // Recive Socket
            {
                recvlen = recvfrom(socket1, &recvpacket, sizeof(recvpacket), 0, (struct sockaddr *)&remaddr, (socklen_t *)&slen);
                if (recvlen > 0) 
                {
                    recvpacket=ack_ntoh(recvpacket);
                    Print_espAckPacket(recvpacket);
                }
            }
        }

    }
	return 0;
}


void print_flint_help()
{
    printf("PlaceHolder for useful tips\n");
}

void print_greating(void)
{
    fprintf(stderr, "Steel:Entangled Sparks: FM Transmitter Contoller for Entagngled Sparks Version %1.2f\n", VESRION);
    fprintf(stderr, "Copyright (C)2014 Josh Gardiner josh@zool.com\n");
    fprintf(stderr, "\nThis program comes with ABSOLUTELY NO WARRANTY; for details type -w\n");
    fprintf(stderr, "This is free software, and you are welcome to redistribute it\n");
    fprintf(stderr, "under certain conditions; type -c for details.\n");
    fprintf(stderr, "\ntype -h for help\n\n");
}
