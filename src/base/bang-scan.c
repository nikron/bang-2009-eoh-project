/**
 * \file bang-scan.c
 */

/*
 * listener.c -- a datagram sockets "server" demo
 * using code from beej's guide
 */

#include"bang-scan.h"
#include"bang-signals.h"
#include"bang-types.h"
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<netdb.h>

#define BRDCSTPORT "4950"
#define HNDSHKPORT "4951"

#define MAXBUFLEN 100

// get sockaddr, IPv4 or IPv6:
static void *get_in_addr(struct sockaddr *sa) {
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

static int bang_acknowledge(const char *reply_ip) {
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	if ((rv = getaddrinfo(reply_ip, HNDSHKPORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and make a socket
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("socket");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "failed to bind socket\n");
		return 2;
	}

	if ((numbytes = sendto(sockfd, "BANG.", strlen("BANG."), 0,
					p->ai_addr, p->ai_addrlen)) == -1) {
		perror("sendto");
		exit(1);
	}

	freeaddrinfo(servinfo);

	printf("Acknowledged new user (%s)\n", reply_ip);
	close(sockfd);

	return 0;

}

int listen_for_announce_thread() {
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;
	struct sockaddr_storage their_addr;
	char buf[MAXBUFLEN];
	size_t addr_len;
	char s[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, BRDCSTPORT, &hints, &servinfo)) == -1) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
						p->ai_protocol)) == -1) {
			perror("socket");
			continue;
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("bind");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "failed to bind socket\n");
		return 2;
	}

	freeaddrinfo(servinfo);

	printf("Listening for new clients...\n");

	addr_len = sizeof their_addr;
	if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
					(struct sockaddr *)&their_addr, &addr_len)) == -1) {
		perror("recvfrom");
		exit(1);
	}

	printf("New user (%s)\n",
			inet_ntop(their_addr.ss_family,
				get_in_addr((struct sockaddr *)&their_addr),
				s, sizeof s));
	buf[numbytes] = '\0';
	printf("packet contains \"%s\"\n", buf);

	close(sockfd);

	if (strcmp(buf, "BANG?") == 0)
		bang_acknowledge(inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s));
	else if (strcmp(buf, "BANG.") == 0)
		printf("Setting up connection with new user.\n");

	return 0;

}

int BANG_announce() {
	int sockfd;
	struct sockaddr_in their_addr; /*  connector's address information */
	struct hostent *he;
	int numbytes;
	int broadcast = 1;

	if ((he=gethostbyname("255.255.255.255")) == NULL) {  /* TODO: Use get addrinfo. */
		herror("gethostbyname");
		exit(1);
	}

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}

	// this call is the difference between this program and talker.c:
	if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast,
				sizeof broadcast) == -1) {
		perror("setsockopt (SO_BROADCAST)");
		exit(1);
	}

	their_addr.sin_family = AF_INET;     // host byte order
	their_addr.sin_port = htons(atoi(BRDCSTPORT)); // short, network byte order
	their_addr.sin_addr = *((struct in_addr *)he->h_addr);
	memset(their_addr.sin_zero, '\0', sizeof their_addr.sin_zero);

	if ((numbytes=sendto(sockfd, "BANG?", strlen("BANG?"), 0,
					(struct sockaddr *)&their_addr, sizeof their_addr)) == -1) {
		perror("sendto");
		exit(1);
	}

	printf("Broadcasting my existence to LAN...\n", numbytes, inet_ntoa(their_addr.sin_addr));

	close(sockfd);

	return 0;

}
