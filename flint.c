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
#include "sparkfunc.h"

#define BUFFERSIZE 2048

float version = 1.00;

double processData(char *message, int messageLength);

int main(int argc, char**argv)
{
	fprintf(stderr, "Flint:Entangled Sparks: Frame Contoller for Entagngled Sparks Version %1.2f\n", version);
	fprintf(stderr, "Copyright (C)2014 Josh Gardiner josh@zool.com\n");
	fprintf(stderr, "\nThis program comes with ABSOLUTELY NO WARRANTY; for details type -w'\n");
	fprintf(stderr, "This is free software, and you are welcome to redistribute it\n");
	fprintf(stderr, "under certain conditions; type -c for details.\n");
	fprintf(stderr, "\ntype -h for help\n\n");

	int cmd_option;
	int port = 1535;
	char *server = "127.0.0.1";
	int delay = 1000;
	int testpattern=0;

	while((cmd_option=getopt(argc, argv, "hp:s:t:wcr:")) != EOF)
	switch(cmd_option)
	{
		default:
		case 'h': print_flint_help();break;
		case 'p': port=atoi(optarg); break;
		case 's': server=(optarg); break;
		case 't': delay=atoi(optarg); break;
		case 'w': print_warranty();break;
		case 'c': print_conditions();break;
		case 'r': testpattern=atoi(optarg);break;
	}

	struct sockaddr_in myaddr;
	struct sockaddr_in remaddr;

	int fd;
	int i;
	int slen=sizeof(remaddr);

	char buf[BUFFERSIZE];
	int recvlen;


	if ((fd=socket(AF_INET, SOCK_DGRAM, 0))==-1)
		printf("socket created\n");


	memset((char *)&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddr.sin_port = htons(0);

	if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
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
	short shiftleft1=1;
	short shiftleft2=1;
	short shiftleft3=1;
	short shiftleft4=1;
	uint64_t data01 = 0X00000020;
	uint64_t data02 = 0X00000040;
	uint64_t data03 = 0X00000080;
	uint64_t data04 = 0X00000100;

	if(testpattern==2)
	{
		data01 = 0x00000000;
		data02 = 0x00000000;
		data03 = 0x00000000;
		data04 = 0x00000000;
	}

	if(testpattern==4)
	{
		data01 = 0x00000000;
		data02 = 0x00000000;
	}
	if(testpattern==5)
	{
		data01 = 0x00000000;
		data02 = 0x00000001;
	}

	if(testpattern==3)
	{
		data01 = 0xFFFFFFFF;
	}

        if (testpattern == 6)
        {
            data01 = 0x00000001;
            data02 = 0x00000005;
        }

	double timenow;
	//double acktime;
	//double roundtriptime;
	while(1)
	{


		for (i=0; i < 1000000 ; i++) {
			timenow=getTime();
			printf("Sending Frame %d to %s port %d\n", i, server, port);
			sprintf(buf, "FRAMEID=%d\nTIMESTAMP=%8.8f\nDATA01=%llu\nDATA02=%llu\nDATA03=%llu\nDATA04=%llu\n"
				, i, timenow,data01,data02,data03,data04);
			if (sendto(fd, buf, strlen(buf), 0, (struct sockaddr *)&remaddr, slen)==-1) {
				perror("sendto");
				exit(1);
			}

			recvlen = recvfrom(fd, buf, BUFFERSIZE, 0, (struct sockaddr *)&remaddr, (socklen_t *)&slen);
					if (recvlen >= 0) {
							buf[recvlen] = 0;
							processData(buf,recvlen);
							fprintf(stderr, "Latency=%lf\n", ((getTime()-timenow)) );
					}

		if(testpattern==1)
		{
                    if (data01 == 0X80000000){ shiftleft1=0; }
                    if (shiftleft1==1) {data01 <<= 1;}
                    if (shiftleft1==0) {data01 >>= 1;}
                    if (data01 == 0X00000001) {shiftleft1=1;}
                    
                    if (data02 == 0X80000000){ shiftleft2=0; }
                    if (shiftleft2==1) {data02 <<= 1;}
                    if (shiftleft2==0) {data02 >>= 1;}
                    if (data02 == 0X00000001) {shiftleft2=1;}
                    
                    if (data03 == 0X80000000){ shiftleft3=0; }
                    if (shiftleft3==1) {data03 <<= 1;}
                    if (shiftleft3==0) {data03 >>= 1;}
                    if (data03 == 0X00000001) {shiftleft3=1;}
                    
                    if (data04 == 0X80000000){ shiftleft4=0; }
                    if (shiftleft4==1) {data04 <<= 1;}
                    if (shiftleft4==0) {data04 >>= 1;}
                    if (data04 == 0X00000001) {shiftleft4=1;}
		}
		if(testpattern==2)
		{
                    data01 ^= 0b11111111111111111111111111111111;
                    data02 ^= 0b11111111111111111111111111111111;
                    data03 ^= 0b11111111111111111111111111111111;
//                    data04 ^= 0b11111111111111111111111111111111;
                    if (data04 == 0)
                        data04 = 0xFFFFFFFF;
                    else
                        data04 = 0x00000000;
		}

		if(testpattern==4)
		{
			data01 ^= 0b00000000000000000000000000000001;
			data02 ^= 0b00000000000000000000000000000001;
		}
		if(testpattern==5)
		{
			//data01 ^= 0b00000000000000000000000000000001;
			data02 ^= 0b00000000000000000000000000000001;
		}
                if (testpattern == 6)
                {
                    data01 = data01 << 1;
                    data02 = data02 << 1;

                    if ( (data01 & 0x000000FF) == 0)
                    {
                        data01 = 0x00000001;
                    }

                    if ( (data02 & 0x000000FF) == 0)
                    {
                        data02 = 0x00000005;
                    }
                }

		usleep(1000*delay);

		}

	}

	close(fd);
	return 0;
}

double processData(char *message, int messageLength)
{
	char FRAMEIDstr[100];
	double recivedtime;

	sscanf(message, "%s %lf",FRAMEIDstr, &recivedtime);

	//fprintf(stderr, "%s\n %s\n %s\n %s\n %s\n %s\n",FRAMEIDstr, TIMESTAMPstr, DATA01str, DATA02str, DATA03str, DATA04str);

	if (memcmp(FRAMEIDstr,"FRAMEID=",8)==0 ) {
		//data = split(FRAMEIDstr,delim);
		fprintf(stderr,"%s ACK\n",FRAMEIDstr);
	}

	//fprintf(stderr,"RecivedTime=%lf \n", recivedtime);
	return (recivedtime);

}
