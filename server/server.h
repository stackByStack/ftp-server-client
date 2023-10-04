#ifndef SERVER_H
#define SERVER_H
#include "utils/utils.h"
#include "utils/argTypes.h"
#include "cmd.h"

/**
 * @brief This function is used to handle the ftp session.
 * 
 * @param arg The argument of the function. It is a pointer to a `ftp_session_arg` struct.
 */
void ftp_session(void * arg);

/**
 * @brief This function is used to handle the ftp session. QUIT may be sent by the client.
 * 
 * @param sock_cmd The socket fd of the command connection.
 * @param password In the mode of anonymous login, this parameter is the identity of the user.
*/
int ftp_login(int sock_cmd, char* password);

/**
 * @brief This function is used to parse the command sent by the client.
 * 
 * @param sock_cmd The socket fd of the command connection.
 * @param buf The buffer used to store the command sent by the client.
 * @param cmd The command sent by the client.
 * @param arg The argument of the command sent by the client.
*/
void parse_command(int sock_cmd, char *buf, char *cmd, char *arg);

/**
 * @brief This function is used to process the command sent by the client.
 * 
 * @param sock_cmd The socket fd of the command connection.
 * @param cmd The command sent by the client.
 * @param arg The argument of the command sent by the client.
 * @param cwd The current working directory of the client.
 * @param rootWorkDir The root working directory of the server.
*/
void process_command(void *args);

#endif