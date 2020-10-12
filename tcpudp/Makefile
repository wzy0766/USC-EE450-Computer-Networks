all: serverA.out serverB.out mainserver.out client
serverA.out: serverA.cpp
	g++ -o serverA.out serverA.cpp
serverB.out: serverB.cpp
	g++ -o serverB.out serverB.cpp
mainserver.out: servermain.cpp
	g++ -o mainserver.out servermain.cpp
client: client.cpp
	g++ -o client client.cpp
serverA:
	./serverA.out
serverB:
	./serverB.out
mainserver:
	./mainserver.out
clean:
	rm -f serverA.out serverB.out mainserver.out client
	