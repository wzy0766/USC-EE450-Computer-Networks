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
#include <map>
#include <vector>
#include <sstream>
using namespace std;

#define MYTCPPORT "33680"
#define MYUDPPORT "32680"
#define SERVERAPORT "30680"
#define SERVERBPORT "31680"
#define MAXBUFLEN 100
#define NAMELEN 24

struct MainQuery {
	int type;
	char country[NAMELEN];
	int id;
};
struct ReplyMain {
	int count;
	char countries[10][NAMELEN];
};
struct QueryMain {
	char country[NAMELEN];
	int id;
};

void sendRequest(const char *data, int len, const char *port);

// reference http://www.beej.us/guide/bgnet/html/#datagram
int setupUDP(map<string, string> &countryMap) {
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;
	struct sockaddr_storage their_addr;
	socklen_t addr_len;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, MYUDPPORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		exit(1);
	}

	// loop through all the results and bind to the first we can
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol))
				== -1) {
			perror("listener: socket");
			continue;
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("listener: bind");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "listener: failed to bind socket\n");
		exit(2);
	}

	freeaddrinfo(servinfo);

	cout << "The Main server is up and running." << endl;

	addr_len = sizeof their_addr;

	MainQuery query;
	query.type = 0;
	sendRequest((const char*) &query, (int) sizeof(query), SERVERAPORT);
	ReplyMain reply;
	if ((numbytes = recvfrom(sockfd, &reply, sizeof(reply), 0,
			(struct sockaddr*) &their_addr, &addr_len)) == -1) {
		perror("recvfrom");
		exit(1);
	}
	cout
			<< "The Main server has received the country list from server A using UDP over port "
			<< MYUDPPORT << endl;
	for (int i = 0; i < reply.count; i++) {
		countryMap[reply.countries[i]] = SERVERAPORT;
		cout << reply.countries[i] << endl;
	}

	sendRequest((const char*) &query, (int) sizeof(query), SERVERBPORT);
	if ((numbytes = recvfrom(sockfd, &reply, sizeof(reply), 0,
			(struct sockaddr*) &their_addr, &addr_len)) == -1) {
		perror("recvfrom");
		exit(1);
	}
	cout
			<< "The Main server has received the country list from server B using UDP over port "
			<< MYUDPPORT << endl;
	for (int i = 0; i < reply.count; i++) {
		countryMap[reply.countries[i]] = SERVERBPORT;
		cout << reply.countries[i] << endl;
	}
	return sockfd;
}
// reference http://www.beej.us/guide/bgnet/html/#a-simple-stream-server
void sigchld_handler(int s) {
	// waitpid() might overwrite errno, so we save and restore it:
	int saved_errno = errno;

	while (waitpid(-1, NULL, WNOHANG) > 0)
		;

	errno = saved_errno;
}

void setupTCP(map<string, string> &countryMap, int udp) {
	int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes = 1;
	int rv;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, MYTCPPORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		exit(1);
	}

	// loop through all the results and bind to the first we can
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol))
				== -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))
				== -1) {
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

	freeaddrinfo(servinfo); // all done with this structure

	if (p == NULL) {
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}

	if (listen(sockfd, 10) == -1) {
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

	while (1) {  // main accept() loop
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr*) &their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}

		if (!fork()) { // this is the child process
			close(sockfd); // child doesn't need the listener
			// recv client
			QueryMain query;
			if (recv(new_fd, &query, sizeof(query), 0) == -1)
				perror("recv");
			cout << "The Main server has received the request on User "
					<< query.id << " in " << query.country
					<< " from client using TCP over port " << MYTCPPORT << endl;
			// find map
			map<string, string>::iterator iter = countryMap.find(
					string(query.country));
			char msg[MAXBUFLEN];
			if (iter != countryMap.end()) {
				const char *s = "A";
				if (iter->second == SERVERBPORT) {
					s = "B";
				}
				cout << query.country << " shows up in server A/B" << endl;
				// send to back server
				MainQuery req;
				req.type = 1;
				req.id = query.id;
				strcpy(req.country, query.country);
				sendRequest((const char*) &req, sizeof(req),
						iter->second.c_str());
				cout << "The Main Server has sent request from User "
						<< query.id << " to server A/B using UDP over port "
						<< MYUDPPORT << endl;
				// recvfrom back server
				struct sockaddr_storage their_addr;
				socklen_t addr_len = sizeof(their_addr);
				if (recvfrom(udp, msg, sizeof(msg), 0,
						(struct sockaddr*) &their_addr, &addr_len) == -1) {
					perror("recvfrom");
					exit(1);
				}
				cout
						<< "The Main server has received searching result(s) of User "
						<< query.id << " from server" << s << endl;
				if (strncmp(msg, "User", 4) == 0) {
					cout
							<< "The Main server has received \"User ID: Not found\" from server "
							<< s << endl;
					cout
							<< "The Main Server has sent error to client using TCP over"
							<< MYTCPPORT << endl;
				} else {
					cout
							<< "The Main Server has sent searching result(s) to client using TCP over port "
							<< MYTCPPORT << endl;
				}
			} else {
				sprintf(msg, "%s does not show up in server A&B",
						query.country);
				cout
						<< "The Main Server has sent \"Country Name: Not found\" to client1/2 using TCP over port "
						<< MYTCPPORT << endl;
			}
			// send client
			if (send(new_fd, msg, sizeof(msg), 0) == -1)
				perror("send");

			close(new_fd);
			exit(0);
		}
		close(new_fd);  // parent doesn't need this
	}
}

void sendRequest(const char *data, int len, const char *port) {
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	if ((rv = getaddrinfo("localhost", port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		exit(1);
	}

	// loop through all the results and make a socket
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol))
				== -1) {
			perror("talker: socket");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "talker: failed to create socket\n");
		return exit(2);
	}

	if ((numbytes = sendto(sockfd, data, len, 0, p->ai_addr, p->ai_addrlen))
			== -1) {
		perror("talker: sendto");
		exit(1);
	}

	freeaddrinfo(servinfo);

	close(sockfd);

}

int main() {
	map<string, string> countryMap;
	int udp = setupUDP(countryMap);
	setupTCP(countryMap, udp);

	return 0;
}
