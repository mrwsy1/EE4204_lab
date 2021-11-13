#include "headsock.h"

void str_ser(int sockfd, struct sockaddr *addr, int addrlen);

int main(int argc, char *argv[])
{
	int sockfd, ret;
	struct sockaddr_in my_addr;

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) { //create socket
		printf("error in socket");
		exit(1);
	}

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(MYUDP_PORT);
	my_addr.sin_addr.s_addr = INADDR_ANY;
	bzero(&(my_addr.sin_zero), 8);
	ret = bind(sockfd, (struct sockaddr *) &my_addr, sizeof(struct sockaddr)); //allocate local socket number
	if (ret <0) {
		printf("error in binding");
		exit(1);
	}
	printf("start receiving\n");
	
	str_ser(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr_in));
		
	close(sockfd);
	exit(0);
}


void str_ser(int sockfd, struct sockaddr *addr, int addrlen){
	char buf[BUFSIZE];
	FILE *fp;
	char recvs[DATALEN];
	struct ack_so ack;
	int end, n = 0;
	long lseek=0;
	double random_probability = 0.0;
	double error_probability = 0.5;
	end = 0;
	
	while(!end){
		n= recvfrom(sockfd, &recvs, DATALEN, 0, addr, (socklen_t *)&addrlen); //receive data
		
		random_probability = (double)rand() / (double)RAND_MAX; //error simulation
		if (random_probability < error_probability){
			ack.num = 2;
			ack.len = 0;
			if ((n = sendto(sockfd, &ack, 2, 0, addr, addrlen))==-1){ //send NACK (2)
				printf("send error!");
				exit(1);
			}
			printf("NACK\n");
			continue;
		}
		
		if (n==-1){
			printf("error when receiving\n");
			exit(1);
		}
			
		if (recvs[n-1] == '\0'){
			end = 1;
			n --;
		}
		memcpy((buf+lseek), recvs, n);
		lseek += n;
		
		ack.num = 1;
		ack.len = 0;
		if ((n = sendto(sockfd, &ack, 2, 0, addr, addrlen))==-1){ //send ACK (1)
			printf("send error!");
			exit(1);
		}
		printf("ACK\n");
	}
	
	if ((fp = fopen ("myfile_receive.txt","wt")) == NULL){ //write file
		printf("File doesn't exit\n");
		exit(0);
	}
	fwrite (buf , 1 , lseek , fp);
	fclose(fp);
	printf("a file has been successfully received!\nthe total data received is %d bytes\n", (int)lseek);
}
