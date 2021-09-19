#include "TinyWebServer.h"
/*********************************************************************
 * The Rio package - robust I/O functions
 **********************************************************************/
ssize_t rio_readn(int fd, void* usrbuf, size_t n)
{
    size_t nleft = n;
    ssize_t nread;
    char *bufp = usrbuf;

    while(nleft > 0)
    {
        errno = 0;        
        if((nread = read(fd, bufp, nleft)) < 0)
        {
            if(errno == EINTR)/*Interrupted by sig handler return*/
                nread = 0;
            else
                return -1;
        }
        else if(nread == 0) 
            break;
        
        nleft -= nread;
        bufp += nread;     
    }
    return n-nleft;
}

ssize_t rio_writen(int fd, void* usrbuf, size_t n)
{
    size_t nleft = n;
    ssize_t nwritten;
    char *bufp = usrbuf;

    while(nleft > 0)
    {
        errno = 0;
        if((nwritten = write(fd, bufp, nleft)) <= 0)
        {
            if(errno = EINTR)
                nwritten = 0;
            else
                return -1;

        }
        
        nleft -= nwritten;
        bufp += nleft;
    }
    return n;
}

void rio_readinitb(rio_t *rp, int fd)
{
    rp->rio_bufptr = rp->rio_buf;
    rp->rio_fd = fd;
    rp->rio_cnt = 0;
}

static ssize_t rio_read(rio_t* rp, char* usrbuf, size_t n)
{
    int cnt;
    
    while(rp->rio_cnt <= 0)/*refill if buf is empty*/
    {
        errno = 0;
        rp->rio_cnt = read(rp->rio_fd, rp->rio_buf, sizeof(rp->rio_buf));
        if(rp->rio_cnt < 0)
            if(errno != EINTR)
                return -1;
        else if(rp->rio_cnt == 0)
            return 0;
        else 
            rp->rio_bufptr = rp->rio_buf;
    }

    /*Copy min(n,rp->rio_cnt) bytes from internal buf to user buf*/
    cnt = n;
    
    if(rp->rio_cnt < n)
        cnt = rp->rio_cnt;
    
    memcpy(usrbuf, rp->rio_bufptr, cnt);
    rp->rio_bufptr += cnt;
    rp->rio_cnt -= cnt;
    return cnt;
}

ssize_t rio_readlineb(rio_t* rp, void *usrbuf, size_t maxlen)
{
    int n;
    ssize_t cn;
    char c,*buf = usrbuf;
    for(n=1;n<maxlen;n++)
    {
        if((cn = rio_read(rp, &c, 1)) == 1)
        {
            *(buf++) = c;
            if(c == '\n')
            {
                n++;
                break;
            } 
        }
        else if(cn == 0)
        {
            if(n==1)
                return 0;
            else 
                break;
        }
        else
            return -1;
    }
    *buf = 0;
    return n-1;
}

ssize_t rio_readnb(rio_t* rp, void *usrbuf, size_t n)
{
    size_t nleft = n;
    ssize_t nread;
    char *buf = usrbuf;
    while (nleft > 0)
    {
        if((nread = rio_read(rp, buf, n)) < 0)
            return -1;
        else if(nread == 0)
            break;
    
        nleft -= nread;
        buf += nread;
    }
    return n-nleft;
}


/**********************************
 * Wrappers for robust I/O routines
 **********************************/
ssize_t Rio_readn(int fd, void *ptr, size_t nbytes) 
{
    ssize_t n;
  
    if ((n = rio_readn(fd, ptr, nbytes)) < 0)
	unix_error("Rio_readn error");
    return n;
}

void Rio_writen(int fd, void *usrbuf, size_t n) 
{
    if (rio_writen(fd, usrbuf, n) != n)
	unix_error("Rio_writen error");
}

void Rio_readinitb(rio_t *rp, int fd)
{
    rio_readinitb(rp, fd);
} 

ssize_t Rio_readnb(rio_t *rp, void *usrbuf, size_t n) 
{
    ssize_t rc;

    if ((rc = rio_readnb(rp, usrbuf, n)) < 0)
	unix_error("Rio_readnb error");
    return rc;
}

ssize_t Rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen) 
/*********************************************************************
 * Client/Server helper functions
 **********************************************************************/
int openlisenfd(char* ip, int port)
{
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htonl(port);
    inet_pton(AF_INET, ip, address.sin_addr);
    
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd <= 0)
        unix_error("create socket failed");
    int ret = bind(listenfd, (struct sockaddr*)&address, sizeof(address));
    if(ret == -1)
        unix_error("bind socket failed");
    ret = listen(listenfd, 5);
        unix_error("listen socket failed");
    return listenfd;
}

int acceptREQ(int fd)
{
    struct sockaddr_in client_address;
    socklen_t client_addrlen = sizeof(client_address);
    int connfd = accept(fd, (struct sockaddr*)&client_address, &client_addrlen);
    if(connfd < 0)
        unix_error("accept request failed");
    int ret = getnameinfo((struct sockaddr*)&client_address, client_addrlen, host, MAXLINE, sername, MAXLINE, 0);
    if(ret != 0)
    printf("Accepted connection from (%s, %d)\n",hostname,ntohl(client_address.sin_port));
    else
    fprintf(stderr,"%s: %s\n","getnameinfo failed", strerror(errno));
    
    return connfd;
}

void closeconn(int fd)
{
    if(close(fd) < 0)
        unix("close connected socket failed");
}

void closelis(int fd)
{
    if(close(fd) < 0)
        unix("close listening socket");
}
/************************** 
 * Error-handling functions
 **************************/
/* $begin errorfuns */
/* $begin unixerror */
void unix_error(char *msg) /* Unix-style error */
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(0);
}
/* $end unixerror */







