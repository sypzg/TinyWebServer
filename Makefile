all : Server

Server : Server.c TinyWebServer.o 
	gcc -o Server Server.c TinyWebServer.o -g
	
TinyWebServer.o: TinyWebServer.c 
	gcc -o TinyWebServer.o -c TinyWebServer.c 

.PHONLY:clean
clean:
	rm -rf *.o Server all