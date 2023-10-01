#include "cmd.h"

int port_process(int sock_cmd, int *sock_data, char *arg, int *dataLinkEstablished, pthread_mutex_t *mutex, int *passive_mode)
{
    int port[6];
    int i = 0;
    char *token = strtok(arg, ",");

    // Check if expected number of tokens are present
    if (token == NULL)
    {
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, Invalid input. Expected format: h1,h2,h3,h4,p1,p2\n", sock_cmd);
        // send error message to client with error code
        char *msg = "Invalid input. Expected format: h1,h2,h3,h4,p1,p2\n";
        socket_send_response(sock_cmd, 501, msg);
        return 1;
    }

    while (token != NULL)
    {
        // Check if index exceeds array bounds
        if (i >= 6)
        {
            logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, Too many values provided. Expected format: h1,h2,h3,h4,p1,p2\n", sock_cmd);
            // send error message to client with error code
            char *msg = "Too many values provided. Expected format: h1,h2,h3,h4,p1,p2\n";
            socket_send_response(sock_cmd, 501, msg);
            return 1;
        }

        // Convert token to integer
        int value = atoi(token);

        // Check if conversion was successful
        if (value == 0 && *token != '0')
        {
            logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, Invalid value: %s\n", sock_cmd, token);
            // send error message to client with error code
            char msg[100];
            sprintf(msg, "Invalid value: %s\n", token);
            socket_send_response(sock_cmd, 501, msg);
            return 1;
        }

        port[i++] = value;
        token = strtok(NULL, ",");
    }

    // Check if the expected number of tokens were provided
    if (i != 6)
    {
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, Insufficient values provided. Expected format: h1,h2,h3,h4,p1,p2\n", sock_cmd);
        // send error message to client with error code
        char *msg = "Insufficient values provided. Expected format: h1,h2,h3,h4,p1,p2\n";
        socket_send_response(sock_cmd, 501, msg);
        return 1;
    }

    int port_num = port[4] * 256 + port[5];
    char ip[30];
    sprintf(ip, "%d.%d.%d.%d", port[0], port[1], port[2], port[3]);
    logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, ip: %s, port: %d\n", sock_cmd, ip, port_num);

    // send the response of accepting the message to client
    char msg[100];
    sprintf(msg, "PORT command successful. Connecting to %s:%d\n", ip, port_num);
    socket_send_response(sock_cmd, 200, msg);

    pthread_mutex_lock(mutex);
    close(*sock_data); // Close the previous connection

    *sock_data = -1; // Reset sock_data to indicate the connection is closed

    int new_sock_data = socket_connect(ip, port_num);
    if (new_sock_data < 0)
    {
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, Error: cannot connect to the client.\n", sock_cmd);
        *dataLinkEstablished = 0;
        // delay showing error message to client until RETR or STOR command
        return -1;
    }

    *sock_data = new_sock_data; // Update sock_data with the new connection
    *dataLinkEstablished = 1;
    pthread_mutex_unlock(mutex);
    return 0;
}