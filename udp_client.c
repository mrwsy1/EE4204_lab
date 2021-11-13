#include "headsock.h"

float str_cli(FILE *fp, int sockfd, struct sockaddr *addr, int addrlen, long *len);       
void tv_sub(struct  timeval *out, struct timeval *in);

int main(int argc, char *argv[]) {
	int sockfd;
	float ti, rt;
	long len;
	struct sockaddr_in ser_addr;
	char ** pptr;
	struct hostent *sh;
	struct in_addr **addrs;
	FILE *fp;

	if (argc!= 2) {
		printf("parameters not match.");
		exit(0);
	}
	
	sh=gethostbyname(argv[1]); //get host's information
	if (sh==NULL) {
		printf("error when gethostbyname");
		exit(0);
	}

	sockfd = socket(AF_INET, SOCK_DGRAM, 0); //create socket
	if (sockfd<0) {
		printf("error in socket");
		exit(1);
	}

	addrs = (struct in_addr **)sh->h_addr_list;
	printf("canonical name: %s\n", sh->h_name);
	for (pptr=sh->h_aliases; *pptr != NULL; pptr++)
		printf("the aliases name is: %s\n", *pptr);
	switch(sh->h_addrtype)
	{
		case AF_INET:
			printf("AF_INET\n");
		break;
		default:
			printf("unknown addrtype\n");
		break;
	}

	ser_addr.sin_family = AF_INET;
	ser_addr.sin_port = htons(MYUDP_PORT);
	memcpy(&(ser_addr.sin_addr.s_addr), *addrs, sizeof(struct in_addr));
	bzero(&(ser_addr.sin_zero), 8);
	
	if((fp = fopen ("myfile.txt","r+t")) == NULL) { //read file
		printf("File doesn't exit\n");
		exit(0);
	}

	//receive and send
	ti = str_cli(fp, sockfd, (struct sockaddr *)&ser_addr, sizeof(struct sockaddr_in), &len);
	rt = (len/(float)ti);
	
	printf("Time(ms) : %.3f, Data sent(byte): %d\nData rate: %f (Kbytes/s)\n", ti, (int)len, rt);

	close(sockfd);
	fclose(fp);

	exit(0);
}


float str_cli(FILE *fp, int sockfd, struct sockaddr *addr, int addrlen, long *len) {
	char *buf;
	long lsize, ci; //size of file, current index in buff
	char sends[DATALEN];
	struct ack_so ack;
	int n, slen; //size of string to send
	float time_inv = 0.0;
	struct timeval sendt, recvt;
	int received;
	ci = 0;

	fseek (fp , 0 , SEEK_END);
	lsize = ftell (fp);
	rewind (fp);
	printf("The file length is %d bytes\n", (int)lsize);
	printf("the packet length is %d bytes\n",DATALEN);

	// allocate memory to contain the whole file.
	buf = (char *) malloc (lsize+1);
	if (buf == NULL) exit (2);

	// copy the file into the buffer.
	fread (buf,1,lsize,fp);
	
	buf[lsize] ='\0';
	gettimeofday(&sendt, NULL);
	
	while(ci<= lsize) {
		if ((lsize+1-ci) <= DATALEN) //send data in size of datalen
			slen = lsize+1-ci;
		else 
			slen = DATALEN;
			
		memcpy(sends, (buf+ci), slen);
				
		if((n = sendto(sockfd, &sends, slen, 0, addr, addrlen))== -1) { //send data
			printf("send error!");
			exit(1);
		}
		
		received=0;
		
		while(!received){ //stop and wait
			if ((n= recvfrom(sockfd, &ack, 2, 0, addr, (socklen_t*)&addrlen))==-1) {
				printf("error when receiving\n");
				exit(1);
			}
			
			if (ack.num==1 && ack.len==0){
				printf("ACK\n");
				ci += slen;
				received=1;
			}
			else if (ack.num==2 && ack.len==0){
				printf("NACK\n");
				n = sendto(sockfd, &sends, slen, 0, addr, addrlen);
			}
			else if (ack.num != 1|| ack.len != 0)
				printf("error in transmission\n");
			else {
				ci += slen;
				received=1;
			}
		}
	}
	
	gettimeofday(&recvt, NULL);
	*len= ci;
	tv_sub(&recvt, &sendt);
	time_inv += (recvt.tv_sec)*1000.0 + (recvt.tv_usec)/1000.0;
	return(time_inv);
}


void tv_sub(struct  timeval *out, struct timeval *in) {
	if ((out->tv_usec -= in->tv_usec) <0) {
		--out ->tv_sec;
		out ->tv_usec += 1000000;
	}
	out->tv_sec -= in->tv_sec;
}
