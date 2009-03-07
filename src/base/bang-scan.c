//Uses code from Beej's guide
//TODO- Catch errors from functions
#include "bang-scan.h"
#include"bang-scan.h"
#include"bang-signals.h"
#include"bang-types.h"
#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<pthread.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<netdb.h>

//Change this to be inputtable
#define BRDCSTPORT "4950"
#define SCAN_MSG_LEN 5

pthread_t scanThread;

//-----------------Local----------------------------------------------

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

static int sendBangReply(char *reply_ip) {

    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    if ((rv = getaddrinfo(reply_ip, BRDCSTPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and make a socket
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
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
        BANG_scan_close();
    }

    freeaddrinfo(servinfo);

#ifdef BDEBUG_1
    printf("Acknowledged new peer (%s)\n", reply_ip);
#endif
    close(sockfd);

    return 0;

}

static int listenForNewPeers() {
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    struct sockaddr_storage new_peer_addr;
    char buf[SCAN_MSG_LEN];
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
#ifdef BDEBUG_1
    printf("Listening for new peers...\n");
#endif
    addr_len = sizeof new_peer_addr;
    if ((numbytes = recvfrom(sockfd, buf, SCAN_MSG_LEN, 0,
        (struct sockaddr *)&new_peer_addr, &addr_len)) == -1) {
        perror("recvfrom");
        return 1;
    }

    close(sockfd);

    //If an open call was sent
    if (strncmp(buf, "BANG?", SCAN_MSG_LEN) == 0)
	//Answer it
        if (sendBangReply(inet_ntop(new_peer_addr.ss_family, get_in_addr((struct sockaddr *)&new_peer_addr), s, sizeof s)) != 0)
		return 1;
    //Otherwise if it was an answer to our call
    else if (strncmp(buf, "BANG.", SCAN_MSG_LEN) == 0) {
	//Create signal argument (new peer address)
	BANG_sigargs scanArg;
	scanArg.args = &new_peer_addr;
	scanArg.args = sizeof(struct sockaddr_storage);
	//Send signal with address for processing
	if (BANG_send_signal(BANG_PEER_WANTS_TO_CONNECT, &scanArg, 1) == -1)
		BANG_scan_close();
    }

    return 0;

}

static int announceExistenceToLAN() {
    int sockfd;
    struct sockaddr_in new_peer_addr; // connector's address information
    struct hostent *he;
    int numbytes;
    int broadcast = 1;

    if ((he=gethostbyname("255.255.255.255")) == NULL) {  // get the host info
        herror("gethostbyname");
        BANG_scan_close();
    }

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket");
        BANG_scan_close();
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast,
        sizeof broadcast) == -1) {
        perror("setsockopt (SO_BROADCAST)");
        BANG_scan_close();
    }

    new_peer_addr.sin_family = AF_INET;     // host byte order
    new_peer_addr.sin_port = htons(atoi(BRDCSTPORT)); // short, network byte order
    new_peer_addr.sin_addr = *((struct in_addr *)he->h_addr);
    memset(new_peer_addr.sin_zero, '\0', sizeof new_peer_addr.sin_zero);

    if ((numbytes=sendto(sockfd, "BANG?", strlen("BANG?"), 0,
             (struct sockaddr *)&new_peer_addr, sizeof new_peer_addr)) == -1) {
        perror("sendto");
        BANG_scan_close();
    }

#ifdef BDEBUG_1
    printf("Broadcasting my existence to LAN...\n");
#endif

    close(sockfd);

    return 0;

}

static void *scanControl(void *dontUse) {
	dontUse = NULL;
	int retVal = 0;

	announceExistenceToLAN();
	while (1) {
		if ((retVal = listenForNewPeers()) != 0)
			fprintf(stderr, "Fail on scan: %d\n", retVal);
	}

	return;
}
//--------------------------------------------------------------------

//------------------Global--------------------------------------------

//Create working thread
void BANG_scan_init() {
	pthread_create(&scanThread, NULL, scanControl, NULL);
}

//Kill working thread
void BANG_scan_close() {
	//DIE DIE DIE
	pthread_kill(scanThread, SIGKILL); //If possible, change this to pthread_cancel()
	pthread_join(scanThread, NULL);
}
