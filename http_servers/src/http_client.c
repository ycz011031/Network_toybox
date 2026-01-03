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
#include <stdbool.h>

//#define PORT "3490" // the port client will be connecting to 

#define MAXDATASIZE 4096 // max number of bytes we can get at once 

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
	int sockfd;  
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

	if (argc != 2) {
	    fprintf(stderr,"usage: client hostname\n");
	    exit(1);
	}

	// resolving host IP, PORT and file name
	const char *url = argv[1];
	if (strncmp(url,"http://",7)==0) {
		url += 7;
	}else {
		fprintf(stderr,"invalid url\n");
		exit(1);
	}
	const char *port_idx = strchr(url,':');
	const char *path_idx = strchr(url,'/');
	if (!path_idx) {
    	fprintf(stderr,"No File specified\n");
	    exit(1);
	}
	char host_ip[256];
	int host_ip_length;
	uint16_t port = 80;
	if (port_idx && (port_idx < path_idx)){
		host_ip_length = port_idx-url;
		port = atoi(port_idx+1);
	}else{
		host_ip_length = path_idx - url;
		port = 80;
	}
	strncpy(host_ip, url, host_ip_length);
	const char *path = path_idx;

	char port_str[6];
	snprintf(port_str, sizeof(port_str), "%u", port);
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;


	// --------Preparing HTTP get request----

	char request[256];
	snprintf(request, sizeof(request),
         "GET %s HTTP/1.1\r\n\r\n",
         path);
		 
	if ((rv = getaddrinfo(host_ip, port_str, &hints, &servinfo)) != 0) {
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

	if (send(sockfd,request,strlen(request),0)==-1){
		perror("client send request error");
	}

	FILE *outfile = fopen("output","wb");
	if(!outfile){
		perror("Client fopen failed");
		exit(1);
	}

	int n;
	bool header_done = false;

	while ((n=recv(sockfd,buf,sizeof(buf),0))>0){
		if(header_done){
			fwrite(buf,1,n,outfile);
		}else{
			char *header_end = strstr(buf,"\r\n\r\n");
			if(header_end){
				header_done = true;
				int header_len = header_end - buf;
				char header_line[128];
				strncpy(header_line,buf,header_len);
				header_line[header_len]='\0';

				int code;
				if (sscanf(header_line,"HTTP/%*s %d", &code)==1){
					if(code !=200){
						printf("%s\n", header_line);
						exit(1);
					
					}else{
						printf("http status ok");
						char*body_start = header_end+4;
						int body_len = n - (body_start - buf);
						fwrite(body_start,1,body_len,outfile);
					}
				}

			}
			//char*body = strstr(buf,"");
		}
	}
	if (n==-1) perror("recv");
	fclose(outfile);


	close(sockfd);

	return 0;
}

