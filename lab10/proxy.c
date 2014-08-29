/*
 * proxy.c - CS:APP Web proxy
 *
 * TEAM MEMBERS:
 *     Andrew Carnegie, ac00@cs.cmu.edu 
 *     Harry Q. Bovik, bovik@cs.cmu.edu
 * 
 * IMPORTANT: Give a high level description of your code here. You
 * must also provide a header comment at the beginning of each
 * function that describes what that function does.
 */

 /*Method:producer - consumer
  *I think the method will have the advantage in thread_create and thread_close
  *Name:    Gao ce
  *ID:      5120379091
  */
 
#include "csapp.h"

#define NTHREADS 16//the program has 16 working threads
#define SBUFSIZE 256//the size of the buf

/*
 * Function prototypes
 */
int parse_uri(char *uri, char *target_addr, char *path, int  *port);
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, char *uri, int size);
int open_clientfdSafe(char *hostname, int port);

typedef struct
{
    struct sockaddr_in clientaddr;
    int clientfd;
}clientFdAddr;

typedef struct
{
     clientFdAddr **buf;
     int n;
     int front;
     int rear;
     sem_t mutex;
     sem_t slots;
     sem_t items;
}sbuf_t;//for the con

void sbuf_init(sbuf_t *sp, int n);
void sbuf_deinit(sbuf_t *sp);
void sbuf_insert(sbuf_t *sp, clientFdAddr *client);
clientFdAddr *sbuf_remove(sbuf_t *sp);

void doit(clientFdAddr *client);

int sendRequest(int serverfd, char *buf, char *method, char *uri, char *version, rio_t *Rio);

void *thread(void *vargp);

void clientCheck(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
ssize_t Rio_readnOfMine(int fd, void *ptr, size_t n);//for the binary read and write between the in and the app;
void Rio_writenOfMine(int fd, void *usrbuf, size_t n);//for the binary read and write between the in and the app;
ssize_t Rio_readlinebOfMine(rio_t *rp, void *usrbuf, size_t maxlen); 
// void Rio_writenOfMine1(int fd, void *usrbuf, size_t n);

sem_t mutexGeneral, mutexForLog;
sbuf_t sbuf;

/* 
 * main - Main routine for the proxy program 
 */
int main(int argc, char **argv)
{
    int listenfd = -1, port = -1;
    int i = 0;
    socklen_t clientlen;
    struct sockaddr_in clientaddrx;
    clientFdAddr *client;
    pthread_t tid;
    
    //initial
    Sem_init(&mutexGeneral, 0, 1);
    Sem_init(&mutexForLog, 0, 1);
    sbuf_init(&sbuf, SBUFSIZE);

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <port number>\n", argv[0]);
        exit(0);
    }

    signal(SIGPIPE, SIG_IGN);

    clientlen = sizeof(clientaddrx);

    port = atoi(argv[1]);
    listenfd = open_listenfd(port);

    for(i = 0; i < NTHREADS; i++)//produce 16 worker threads
          Pthread_create(&tid, NULL, thread, NULL);

    while (1) 
    {
        client = (clientFdAddr *)Malloc(sizeof(clientFdAddr));
        client->clientfd = Accept(listenfd, (struct sockaddr*)&(client->clientaddr), &clientlen);//wait for the request
        sbuf_insert(&sbuf, client);
    }
    exit(0);
}

void *thread(void *vargp)
{
    Pthread_detach(pthread_self());

    while(1)
    {//come from 12.5.5
        clientFdAddr *conn = sbuf_remove(&sbuf);//to lock and unlock
        doit(conn);
        // close(conn->clientfd);
    }
}

