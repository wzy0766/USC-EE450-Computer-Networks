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

#define MYPORT "31680"
#define SERVERPORT "32680"
#define MAXBUFLEN 100
#define NAMELEN 24
#define USERNOTFOUND -2

struct MainQuery {
	int type;
	char country[NAMELEN];
	int id;
};
struct ReplyMain {
	int count;
	char countries[10][NAMELEN];
};

class Node {
public:
	int id;
	set<int> neighbours;
};

class Country {
public:
	string name;
	vector<Node> nodes;
};
void sendReply(const char *data, int len);
vector<Country*> countries;
void loadData() {
	ifstream in("data2.txt");
	Country *pCountry = NULL;
	while (in) {
		string line;
		getline(in, line);
		if (line.length() == 0) {
			break;
		}
		if (line[line.length() - 1] == '\n') {
			line = line.substr(0, line.length() - 1);
		}
		if (line[line.length() - 1] == '\r') {
			line = line.substr(0, line.length() - 1);
		}
		if (std::isdigit(line[0])) {
			stringstream ss(line);
			int n;
			ss >> n;
			Node node;
			node.id = n;
			while (ss >> n) {
				node.neighbours.insert(n);
			}
			pCountry->nodes.push_back(node);
		} else {
			pCountry = new Country();
			pCountry->name = line;
			countries.push_back(pCountry);
		}
	}
	if (countries.empty()) {
		cout << "Node data in input file" << endl;
		exit(-1);
	}
}

int countCommon(const Node *pNode, const Node *pOther) {
	int n = 0;
	for (set<int>::const_iterator iter = pNode->neighbours.begin();
			iter != pNode->neighbours.end(); iter++) {
		if (pOther->neighbours.find(*iter) != pOther->neighbours.end()) {
			n++;
		}
	}
	return n;
}

int maxCommon(const Node *pNode, const vector<const Node*> &nodes) {
	int maxId = -1;
	int maxN = 0;
	for (size_t i = 0; i < nodes.size(); i++) {
		int n = countCommon(pNode, nodes[i]);
		if (n > maxN || (n == maxN && nodes[i]->id < maxId)) {
			maxId = nodes[i]->id;
			maxN = n;
		}
	}
	return maxId;
}

int maxDegree(const vector<const Node*> &nodes) {
	int maxId = -1;
	int maxN = 0;
	for (size_t i = 0; i < nodes.size(); i++) {
		int n = nodes[i]->neighbours.size();
		if (n > maxN || (n == maxN && nodes[i]->id < maxId)) {
			maxId = nodes[i]->id;
			maxN = n;
		}
	}
	return maxId;
}

int findInCoutry(const vector<Node> &nodes, int id) {
	vector<const Node*> candidates;
	Node const *pNode = NULL;
	for (size_t i = 0; i < nodes.size(); i++) {
		if (id == nodes[i].id) {
			pNode = &nodes[i];
			break;
		}
	}
	if (!pNode) {
		// no such node
		return USERNOTFOUND;
	}
	for (size_t i = 0; i < nodes.size(); i++) {
		if (id != nodes[i].id
				&& pNode->neighbours.find(nodes[i].id)
						== pNode->neighbours.end()) {
			candidates.push_back(&nodes[i]);
		}
	}
	if (candidates.empty()) {
		// None
		return -3;
	}
	int result = maxCommon(pNode, candidates);
	if (result == -1) {
		result = maxDegree(candidates);
	}

	return result;
}

int find(const string &country, int id) {
	for (size_t i = 0; i < countries.size(); i++) {
		if (country == countries[i]->name) {
			return findInCoutry(countries[i]->nodes, id);
		}
	}
	// no such country
	return -1;
}

// reference http://www.beej.us/guide/bgnet/html/#datagram
void setupUDP() {
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

	if ((rv = getaddrinfo(NULL, MYPORT, &hints, &servinfo)) != 0) {
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

	cout << "The Server B is up and running using UDP on port " << MYPORT
			<< endl;
	while (true) {
		addr_len = sizeof their_addr;
		MainQuery query;
		if ((numbytes = recvfrom(sockfd, &query, sizeof(query), 0,
				(struct sockaddr*) &their_addr, &addr_len)) == -1) {
			perror("recvfrom");
			exit(1);
		}
		if (query.type == 0) {
			// init
			ReplyMain reply;
			reply.count = countries.size();
			int i = 0;
			for (vector<Country*>::iterator iter = countries.begin();
					iter != countries.end(); iter++) {
				strcpy(reply.countries[i], (*iter)->name.c_str());
				i++;
			}
			sendReply((const char*) &reply, (int) sizeof(reply));
			cout << "The Server B has sent a country list to Main Server"
					<< endl;
		} else {
			// find
			cout
					<< "The Server B has received request for finding possible friends of User "
					<< query.id << " in " << query.country << endl;
			int id = find(query.country, query.id);
			char msg[MAXBUFLEN] = "None";
			if (id == USERNOTFOUND) {
				sprintf(msg, "User %d not found", query.id);
				cout << "The Server B has sent \"User " << query.id
						<< " not found\" to Main Server" << endl;
			} else if (id >= 0) {
				sprintf(msg, "%d", id);
				cout
						<< "The Server B is searching possible friends for User<userID>"
						<< endl << "Here are the results:" << id << endl;
				cout << "The Server B has sent the result(s) to Main Server"
						<< endl;
			} else {
				cout
						<< "The Server B is searching possible friends for User<userID>"
						<< endl << "Here are the results: None" << endl;
				cout << "The Server B has sent the result(s) to Main Server"
						<< endl;
			}
			sendReply(msg, sizeof(msg));
		}
	}
}

void sendReply(const char *data, int len) {
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	if ((rv = getaddrinfo("localhost", SERVERPORT, &hints, &servinfo)) != 0) {
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
	loadData();
//	int id = find("xYz", 0);
//	cout << id << endl;
	setupUDP();

	return 0;
}
