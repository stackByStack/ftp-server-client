#ifndef CMD_H
#define CMD_H

#include "utils/utils.h"
#include "utils/argTypes.h"

extern Logger logger;

/**
 * @brief Process the PORT command
 * 
 * @param sock_cmd Socket for the command channel
 * @param sock_data Socket for the data channel
 * @param arg Argument of the command
 * @param dataLinkEstablished Flag to check if the data link is established
 * @param mutex Mutex to lock the data link established flag
 * @param passive_mode Flag to check if the server is in passive mode
 * @return int Returns 0 if the command was successful, -1 otherwise
*/
int port_process(int sock_cmd, int *sock_data, char* arg, int *dataLinkEstablished, pthread_mutex_t *mutex, int *passive_mode);

/**
 * @brief Process the PASV command
 * 
 * @param sock_cmd Socket for the command channel
 * @param sock_data Socket for the data channel
 * @param dataLinkEstablished Flag to check if the data link is established
 * @param mutex Mutex to lock the data link established flag
 * @param passive_mode Flag to check if the server is in passive mode
 * @param pool Thread pool
 * @return int Returns 0 if the command was successful, -1 otherwise
*/
int pasv_process(int sock_cmd, int *sock_data, int *dataLinkEstablished, pthread_mutex_t *mutex, int *passive_mode, ThreadPool *pool);

/**
 * @brief listen connection of the passive mode
 * 
 * @param sock_data Socket for the data channel
 * @param dataLinkEstablished Flag to check if the data link is established
 * @param mutex Mutex to lock the data link established flag
 * @param passive_mode Flag to check if the server is in passive mode
 * @param pool Thread pool
*/
void listen_pasv(void *args);

/**
 * @brief Process the RETR command
 *
 * @param sock_cmd Socket for the command channel
 * @param sock_data Socket for the data channel
 * @param arg Argument of the command
 * @param cwd The current working directory of the client
 * @param rootWorkDir The root working directory of the server
 * @param dataLinkEstablished Flag to check if the data link is established
 * @param mutex Mutex to lock the data link established flag
 * @param passive_mode Flag to check if the server is in passive mode
 * @return int Returns 0 if the command was successful, -1 otherwise 
*/
int retr_process(int sock_cmd, int *sock_data, char *arg, char *cwd, char *rootWorkDir, int *dataLinkEstablished, pthread_mutex_t *mutex, int *passive_mode);

#endif