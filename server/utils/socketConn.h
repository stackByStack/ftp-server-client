#ifndef SOCKETCONN_H
#define SOCKETCONN_H

#include <arpa/inet.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>		// getaddrinfo()
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include "logger.h"

#define DEBUG 0
#define MAXSIZE 4096 // max buffer size

extern Logger logger;

/**
 * @brief Create a socket object, bind and listen
 * 
 * @param port 
 * @return socket fd on success, -1 on error
 */
int socket_create(int port);

/**
 * @brief Connect to a host on a certain port number
 * 
 * @param port 
 * @param host 
 * @return socket fd on success, -1 on error
 */
int socket_connect(char *host, int port);

/**
 * @brief Accept a connection on a socket
 * 
 * @param listenfd 
 * @return socket fd on success, -1 on error
 */
int socket_accept(int listenfd);

/**
 * @brief recv data from the connected host
 * 
 * @param sockfd
 * @param buf 
 * @param bufsize 
 * @return -1 on error, bytes received on success
 */
int socket_recv_data(int sockfd, char *buf, int bufsize);

/**
 * @brief send data to the connected host
 * 
 * @param sockfd 
 * @param buf 
 * @param bufsize 
 * @return -1 on error, bytes sent on success
 */
int socket_send_data(int sockfd, char *buf, int bufsize);


/**
 * @brief send response to the connected host
 * 
 * @param sockfd 
 * @param rc 
 * @param msg 
 * @return -1 on error, bytes sent on success
 */
int socket_send_response(int sockfd, int rc, char *msg);

/**
 * @brief Close socket connection
 * 
 * @param sockfd 
 * @return -1 on error, 0 on success
 */
int socket_close(int sockfd);

/**
 * @brief Get the IP address of the host
 * 
 * @param ip 
 * @return -1 on error, 0 on success
 */
int socket_get_ip(char *ip);

/**
 * @brief close data connection
 * 
 * @param sock_data data socket
 * @param dataLinkEstablished flag to indicate if data connection is established
 * @param mutex mutex to lock dataLinkEstablished
*/
void close_data_conn(int *sock_data, int *dataLinkEstablished, pthread_mutex_t *mutex);

#endif