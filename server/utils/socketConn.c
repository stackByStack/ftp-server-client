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
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, socket create error\n", sockfd);
        return -1;
    }

    memset(&s_addr, 0, sizeof(s_addr));
    s_addr.sin_family = AF_INET;
    s_addr.sin_port = htons(port);
    s_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // set the socket option to be reusable
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &True, sizeof(int)) == -1) {
		close(sockfd);
        #ifdef DEBUG
		perror("setsockopt() error");
        #endif
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, setsockopt() error\n", sockfd);
		return -1; 
	}
    
    if (bind(sockfd, (struct sockaddr *)&s_addr, sizeof(s_addr)) < 0)
    {
        #ifdef DEBUG
        perror("socket bind error");
        #endif
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, socket bind error\n", sockfd);
        return -1;
    }

    // set the length of the queue
    if (listen(sockfd, 10) < 0)
    {
        #ifdef DEBUG
        perror("establish listen error");
        #endif
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, establish listen error\n", sockfd);
        return -1;
    }

    logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, socket created\n", sockfd);
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
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, socket create error\n", sockfd);
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
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, inet_pton error\n", sockfd);
        return -1;
    }
    if (connect(sockfd, (struct sockaddr *)&s_addr, sizeof(s_addr)) < 0)
    {
        #ifdef DEBUG
        perror("connect to a remote host error");
        #endif
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, connect to a remote host error\n", sockfd);
        return -1;
    }
    logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, socket connected\n", sockfd);
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
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, accept error\n", clientfd);
        return -1;
    }
    logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, socket accepted\n", clientfd);
    return clientfd;
}

int socket_recv_data(int sockfd, char *buf, int bufsize)
{
    int total_len = 0;
    int recv_len;

    // Loop until the complete command is received or an error occurs
    while (total_len < MAXSIZE - 1)
    {
        recv_len = recv(sockfd, buf + total_len, bufsize - total_len - 1, 0);

        if (recv_len < 0)
        {
            #ifdef DEBUG
            perror("recv data error");
            #endif
            logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, recv data error\n", sockfd);
            return -1;
        }
        else if (recv_len == 0)
        {
            // Connection closed by the other end
            logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, connection closed by the other end\n", sockfd);
            break;
        }

        total_len += recv_len;
        // logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, received: %s\n", sockfd, buf);

        // Check if the complete command is received
        if (buf[total_len - 1] == '\n' || buf[total_len - 1] == '\r')
        {
            logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, received: %s\n", sockfd, buf);
            break;
        }
        
    }

    buf[total_len] = '\0'; // Null-terminate the received data

    return total_len;
}

int socket_send_data(int sockfd, char *buf, int bufsize)
{
    int len;
    if ((len = send(sockfd, buf, bufsize, 0)) < 0)
    {
        #ifdef DEBUG
        perror("send data error");
        #endif
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, send data error\n", sockfd);
        return -1;
    }
    logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, sent: %s\n", sockfd, buf);
    return len;
}

int socket_close(int sockfd)
{
    if (close(sockfd) < 0)
    {
        #ifdef DEBUG
        perror("close socket error");
        #endif
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, close socket error\n", sockfd);
        return -1;
    }
    logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, socket closed\n", sockfd);
    return 0;
}

int socket_send_response(int sockfd, int rc, char *msg)
{
    char buf[MAXSIZE];
    memset(buf, 0, MAXSIZE);
    sprintf(buf, "%d %s", rc, msg);
    int str_len = strlen(buf);
    if (socket_send_data(sockfd, buf, str_len) < 0)
    {
        #ifdef DEBUG
        perror("send response error");
        #endif
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, send response error\n", sockfd);
        return -1;
    }
    logMessage(&logger, LOG_LEVEL_INFO, "sd: %d,  response sent: %s\n", sockfd, buf);
    return 0;
}

int socket_get_ip(char *ip)
{
    char host[100];
    struct hostent *hostinfo;

    if (gethostname(host, sizeof(host)) < 0)
    {
        #ifdef DEBUG
        perror("Error getting host name");
        #endif
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, Error getting host name\n", -1);
        return -1;
    }

    if ((hostinfo = gethostbyname(host)) == NULL)
    {
        #ifdef DEBUG
        perror("Error getting host by name");
        #endif
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, Error getting host by name\n", -1);
        return -1;
    }

    struct in_addr **addr_list = (struct in_addr **)hostinfo->h_addr_list;

    if (addr_list[0] == NULL)
    {
        #ifdef DEBUG
        fprintf(stderr, "No IP address found for the host\n");
        #endif
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, No IP address found for the host\n", -1);
        return -1;
    }

    strcpy(ip, inet_ntoa(*addr_list[0]));
    logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, IP address: %s\n", -1, ip);

    return 0;
}

void close_data_conn(int *sock_data, int *dataLinkEstablished, pthread_mutex_t *mutex)
{
    pthread_mutex_lock(mutex);
    if (*dataLinkEstablished == 1)
    {
        // Data link established, close the connection
        int shut = shutdown(*sock_data, SHUT_RDWR);
        logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, shutdown: %d\n", *sock_data, shut);
        close(*sock_data);
        *sock_data = -1;
        *dataLinkEstablished = 0;
        logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, data connection closed\n", *sock_data);
    }
    pthread_mutex_unlock(mutex);
}