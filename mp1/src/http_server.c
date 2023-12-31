/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT "3490"  // the port users will be connecting to

#define BACKLOG 10	 // how many pending connections queue will hold

#define MAXDATASIZE 2048

void sigchld_handler(int s)
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

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
	int sockfd, new_fd, byte_received, byte_read;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN], buf[MAXDATASIZE], reply[MAXDATASIZE], addr[MAXDATASIZE];
	const char correct[]="HTTP/1.1 200 OK\r\n";
	const char not_find[]="HTTP/1.1 404 Not Found\r\n";
	const char o_err[]="HTTP/1.1 400 Bad Request\r\n";
	char *pp=NULL;
	int rv;
	FILE* fp;

	memset(buf,0,sizeof(buf));
	memset(reply,0,sizeof(reply));
	memset(addr,0,sizeof(addr));

	if (argc != 2) {
	    fprintf(stderr,"usage: lacking server port number\n");
	    exit(1);
	}
	printf("port:%s\n",argv[1]);

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		return 2;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	// if(fopen("file.txt","rb")==NULL)printf("NO!\n");
	// else printf("YES!\n");
	// if(fopen("/Downloads/ECE438mp/mp1/file.txt","rb")==NULL)printf("NO!\n");
	// else printf("YES!\n");
	printf("server: waiting for connections...\n");

	while(1) {  // main accept() loop
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}

		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof s);
		printf("server: got connection from %s\n", s);

		if (!fork()) { // this is the child process
			close(sockfd); // child doesn't need the listener
			
			memset(buf,0,sizeof(buf));
			memset(reply,0,sizeof(reply));
			byte_received = recv(new_fd, buf, sizeof(buf)-1, 0);
			if(byte_received<=0)perror("recv error!");
			buf[byte_received]='\0';
			printf("%s\n",buf);

			if ((pp = strstr(buf,"GET"))!=NULL){
				pp = strchr(buf,'/');
				memcpy(addr, pp+1, (int)(strchr(pp+1,' ')-pp)-1);
				printf("fetch addr:%s\n",addr);

				if ((pp = strstr(buf,"HTTP/"))!=NULL){
					if((fp = fopen(addr,"rb"))!=NULL){
						strcat(memcpy(reply, correct, strlen(correct)),"\r\n");
					}else{
						strcat(memcpy(reply, not_find, strlen(not_find)),"\r\n");
					}
					
				}else{
					strcat(memcpy(reply, o_err, strlen(o_err)),"\r\n");
				}
			}else{
				strcat(memcpy(reply, o_err, strlen(o_err)),"\r\n");
			}
			printf("reply:%s\n",reply);

			if (send(new_fd, reply, strlen(reply), 0) == -1)
				perror("server send error");

			if(fp!=NULL){
				while(1){
					memset(buf,'\0',sizeof(buf));
					if ((byte_read = fread(buf, sizeof(char), MAXDATASIZE-1,fp))>0){
						printf("%d\n",byte_read);
						printf("%s\n",buf);
						if(send(new_fd, buf, byte_read, 0) == -1){
							perror("server file send error");
							break;
						}
					}else break;
				}
				fclose(fp);
			}
			
			close(new_fd);
			exit(0);
		}
		close(new_fd);  // parent doesn't need this
	}

	return 0;
}