void doit(clientFdAddr *client)
{
    signal(SIGPIPE, SIG_IGN);

     char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE], logString[MAXLINE], returnAccept[MAXLINE];
     char serverName[MAXLINE], path[MAXLINE];
     int port = -1, clientfd = -1, serverfd = -1, logfd = -1, n = -1, cnt = 0;
     rio_t clientRio, serverRio, logRio;
     struct sockaddr_in clientaddr;

     clientfd = client->clientfd;
     clientaddr = client->clientaddr;
     // Free(client);

     printf("server client %d\n", clientfd);

     //initialize the proxy as a server to the client
     Rio_readinitb(&clientRio, clientfd);
     if ( Rio_readlinebOfMine(&clientRio, buf, MAXLINE) < 0)
     {
        clientCheck(clientfd, NULL, "404","Not Found", "The page is not found.");
        return;
     }

     sscanf(buf, "%s %s %s", method, uri, version);

     printf("%s %s %s \n", method, uri, version);

     //if it is "get"
     if (strcasecmp(method, "GET"))
     {
        clientCheck(clientfd, method, "501", "Not Implement", "proxy does not implenment this method");
        return;
     }

    parse_uri(uri, serverName, path, &port);

    printf("server:%s\n", serverName);

    if ((serverfd = open_clientfdSafe(serverName, port)) < 0)
    {//maybe wrong
       strcpy(returnAccept, "Something wrong, The server is not working?");
       printf("to client:%s\n", returnAccept);
       Rio_writenOfMine(clientfd, returnAccept, strlen(returnAccept));
       return;
    }

    sendRequest(serverfd, buf, method, path, version, &clientRio);//send the request to the srever

    Rio_readinitb(&serverRio, serverfd);
    bzero(buf, MAXLINE);

    while ((n = Rio_readlinebOfMine(&serverRio, buf, MAXLINE)) > 0)
    {  
       printf("%s", buf);//print the return value
       if ((rio_writen(clientfd, buf, n)) != n)
          break; 
       cnt += n;
       bzero(buf, MAXLINE);
    }

    format_log_entry(logString, &clientaddr, uri, cnt);
    strcat(logString, "\n");

    P(&mutexForLog);

    logfd = Open("proxy.log", O_CREAT|O_WRONLY|O_APPEND, DEF_MODE);
    Rio_readinitb(&logRio, logfd);
    Rio_writen(logfd, logString, strlen(logString));
    Close(logfd);

    V(&mutexForLog);

    printf("close client%d\n", clientfd);
    Close(clientfd);
    Close(serverfd);
    return;
}   
/*
 * parse_uri - URI parser
 * 
 * Given a URI from an HTTP proxy GET request (i.e., a URL), extract
 * the host name, path name, and port.  The memory for hostname and
 * path must already be allocated and should be at least MAXLINE
 * bytes. Return -1 if there are any problems.
 */
int parse_uri(char *uri, char *hostname, char *path, int *port)
{
    char *hostbegin;
    char *hostend;
    char *pathbegin;
    int len;

    if (strncasecmp(uri, "http://", 7) != 0) {
    hostname[0] = '\0';
    return -1;
    }
       
    /* Extract the host name */
    hostbegin = uri + 7;
    hostend = strpbrk(hostbegin, " :/\r\n\0");
    len = hostend - hostbegin;
    strncpy(hostname, hostbegin, len);
    hostname[len] = '\0';
    
    /* Extract the port number */
    *port = 80; /* default */
    if (*hostend == ':')   
    *port = atoi(hostend + 1);
    
    /* Extract the path */
    pathbegin = strchr(hostbegin, '/');
    if (pathbegin == NULL) {
    path[0] = '\0';
    }
    else {
    strcpy(path, pathbegin);
    }

    return 0;
}
/*
 * format_log_entry - Create a formatted log entry in logstring. 
 * 
 * The inputs are the socket address of the requesting client
 * (sockaddr), the URI from the request (uri), and the size in bytes
 * of the response from the server (size).
 */
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, 
              char *uri, int size)
{
    time_t now;
    char time_str[MAXLINE];
    unsigned long host;
    unsigned char a, b, c, d;
    short port;
    /* Get a formatted time string */
    now = time(NULL);
    strftime(time_str, MAXLINE, "%a %d %b %Y %H:%M:%S %Z", localtime(&now));

    /* 
     * Convert the IP address in network byte order to dotted decimal
     * form. Note that we could have used inet_ntoa, but chose not to
     * because inet_ntoa is a Class 3 thread unsafe function that
     * returns a pointer to a static variable (Ch 13, CS:APP).
     */
    host = ntohl(sockaddr->sin_addr.s_addr);
    a = host >> 24;
    b = (host >> 16) & 0xff;
    c = (host >> 8) & 0xff;
    d = host & 0xff;

    port = ntohs(sockaddr->sin_port);
    /* Return the formatted log entry string */
    sprintf(logstring, "%s: %d.%d.%d.%d : %u %s %d", time_str, a, b, c, d, port, uri, size);
}

