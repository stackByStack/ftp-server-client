#ifndef CMD_H
#define CMD_H

#include "utils/utils.h"

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

#endif