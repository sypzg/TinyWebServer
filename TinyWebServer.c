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
{
    ssize_t rc;

    if ((rc = rio_readlineb(rp, usrbuf, maxlen)) < 0)
	unix_error("Rio_readlineb error");
    return rc;
}

/*********************************************************************
 * Client/Server helper functions
 **********************************************************************/
int openlisenfd(char* ip, int port)
{
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    
    inet_pton(AF_INET, ip, &address.sin_addr);
    
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd <= 0)
        unix_error("create socket failed");
    int ret = bind(listenfd, (struct sockaddr*)&address, sizeof(address));
    if(ret == -1)
        unix_error("bind socket failed");
    ret = listen(listenfd, 5);
    if(ret == -1)
        unix_error("listen socket failed");
    return listenfd;
}

int acceptREQ(int fd)
{
    struct sockaddr_in client_address;
    socklen_t client_addrlen = sizeof(client_address);
    char host[MAXLINE];
    char server[MAXLINE];
    int connfd = accept(fd, (struct sockaddr*)&client_address, &client_addrlen);
    if(connfd < 0)
        unix_error("accept request failed");
    int ret = getnameinfo((struct sockaddr*)&client_address, client_addrlen, host, MAXLINE, server, MAXLINE, 0);
    if(ret != 0)
    printf("Accepted connection from (%s, %d)\n",host,ntohl(client_address.sin_port));
    else
    fprintf(stderr,"%s: %s\n","getnameinfo failed", strerror(errno));
    
    return connfd;
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


/********************************
 * Wrappers for Unix I/O routines
 ********************************/
int Open(const char *pathname, int flags, mode_t mode) 
{
    int rc;

    if ((rc = open(pathname, flags, mode))  < 0)
	unix_error("Open error");
    return rc;
}

ssize_t Read(int fd, void *buf, size_t count) 
{
    ssize_t rc;

    if ((rc = read(fd, buf, count)) < 0) 
	unix_error("Read error");
    return rc;
}

ssize_t Write(int fd, const void *buf, size_t count) 
{
    ssize_t rc;

    if ((rc = write(fd, buf, count)) < 0)
	unix_error("Write error");
    return rc;
}

off_t Lseek(int fildes, off_t offset, int whence) 
{
    off_t rc;

    if ((rc = lseek(fildes, offset, whence)) < 0)
	unix_error("Lseek error");
    return rc;
}

void Close(int fd) 
{
    int rc;

    if ((rc = close(fd)) < 0)
	unix_error("Close error");
}

int Select(int  n, fd_set *readfds, fd_set *writefds,
	   fd_set *exceptfds, struct timeval *timeout) 
{
    int rc;

    if ((rc = select(n, readfds, writefds, exceptfds, timeout)) < 0)
	unix_error("Select error");
    return rc;
}

int Dup2(int fd1, int fd2) 
{
    int rc;

    if ((rc = dup2(fd1, fd2)) < 0)
	unix_error("Dup2 error");
    return rc;
}

void Stat(const char *filename, struct stat *buf) 
{
    if (stat(filename, buf) < 0)
	unix_error("Stat error");
}

void Fstat(int fd, struct stat *buf) 
{
    if (fstat(fd, buf) < 0)
	unix_error("Fstat error");
}

/***************************************
 * Wrappers for memory mapping functions
 ***************************************/
void *Mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset) 
{
    void *ptr;

    if ((ptr = mmap(addr, len, prot, flags, fd, offset)) == ((void *) -1))
	unix_error("mmap error");
    return(ptr);
}

void Munmap(void *start, size_t length) 
{
    if (munmap(start, length) < 0)
	unix_error("munmap error");
}

/*********************************************
 * Wrappers for Unix process control functions
 ********************************************/

/* $begin forkwrapper */
pid_t Fork(void) 
{
    pid_t pid;

    if ((pid = fork()) < 0)
	unix_error("Fork error");
    return pid;
}
/* $end forkwrapper */

void Execve(const char *filename, char *const argv[], char *const envp[]) 
{
    if (execve(filename, argv, envp) < 0)
	unix_error("Execve error");
}

/* $begin wait */
pid_t Wait(int *status) 
{
    pid_t pid;

    if ((pid  = wait(status)) < 0)
	unix_error("Wait error");
    return pid;
}
/* $end wait */

pid_t Waitpid(pid_t pid, int *iptr, int options) 
{
    pid_t retpid;

    if ((retpid  = waitpid(pid, iptr, options)) < 0) 
	unix_error("Waitpid error");
    return(retpid);
}

/* $begin kill */
void Kill(pid_t pid, int signum) 
{
    int rc;

    if ((rc = kill(pid, signum)) < 0)
	unix_error("Kill error");
}
/* $end kill */

void Pause() 
{
    (void)pause();
    return;
}

unsigned int Sleep(unsigned int secs) 
{
    unsigned int rc;

    if ((rc = sleep(secs)) < 0)
	unix_error("Sleep error");
    return rc;
}

unsigned int Alarm(unsigned int seconds) {
    return alarm(seconds);
}
 
void Setpgid(pid_t pid, pid_t pgid) {
    int rc;

    if ((rc = setpgid(pid, pgid)) < 0)
	unix_error("Setpgid error");
    return;
}

pid_t Getpgrp(void) {
    return getpgrp();
}









