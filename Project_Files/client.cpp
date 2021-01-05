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
#include <sys/wait.h>
#include <iostream>
#include <fstream>
#include <string>
#include <set>
#include <vector>
#include <sstream>
using namespace std;

#define SERVERPORT "33680"
#define NAMELEN 24
#define MAXBUFLEN 100

struct QueryMain {
	char country[NAMELEN];
	int id;
};
// reference http://www.beej.us/guide/bgnet/html/#a-simple-stream-client
int main() {
	int sockfd, numbytes;
	struct addrinfo hints, *servinfo, *p;
	int rv;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo("localhost", SERVERPORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol))
				== -1) {
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

	freeaddrinfo(servinfo); // all done with this structure

	cout << "Client is up and running" << endl;
	QueryMain query;
	cout << "Please enter the User ID: ";
	cin >> query.id;
	cout << "Please enter the Country Name:";
	cin >> query.country;

	if ((numbytes = send(sockfd, &query, sizeof(query), 0)) == -1) {
		perror("send");
		exit(1);
	}
	cout << "Client has sent User " << query.id << " and " << query.country
			<< " to Main Server using TCP" << endl;
	char msg[MAXBUFLEN];
	if ((numbytes = recv(sockfd, &msg, sizeof(msg), 0)) == -1) {
		perror("recv");
		exit(1);
	}
	if (strlen(msg) > 10) {
		cout << msg << endl;
	} else if (string("None") == msg) {
		cout << "Client has received results from Main Server:" << endl
				<< "No user is/are possible friend(s) of User" << query.id
				<< " in " << query.country << endl;
	} else {
		cout << "Client has received results from Main Server:" << endl
				<< "User " << msg << " is/are possible friend(s) of User"
				<< query.id << " in " << query.country << endl;
	}
	close(sockfd);
	return 0;
}
