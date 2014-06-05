//Steel -LinzerSchnitte Entangled Sparks Transmitter Software
//Josh Gardiner
//2014 06 05

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <assert.h>
#include "sparkfunc.h"
#include <unistd.h>

#define BUFSIZE 2048
float version = 1.00;

uint32_t frameid;
uint32_t data01 = 0;
uint32_t data02 = 0;
uint32_t data03 = 0;
uint32_t data04 = 0;
double timestamp = 0;

void processData(char *message, int messageLength);

int main(int argc, char **argv)
{
	fprintf(stderr, "Flint: Contoller for Entagngled Sparks\n");
	fprintf(stderr, "Version %1.2f <josh@zool.com> use -h for help\n\n", version);

	int cmd_option;
	int port = 21234;

	while((cmd_option=getopt(argc, argv, "hp:")) != EOF)
	switch(cmd_option)
	{
		default:
		case 'h': print_flint_help();
		case 'p': port=atoi(optarg); break;
	}

	struct sockaddr_in myaddr;
	struct sockaddr_in remaddr;
	socklen_t addrlen = sizeof(remaddr);
	int recvlen;
	int fd;

	char buf[BUFSIZE];

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("cannot create socket\n");
		return 0;
	}

	memset((char *)&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddr.sin_port = htons(port);

	if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
		perror("bind failed");
		return 0;
	}

	printf("waiting on port %d\n", port);

	for (;;) {
		recvlen = recvfrom(fd, buf, BUFSIZE, 0, (struct sockaddr *)&remaddr, &addrlen);
		system("clear");
		if (recvlen > 0) {
			buf[recvlen] = 0;
			processData(buf,recvlen);
		}
		else
			printf("uh oh - something went wrong!\n");
		sprintf(buf, "FRAMEID=%d\n%6.6f\n", frameid, getTime());
		printf("sending response %s", buf);
		if (sendto(fd, buf, strlen(buf), 0, (struct sockaddr *)&remaddr, addrlen) < 0)
			perror("sendto");

	}

}



void processData(char *message, int messageLength)
{
	char FRAMEIDstr[100], TIMESTAMPstr[100], DATA01str[100], DATA02str[100], DATA03str[100], DATA04str[100];

	sscanf(message, "%s %s %s %s %s %s",FRAMEIDstr, TIMESTAMPstr, DATA01str, DATA02str, DATA03str, DATA04str);
	
	//fprintf(stderr, "%s\n %s\n %s\n %s\n %s\n %s\n",FRAMEIDstr, TIMESTAMPstr, DATA01str, DATA02str, DATA03str, DATA04str);

	char *data;
	const char delim[2] = "=";

	if (memcmp(FRAMEIDstr,"FRAMEID=",8)==0 ) {
		data = split(FRAMEIDstr,delim);
		frameid = atoi(data);
		fprintf(stderr,"FRAMEID=%d\n",frameid);
	}
	if (memcmp(TIMESTAMPstr,"TIMESTAMP=",10)==0 ) {
		data = split(TIMESTAMPstr,delim);
		timestamp = atof(data);
		fprintf(stderr,"TIMESTAMP=%6.6f\n",timestamp);
	}
	if (memcmp(DATA01str,"DATA01=",7)==0 ) {
		data = split(DATA01str,delim);
		data01 = atoi(data);
		fprintf(stderr,"%08X = ",data01);
		printbitssimple(data01);
		fprintf(stderr,"\n");
	}
	if (memcmp(DATA02str,"DATA02=",7)==0 ) {
		data = split(DATA02str,delim);
		data02 = atoi(data);
		fprintf(stderr,"%08X = ",data02);
		printbitssimple(data02);
		fprintf(stderr,"\n");
	}
	if (memcmp(DATA03str,"DATA03=",7)==0 ) {
		data = split(DATA03str,delim);
		data03 = atoi(data);
		fprintf(stderr,"%08X = ",data03);
		printbitssimple(data03);
		fprintf(stderr,"\n");
	}
	if (memcmp(DATA04str,"DATA04=",7)==0 ) {
		data = split(DATA04str,delim);
		data04 = atoi(data);
		fprintf(stderr,"%08X = ",data04);
		printbitssimple(data04);
		fprintf(stderr,"\n");
	}

}
