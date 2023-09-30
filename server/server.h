#include "utils/utils.h"
#include "utils/argTypes.h"

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