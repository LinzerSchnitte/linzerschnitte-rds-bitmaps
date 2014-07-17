//Steel -LinzerSchnitte Entangled Sparks Transmitter Software
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
#include <arpa/inet.h>
#include <assert.h>
#include <unistd.h>
#include <inttypes.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/timerfd.h>
#include <sched.h>

#include "util.h"
#include "esp.h"
#include "esp_time.h"
#include "udp.h"
#include "i2c_bitbang.h"

#define BUFFERSIZE 32
#define VESRION 1.10
#define RDS_PACKET_INTERVAL 180000000 //190ms

typedef enum {STANDALONE, UDPCLIENT} MODE;

void print_steel_help(void);
void print_greating(void);

int main(int argc, char **argv)
{
    print_greating(); // Prints greating

    espDataPacket recvpacket; // We Recive espData Packets
    espAckPacket sendpacket;  // We Send espAck Packets

    int port = 1535; // Default Listen Port

    int cmd_option;
    // Parse Options
    while((cmd_option=getopt(argc, argv, "hp:wc")) != EOF)
        switch(cmd_option)
        {
            default:
            case 'h': print_steel_help(); 
            case 'p': port=atoi(optarg); break;
            case 'w': print_warranty();
            case 'c': print_conditions();
        }
    
    // UNCOMMENT TO FOR RDS COMMANDS TO WORK


    //Init GPIO
    
    if (gpioSetup() != OK)
    {
            dbgPrint(DBG_INFO, "gpioSetup failed. Exiting\n");
            return 1;
    }

    LS_CMD(10,0xffff,0);
    sleep(1);
    LS_CMD(10,0xffff,0);
    sleep(1);
    RDS_CONFIG();
    sleep(1);
    RDS_ENABLE_UDG1();
    sleep(1);
    //END GPIO 

    
    uint32_t acks=0;
    uint32_t acks_wrap_buffer=0;
    uint32_t lastFrameID=0;
    uint32_t currentFrameID=0;

    // UDP SOCKET SETUP
    struct sockaddr_in myaddr;
    struct sockaddr_in remaddr;
    socklen_t addrlen = sizeof(remaddr);
    int recvlen;
    int recvsocket;
    int tick=0;

    if ((recvsocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
    {
        perror("cannot create socket\n");
        return 0;
    }

    memset((char *)&myaddr, 0, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY); // listen on all interfaces
    myaddr.sin_port = htons(port);

    if (bind(recvsocket, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) 
    {
        perror("bind failed");
        return 0;
    }
    // END UDP SOCKET SETUP

    // START POLLING SETUP
    int rv;
    

    struct sched_param schedparm;
    memset(&schedparm, 0, sizeof(schedparm));
    schedparm.sched_priority = 1; // lowest rt priority
    sched_setscheduler(0, SCHED_FIFO, &schedparm);

    struct itimerspec timspec;
    bzero(&timspec, sizeof(timspec));
    timspec.it_interval.tv_sec = 0;
    timspec.it_interval.tv_nsec = RDS_PACKET_INTERVAL;
    timspec.it_value.tv_nsec =1;

    int timerfd = timerfd_create(CLOCK_MONOTONIC,0);
    timerfd_settime(timerfd, 0, &timspec, 0);

    struct pollfd ufds[2];
    ufds[0].fd = timerfd;
    ufds[0].events = POLLIN; //| POLLPRI; // check for normal or out-of-band

    ufds[1].fd = recvsocket;
    ufds[1].events = POLLIN; //| POLLPRI; // check for normal or out-of-band
    uint64_t loopcount=0;

    Clock steelClock;
    Clock steelOffsetClock;
    steelOffsetClock=initClock();
    steelClock=getProgramClock(steelOffsetClock);

    // Create New Queue
    RDS_Queue rdsqueue;
    rdsqueue=QueueCreate();

    // Create RDS Queue Command
    RDS_Command recvcmd, sendcmd;
    sendcmd.cmd = 0;
    sendcmd.data = 0;

    for(;;)
    {
        rv = poll(ufds, 2, 1);
        //tick++;
        if (rv == -1) 
        {
            perror("poll"); // error occurred in poll()
        } else if (rv == 0) 
        {
            // Do Nothing

        } else 
        {
            if (ufds[0].revents & POLLIN) 
            {
                read(timerfd, &loopcount, sizeof(uint64_t)); // reset Timer
                // Consume BUFFER AND Send RDS DATA
                // EXAMPLE RDS
                if (!QueueEmpty(rdsqueue))
                {
                    sendcmd=QueueDelete(rdsqueue);
                }
                LS_RAW2(sendcmd.cmd,sendcmd.data,0);


                printf("RDS COMMAND %u RDS DATA %08X\n",sendcmd.cmd,sendcmd.data);
                tick++;
            }

            if (ufds[1].revents & POLLIN) 
            {
                recvlen = recvfrom(recvsocket, &recvpacket, sizeof(espDataPacket), 0, (struct sockaddr *)&remaddr, &addrlen);
                if (recvlen > 0)
                {
                    recvpacket=data_ntoh(recvpacket);
                    printf("RECV Packet FrameID=  %d\n",recvpacket.frameid);
                    Print_espDataPacket(recvpacket);
                } else 
                {
                    printf("uh oh - something went wrong!\n");
                }
                steelClock=getProgramClock(steelOffsetClock);
                // Send Acks..........
                lastFrameID=currentFrameID;
                currentFrameID=recvpacket.frameid;
                sendpacket.prot_header  = recvpacket.prot_header;
                sendpacket.frameid      = recvpacket.frameid;
                sendpacket.ptime_sec    = recvpacket.ptime_sec;
                sendpacket.ptime_usec   = recvpacket.ptime_usec;
                sendpacket.acktime_sec  = steelClock.seconds;
                sendpacket.acktime_usec = steelClock.useconds;


                // Ack Bitfield
                if ( (currentFrameID-lastFrameID)>0 )
                {
                    acks <<= (currentFrameID-lastFrameID);
                    acks |= acks_wrap_buffer>> (BUFFERSIZE-(currentFrameID-lastFrameID));
                    acks_wrap_buffer <<= (currentFrameID-lastFrameID);
                    acks |= 1;
                    sendpacket.acks=acks;
                } else if ( (currentFrameID-lastFrameID)<0 )
                {
                    acks_wrap_buffer >>= (lastFrameID-currentFrameID) ;
                    acks_wrap_buffer |= (acks<<(BUFFERSIZE-(lastFrameID-currentFrameID)));
                    acks >>= (lastFrameID-currentFrameID);
                    acks |= 0xFFFFFFFF<<(BUFFERSIZE-(lastFrameID-currentFrameID));
                    acks |= 1;
                    sendpacket.acks=acks;
                }
                // Send ACK Packet          
                sendpacket=ack_hton(sendpacket);
                if (sendto(recvsocket, &sendpacket, sizeof(espAckPacket), 0, (struct sockaddr *)&remaddr, addrlen) < 0)
                {
                perror("sendto");
                }
                recvcmd.cmd=recvpacket.cmd;
                recvcmd.data=recvpacket.data;
                QueueEnter(rdsqueue,recvcmd);
                // Enter RDS CMD INTO QUEUE


            }
        }
    } 

    return 0;
}


void print_steel_help()
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
