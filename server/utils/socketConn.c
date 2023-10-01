#include "socketConn.h"

int socket_create(int port)
{
    int sockfd;
    struct sockaddr_in s_addr;
    int True = 1;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        #ifdef DEBUG
        perror("socket create error");
        #endif
        return -1;
    }

    memset(&s_addr, 0, sizeof(s_addr));
    s_addr.sin_family = AF_INET;
    s_addr.sin_port = htons(port);
    s_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // set the socket option to be reusable
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &True, sizeof(int)) == -1) {
		close(sockfd);
		perror("setsockopt() error");
		return -1; 
	}
    
    if (bind(sockfd, (struct sockaddr *)&s_addr, sizeof(s_addr)) < 0)
    {
        #ifdef DEBUG
        perror("socket bind error");
        #endif
        return -1;
    }

    // set the length of the queue
    if (listen(sockfd, 10) < 0)
    {
        #ifdef DEBUG
        perror("establish listen error");
        #endif
        return -1;
    }
    return sockfd;
}

int socket_connect(char *host, int port)
{
    int sockfd;
    struct sockaddr_in s_addr;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        #ifdef DEBUG
        perror("socket create error");
        #endif
        return -1;
    }
    memset(&s_addr, 0, sizeof(s_addr));
    s_addr.sin_family = AF_INET;
    s_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, host, &s_addr.sin_addr) <= 0)
    {
        #ifdef DEBUG
        perror("inet_pton error");
        #endif
        return -1;
    }
    if (connect(sockfd, (struct sockaddr *)&s_addr, sizeof(s_addr)) < 0)
    {
        #ifdef DEBUG
        perror("connect to a remote host error");
        #endif
        return -1;
    }
    return sockfd;
}

int socket_accept(int listenfd)
{
    int clientfd;
    struct sockaddr_in c_addr;
    socklen_t c_len = sizeof(c_addr);
    if ((clientfd = accept(listenfd, (struct sockaddr *)&c_addr, &c_len)) < 0)
    {
        #ifdef DEBUG
        perror("accept error");
        #endif
        return -1;
    }
    return clientfd;
}

int socket_recv_data(int sockfd, char *buf, int bufsize)
{
    int len;
    if ((len = recv(sockfd, buf, bufsize, 0)) < 0)
    {
        #ifdef DEBUG
        perror("recv data error");
        #endif
        return -1;
    }
    return len;
}

int socket_send_data(int sockfd, char *buf, int bufsize)
{
    int len;
    if ((len = send(sockfd, buf, bufsize, 0)) < 0)
    {
        #ifdef DEBUG
        perror("send data error");
        #endif
        return -1;
    }
    return len;
}

int socket_close(int sockfd)
{
    if (close(sockfd) < 0)
    {
        #ifdef DEBUG
        perror("close socket error");
        #endif
        return -1;
    }
    return 0;
}

int socket_send_response(int sockfd, int rc, char *msg)
{
    char buf[MAXSIZE];
    memset(buf, 0, MAXSIZE);
    sprintf(buf, "%d %s", rc, msg);
    if (socket_send_data(sockfd, buf, MAXSIZE) < 0)
    {
        #ifdef DEBUG
        perror("send response error");
        #endif
        return -1;
    }
    return 0;
}