#include "TinyWebServer.h"
#include <sys/signal.h>
#define MAXLINE 8192
//fucntion declaration
void doit(int fd);
void read_requesthdrs(rio_t* rp);
int parse_uri(char* uri, char *filename, char* cgiargs);
void serve_static(int fd, char* filename, int filesize);
void get_filetype(char *filename, char* filetype);
void serve_dynamic(int fd, char* filename, char* cgiargs);
void clienterror(int fd, char* cause, char* errnum, char* shortmsg, char* longmsg);

//main body
int main(int argc, char* argv[])
{

    if(argc != 3)
    {
        printf("usage: %s <ip> <port>\n",basename(argv[0]));
        exit(1);
    }

    int listenfd = openlisenfd(argv[2],atoi(argv[3]));

    while(1)
    {
        int connfd = acceptREQ(listenfd);
        doit(connfd);
        closecon(connfd);
    }
    closelis(listenfd);
}

void diot(int fd)
{
    int is_static;
    struct stat sbuf;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char filename[MAXLINE], cgiargs[MAXLIN];
    
    rio_t rio;
    /*Read requst line and headers*/
    Rio_readinitb(&rio, fd);
    Rio_readlineb(&rio, buf, MAXLINE);
    printf("Request headers:\n");
    printf("%s", buf);
    sscanf(buf, "%s %s %s", method, uri, version);
    if(strcasecmp(method, "GET"))
    {
        clienterror(fd, method, "501", "Not implement", "Tiny does not implement this method");
        return;
    }
    read_requesthdrs(&rio);

    /*Parse URI from GET request*/
    is_static = parse_uri(uri, filename, cgiargs);
    if(stat(filename, &sbuf) < 0)
    {
        clienterror(fd, filename, "404", "Not found", "Tiny couldn`t find this file");
        return;
    }
    if(is_static)/*Server static content*/
    {
        if(!(SI_SIGIO(sbuf.st_mode))
    }
}