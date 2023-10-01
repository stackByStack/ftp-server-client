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
    int shut = shutdown(*sock_data, SHUT_RDWR); // Shutdown the previous connection
    logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, shutdown: %d\n", *sock_data, shut);
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

int pasv_process(int sock_cmd, int *sock_data, int *dataLinkEstablished, pthread_mutex_t *mutex, int *passive_mode, ThreadPool *pool)
{
    srand(time(NULL));
    int port_num = rand() % 45535 + 20000;
    char ip[30];
    socket_get_ip(ip);

    pthread_mutex_lock(mutex);
    int shut = shutdown(*sock_data, SHUT_RDWR); // Shutdown the previous connection
    logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, shutdown: %d\n", *sock_data, shut);
    close(*sock_data); // Close the previous connection

    *sock_data = -1; // Reset sock_data to indicate the connection is closed

    int new_sock_data = -1;
    int retries = 0;
    const int max_retries = 20;

    // Retry with different ports
    while (retries < max_retries)
    {
        new_sock_data = socket_create(port_num);
        if (new_sock_data >= 0)
        {
            break; // Port is available, break out of the loop
        }
        if (errno == EADDRINUSE)
        {
            // Port is already in use, try the next port
            port_num++;
            retries++;
        }
        else
        {
            // Other error occurred, handle it accordingly
            logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, Error: cannot listen to the client. errno: %d\n", sock_cmd, errno);
            *dataLinkEstablished = 0;
            // Delay showing the error message to the client until RETR or STOR command
            pthread_mutex_unlock(mutex);
            return -1;
        }
    }

    if (retries >= max_retries)
    {
        // Failed to find an available port after maximum retries
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, Error: maximum retries exceeded. Cannot listen to the client.\n", sock_cmd);
        *dataLinkEstablished = 0;
        // Delay showing the error message to the client until RETR or STOR command
        pthread_mutex_unlock(mutex);
        return -1;
    }

    *sock_data = new_sock_data; // Update sock_data with the new connection
    *dataLinkEstablished = 0; //Still listening for client to connect
    pthread_mutex_unlock(mutex);

    // ip format 127,0,0,1
    char ip_formatted[30];
    strcpy(ip_formatted, ip);
    for (int i = 0; i < strlen(ip_formatted); i++)
    {
        if (ip_formatted[i] == '.')
            ip_formatted[i] = ',';
    }

    // prepare the message to be sent to client
    char msg[100];
    sprintf(msg, "Entering Passive Mode (%s,%d,%d)\n", ip_formatted, port_num / 256, port_num % 256);

    // send the response of accepting the message to client
    socket_send_response(sock_cmd, 227, msg);

    //submit the task of listening to the client to the thread pool
    listen_pasv_arg *arg = (listen_pasv_arg *)malloc(sizeof(listen_pasv_arg));
    arg->sock_data = sock_data;
    arg->dataLinkEstablished = dataLinkEstablished;
    arg->passive_mode = passive_mode;
    arg->pool = pool;
    arg->mutex = mutex;
    submit_task(pool, listen_pasv, arg);
}

void listen_pasv(void *args)
{
    listen_pasv_arg *arg = (listen_pasv_arg *)args;
    int *sock_data = arg->sock_data;
    int *dataLinkEstablished = arg->dataLinkEstablished;
    int *passive_mode = arg->passive_mode;
    ThreadPool *pool = arg->pool;
    pthread_mutex_t *mutex = arg->mutex;

    pthread_mutex_lock(mutex);
    if(*dataLinkEstablished == 1)
    {
        pthread_mutex_unlock(mutex);
        return;
    }

    int new_sock_data = socket_accept(sock_data);
    if (new_sock_data < 0)
    {
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, listen_pasv Error: cannot accept the client.\n", sock_data);
    }

    pthread_mutex_lock(mutex);
    if(*dataLinkEstablished == 1)
    {
        // Data link already established, close the new connection
        close(new_sock_data);
    }
    else
    {
        // Data link not established, update sock_data with the new connection
        *dataLinkEstablished = 1;
        *sock_data = new_sock_data;
    }
    pthread_mutex_unlock(mutex);
    
}