/*Send the request to the server as a client*/
int sendRequest(int serverfd, char *buf, char *method, char *uri, char *version, rio_t *Rio)
{
    char request[MAXLINE];
    strcpy(request, method);

    strcat(request, " ");
    strcat(request, uri);
    strcat(request, " ");
    strcat(request, version);
    strcat(request, "\r\n");
    
    while(strcmp(buf, "\r\n"))
    {//check whether is clear
        bzero(buf, MAXLINE); 
        Rio_readlinebOfMine(Rio, buf, MAXLINE);
        strcat(request, buf);
    }

    Rio_writenOfMine(serverfd, request, sizeof(request));
}

/*open_clientfdSafe, thread safe*/
int open_clientfdSafe(char *hostname, int port)
{    
    int clientfd;
    struct hostent *hp;
    struct sockaddr_in serveraddr;

    if((clientfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)//check errno for cause of error
        return -1;

    bzero((char * )&serveraddr, sizeof(serveraddr));

    serveraddr.sin_family = AF_INET;

    P(&mutexGeneral);

    if((hp = gethostbyname(hostname)) == NULL)
        return -2;

    bcopy((char *)hp->h_addr_list[0], (char *)&serveraddr.sin_addr.s_addr, hp->h_length);

    V(&mutexGeneral);

    serveraddr.sin_port = htons(port);

    if(connect(clientfd, (SA *) &serveraddr, sizeof(serveraddr)) < 0)
        return -1;

    return clientfd;
}     


/*
 * clientCheck - returns an error message to the client
 */
/* $begin clientCheck */
void clientCheck(int fd, char *cause, char *errnum, 
         char *shortmsg, char *longmsg)
{
    char buf[MAXLINE], body[MAXBUF];

    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}

ssize_t Rio_readnOfMine(int fd, void *ptr, size_t n) 
{
    ssize_t nb;
  
    if ((nb = rio_readn(fd, ptr, n)) < 0){
    printf("Rio_readn error");
        return 0 ;
    }
    return nb;
}

void Rio_writenOfMine(int fd, void *usrbuf, size_t n) 
{
    if (rio_writen(fd, usrbuf, n) != n)
       printf("Rio_writenOfMine error");
}

ssize_t Rio_readlinebOfMine(rio_t *rp, void *usrbuf, size_t maxlen) 
{
    ssize_t rc;
    if ((rc = rio_readlineb(rp, usrbuf, maxlen)) < 0){
        printf("Rio_readnlineb error");
        return 0;
    }
    return rc;
}

// void Rio_writenOfMine1(int fd, void *usrbuf, size_t n) 
// {
//     if (Rio_writen(fd, usrbuf, n) != n)
//         return;
// }

void sbuf_init(sbuf_t *sp, int n)
{
     sp->buf = Calloc(n,sizeof(clientFdAddr *));
     sp->n = n;
     sp->front = sp->rear = 0;
     Sem_init(&sp->mutex, 0, 1);
     Sem_init(&sp->slots, 0, n);
     Sem_init(&sp->items, 0, 0);
}

void sbuf_deinit(sbuf_t *sp)
{
     Free(sp->buf);
}

void sbuf_insert(sbuf_t *sp, clientFdAddr *item)
{
     P(&sp->slots);
     P(&sp->mutex);
     sp->buf[(++sp->rear)%(sp->n)] = item;
     V(&sp->mutex);
     V(&sp->items);
}
clientFdAddr *sbuf_remove(sbuf_t *sp)
{
     clientFdAddr *item;
     P(&sp->items);
     P(&sp->mutex);
     item = sp->buf[(++sp->front)%(sp->n)];
     V(&sp->mutex);
     V(&sp->slots);
     return item;
}

