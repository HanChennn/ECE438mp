/*
** client.c -- a stream socket client demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#define PORT "3490" // the port client will be connecting to 

#define MAXDATASIZE 100 // max number of bytes we can get at once 

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
	int sockfd, numbytes;  
	char buf[MAXDATASIZE],hostname[MAXDATASIZE],port[MAXDATASIZE], out1[MAXDATASIZE], out2[MAXDATASIZE], wget[MAXDATASIZE*4];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];
	char *h=NULL, *h_=NULL, *h_what=NULL;
	FILE *fp;

	if (argc != 2) {
	    fprintf(stderr,"usage: client hostname\n");
	    exit(1);
	}

	memset(port,0,sizeof(port));
	memset(hostname,0,sizeof(hostname));
	memset(out1,0,sizeof(out1));
	memset(out2,0,sizeof(out2));
	memset(wget,0,sizeof(wget));

	h_what = strchr(argv[1],':');
	if ((h = strchr(h_what+1,':'))!=NULL){
		h_ = strchr(h,'/');
		// printf("%s\n%s\n",h,h_);
		memcpy(port, h+1, h_-h-1);
		memcpy(hostname, h_what+3, (int)(h-h_what)-3);
	}else{
		h = strchr(h_what+3, '/');
		memcpy(port, "80", 2);
		memcpy(hostname, h_what+3, (int)(h-h_what)-3);
	}
	printf("host name is : %s\n",hostname);
	printf("port is : %s\n", port);

	memcpy(out1, "wget ", 5);
	memcpy(out2, strchr(argv[1],'/')+2, (int)strlen(argv[1])-(strchr(argv[1],'/')-argv[1]+2));
	strcat(strcat(out1,out2),"\r\n");
	printf("out2 is %s\n", out2);
	printf("out1 is %s\n", out1);

	printf("%s\n",argv[1]);
	memcpy(wget, "GET /", 5);
	memcpy(out2, strchr(out2,'/')+1, (int)strlen(out2)-(int)(strchr(out2,'/')-out2)-1);
	strcat(strcat(hostname,":"),port);
	strcat(strcat(strcat(wget, out2)," HTTP/1.1\r\nUser-Agent: Wget/1.12 (linux-gnu)\r\nHost: "),hostname);
	strcat(wget, "\r\nConnection: Keep-Alive\r\n");
	printf("%s\n",wget);

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if ((rv = getaddrinfo(argv[1], port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("client: connect");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}
	

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure

	send(sockfd, wget, strlen(wget), 0);

	if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
	    perror("recv");
	    exit(1);
	}

	buf[numbytes] = '\0';

	// store the file
	fp = fopen("output","w");
	if (fp==NULL){
		perror("write error");
		exit(0);
	}else{
		fputs(buf, fp);
		printf("finish writing into a file\n");
		fclose(fp);
	}

	printf("client: received '%s'\n",buf);

	close(sockfd);

	return 0;
}

