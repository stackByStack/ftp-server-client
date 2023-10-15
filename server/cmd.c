#include "cmd.h"

int is_directory(const char *path) {
    struct stat path_stat;
    if (stat(path, &path_stat) == 0) {
        return S_ISDIR(path_stat.st_mode);
    }
    return 0;  // Return 0 if stat fails or path is not a directory
}

int port_process(int sock_cmd, int *sock_data, char *arg, int *dataLinkEstablished, pthread_mutex_t *mutex)
{
    int port[6];
    int i = 0;
    char *token = strtok(arg, ",");

    // Check if expected number of tokens are present
    if (token == NULL)
    {
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, Invalid input. Expected format: h1,h2,h3,h4,p1,p2\n", sock_cmd);
        // send error message to client with error code
        char *msg = "Invalid input. Expected format: h1,h2,h3,h4,p1,p2\r\n";
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
            char *msg = "Too many values provided. Expected format: h1,h2,h3,h4,p1,p2\r\n";
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
            sprintf(msg, "Invalid value: %s\r\n", token);
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
        char *msg = "Insufficient values provided. Expected format: h1,h2,h3,h4,p1,p2\r\n";
        socket_send_response(sock_cmd, 501, msg);
        return 1;
    }

    int port_num = port[4] * 256 + port[5];
    char ip[30];
    sprintf(ip, "%d.%d.%d.%d", port[0], port[1], port[2], port[3]);
    logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, ip: %s, port: %d\n", sock_cmd, ip, port_num);

    // send the response of accepting the message to client
    char msg[100];
    sprintf(msg, "PORT command successful. Connecting to %s:%d\r\n", ip, port_num);
    socket_send_response(sock_cmd, 200, msg);
    logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, PORT command successful. Connecting to %s:%d\n", sock_cmd, ip, port_num);

    pthread_mutex_lock(mutex);
    // int shut = shutdown(*sock_data, SHUT_RDWR); // Shutdown the previous connection
    // logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, shutdown: %d\n", *sock_data, shut);
    // close(*sock_data); // Close the previous connection

    // *sock_data = -1; // Reset sock_data to indicate the connection is closed

    int new_sock_data = socket_connect(ip, port_num);
    if (new_sock_data < 0)
    {
        char msg1[100] = {0};
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, Error: cannot connect to the client.\n", sock_cmd);
        sprintf(msg1, "Error: cannot connect to the client.\r\n");
        socket_send_response(sock_cmd, 425, msg1);
        // *dataLinkEstablished = 0;
        // delay showing error message to client until RETR or STOR command
        return -1;
    }

    *sock_data = new_sock_data; // Update sock_data with the new connection
    *dataLinkEstablished += 1;
    logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, Data link established under PORT mode.\n", *sock_data);
    pthread_mutex_unlock(mutex);
    
    return 0;
}

int pasv_process(int sock_cmd, int *sock_data, int *dataLinkEstablished, pthread_mutex_t *mutex, int *passive_mode, ThreadPool *pool)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    // Use milliseconds as the seed for srand
    unsigned int seed = (unsigned int)(tv.tv_usec);
    srand(seed);
    int port_num = (rand()) % 45535 + 20000;
    char ip[30];
    socket_get_ip(ip);

    pthread_mutex_lock(mutex);
    // int shut = shutdown(*sock_data, SHUT_RDWR); // Shutdown the previous connection
    // logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, shutdown: %d\n", *sock_data, shut);
    // close(*sock_data); // Close the previous connection

    // *sock_data = -1; // Reset sock_data to indicate the connection is closed

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
            // *dataLinkEstablished = 0;
            // Delay showing the error message to the client until RETR or STOR command
            pthread_mutex_unlock(mutex);
            return -1;
        }
    }

    if (retries >= max_retries)
    {
        // Failed to find an available port after maximum retries
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, Error: maximum retries exceeded. Cannot listen to the client.\n", sock_cmd);
        char msg1[100] = {0};
        sprintf(msg1, "Error: maximum retries exceeded. Cannot listen to the client.\r\n");
        socket_send_response(sock_cmd, 425, msg1);
        // *dataLinkEstablished = 0;
        // Delay showing the error message to the client until RETR or STOR command
        pthread_mutex_unlock(mutex);
        return -1;
    }

    *sock_data = new_sock_data; // Update sock_data with the new connection
    // *dataLinkEstablished = 0;   // Still listening for client to connect
    pthread_mutex_unlock(mutex);

    // ip format 127,0,0,1
    char ip_formatted[30];
    strcpy(ip_formatted, ip);
    int len = strlen(ip_formatted);
    for (int i = 0; i < len; i++)
    {
        if (ip_formatted[i] == '.')
            ip_formatted[i] = ',';
    }

    // prepare the message to be sent to client
    char msg[100];
    sprintf(msg, "Entering Passive Mode (%s,%d,%d)\r\n", ip_formatted, port_num / 256, port_num % 256);

    // send the response of accepting the message to client
    socket_send_response(sock_cmd, 227, msg);
    logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, Entering Passive Mode (%s,%d,%d)\n", sock_cmd, ip_formatted, port_num / 256, port_num % 256);

    // submit the task of listening to the client to the thread pool
    listen_pasv_arg *arg = (listen_pasv_arg *)malloc(sizeof(listen_pasv_arg));
    arg->sock_data = sock_data;
    arg->dataLinkEstablished = dataLinkEstablished;
    arg->passive_mode = passive_mode;
    // arg->pool = pool;
    arg->mutex = mutex;
    submit_task(pool, listen_pasv, arg);
    logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, Submitted task to listen to client\n", sock_cmd);
    return 0;
}

void listen_pasv(void *args)
{
    listen_pasv_arg *arg = (listen_pasv_arg *)args;
    int *sock_data = arg->sock_data;
    int *dataLinkEstablished = arg->dataLinkEstablished;
    // int *passive_mode = arg->passive_mode;
    // ThreadPool *pool = arg->pool;
    pthread_mutex_t *mutex = arg->mutex;

    // pthread_mutex_lock(mutex);
    // if (*dataLinkEstablished == 1)
    // {
    //     pthread_mutex_unlock(mutex);
    //     logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, listen_pasv: Data link already established. Not listening for new connections.\n", *sock_data);
    //     return;
    // }
    // pthread_mutex_unlock(mutex);

    int new_sock_data = socket_accept(*sock_data);
    if (new_sock_data < 0)
    {
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, listen_pasv Error: cannot accept the client.\n", sock_data);
    }

    pthread_mutex_lock(mutex);
    // if (*dataLinkEstablished == 1)
    // {
    //     // Data link already established, close the new connection
    //     close(new_sock_data);
    //     logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, listen_pasv: Though new link established, Data link already established during the creating . Not listening for new connections.\n", *sock_data);
    // }
    // else
    // {
    //     // Data link not established, update sock_data with the new connection
    //     *dataLinkEstablished = 1;
    //     *sock_data = new_sock_data;
    //     logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, listen_pasv: Data link established.\n", *sock_data);
    // }
    *dataLinkEstablished += 1;
    *sock_data = new_sock_data;
    logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, listen_pasv: Data link established.\n", *sock_data);
    pthread_mutex_unlock(mutex);
}

int retr_process(int sock_cmd, int sock_data, char *arg, char *cwd, char *rootWorkDir, int *dataLinkEstablished, pthread_mutex_t *mutex)
{
    pthread_mutex_lock(mutex);
    if (*dataLinkEstablished == 0)
    {
        // Data link not established, show error message to client
        char *msg = "Data link not established. Please use PORT or PASV command first.\r\n";
        socket_send_response(sock_cmd, 425, msg);
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, Data link not established. Please use PORT or PASV command first.\n", sock_cmd);
        pthread_mutex_unlock(mutex);
        return 1;
    }
    pthread_mutex_unlock(mutex);

    // Data link established, show success message to client
    char msg[100];
    sprintf(msg, "RETR command successful. Downloading %s\r\n", arg);
    socket_send_response(sock_cmd, 150, msg);

    // Get the absolute path of the file to be downloaded
    char path[MAXSIZE];
    get_absolute_path(path, cwd, rootWorkDir, arg);

    // Check if the file exists
    if (access(path, F_OK) == -1)
    {
        // File does not exist, show error message to client
        char msg[MAXSIZE * 2];
        sprintf(msg, "File %s does not exist.\r\n", path);
        socket_send_response(sock_cmd, 550, msg);
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, File %s does not exist.\n", sock_cmd, arg);
        // close the data connection
        close_data_conn(sock_data, dataLinkEstablished, mutex);
        return 1;
    }

    if (isUpperDirectory(path, rootWorkDir) == 1)
    {
        //access control
        char msg[MAXSIZE * 2];
        sprintf(msg, "File %s is not in the current working directory.\r\n", path);
        socket_send_response(sock_cmd, 550, msg);
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, File %s is not in the current working directory.\n", sock_cmd, arg);
        // close the data connection
        close_data_conn(sock_data, dataLinkEstablished, mutex);
        return 1;
    }

    // Check if the file is a directory
    if (is_directory(path))
    {
        // File is a directory, show error message to client
        char msg[100];
        sprintf(msg, "%s is a directory.\r\n", arg);
        socket_send_response(sock_cmd, 550, msg);
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, %s is a directory.\n", sock_cmd, arg);
        // close the data connection
        close_data_conn(sock_data, dataLinkEstablished, mutex);
        return 1;
    }

    // Check if the file is readable
    if (access(path, R_OK) == -1)
    {
        // File is not readable, show error message to client
        char msg[100];
        sprintf(msg, "File %s is not readable.\r\n", arg);
        socket_send_response(sock_cmd, 550, msg);
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, File %s is not readable.\n", sock_cmd, arg);
        // close the data connection
        close_data_conn(sock_data, dataLinkEstablished, mutex);
        return 1;
    }

    // Open the file
    FILE *fp = fopen(path, "rb");
    if (fp == NULL)
    {
        // Error occurred while opening the file, show error message to client
        char msg[100];
        sprintf(msg, "Error occurred while opening the file %s.\r\n", arg);
        socket_send_response(sock_cmd, 551, msg);
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, Error occurred while opening the file %s.\n", sock_cmd, arg);
        return 1;
    }

    // The file can be large, thus it should be sent in multiple batches
    // If the connection is broken in between, we need to send reject code 426 and tell how many bytes is already sent.
    // If we successfully send the entire file, we need to send success code 226.
    // If we encounter any other error when reading from file, we need to send reject code 551.

    // Read and send the file in batches
    char buffer[MAXSIZE];
    size_t bytes_read;
    int total_bytes_sent = 0;

    while ((bytes_read = fread(buffer, 1, MAXSIZE, fp)) > 0)
    {
        if (socket_send_data(sock_data, buffer, bytes_read) < 0)
        {
            // Error occurred while sending data, show error message to client
            char msg[100];
            sprintf(msg, "Error occurred while sending the file %s.\r\n", arg);
            socket_send_response(sock_cmd, 426, msg);
            logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, Error occurred while sending the file %s.\n", sock_cmd, arg);
            fclose(fp);
            // Close the data connection
            close_data_conn(sock_data, dataLinkEstablished, mutex);
            return 1;
        }
        total_bytes_sent += bytes_read;
        logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, bytes_sent: %d/%d\n", sock_cmd, bytes_read, total_bytes_sent);
    }

    // Close the file
    fclose(fp);

    // Send success message to client
    char success_msg[100];
    sprintf(success_msg, "File %s sent successfully. Total bytes sent: %d\r\n", arg, total_bytes_sent);
    socket_send_response(sock_cmd, 226, success_msg);
    logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, File %s sent successfully. Total bytes sent: %d\n", sock_cmd, arg, total_bytes_sent);
    // Close the data connection
    close_data_conn(sock_data, dataLinkEstablished, mutex);

    return 0;
}

int stor_process(int sock_cmd, int sock_data, char *arg, char *cwd, char *rootWorkDir, int *dataLinkEstablished, pthread_mutex_t *mutex)
{
    pthread_mutex_lock(mutex);
    if (*dataLinkEstablished == 0)
    {
        // Data link not established, show error message to client
        char *msg = "Data link not established. Please use PORT or PASV command first.\r\n";
        socket_send_response(sock_cmd, 425, msg);
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, Data link not established. Please use PORT or PASV command first.\n", sock_cmd);
        pthread_mutex_unlock(mutex);
        return 1;
    }
    pthread_mutex_unlock(mutex);

    // Data link established, show success message to client
    char msg[100];
    sprintf(msg, "STOR command successful. Uploading %s\r\n", arg);
    socket_send_response(sock_cmd, 150, msg);

    // Get the absolute path of the file to be uploaded
    char path[MAXSIZE];
    get_absolute_path(path, cwd, rootWorkDir, arg);

    // Check if the file exists
    if (access(path, F_OK) != -1)
    {
        // File already exists, show error message to client
        char msg[100];
        sprintf(msg, "File %s already exists.\r\n", arg);
        socket_send_response(sock_cmd, 550, msg);
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, File %s already exists.\n", sock_cmd, arg);
        // close the data connection
        close_data_conn(sock_data, dataLinkEstablished, mutex);
        return 1;
    }

    if (isUpperDirectory(path, rootWorkDir) == 1)
    {
        //access control
        char msg[MAXSIZE * 2];
        sprintf(msg, "File %s is not in the current working directory.\r\n", path);
        socket_send_response(sock_cmd, 550, msg);
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, File %s is not in the current working directory.\n", sock_cmd, arg);
        // close the data connection
        close_data_conn(sock_data, dataLinkEstablished, mutex);
        return 1;
    }

    // Check if the file is a directory
    if (is_directory(path))
    {
        // File is a directory, show error message to client
        char msg[100];
        sprintf(msg, "%s is a directory.\r\n", arg);
        socket_send_response(sock_cmd, 550, msg);
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, %s is a directory.\n", sock_cmd, arg);
        // close the data connection
        close_data_conn(sock_data, dataLinkEstablished, mutex);
        return 1;
    }

    // Check if the file is writable
    // if (access(path, W_OK) == -1)
    // {
    //     // File is not writable, show error message to client
    //     char msg[100];
    //     sprintf(msg, "File %s is not writable.\r\n", arg);
    //     socket_send_response(sock_cmd, 550, msg);
    //     logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, File %s is not writable.\n", sock_cmd, arg);
    //     // Close the data connection
    //     close_data_conn(sock_data, dataLinkEstablished, mutex);
    //     return 1;
    // }

    // Open the file for writing
    FILE *file = fopen(path, "wb");
    if (file == NULL)
    {
        // Error opening the file, show error message to client
        char msg[100];
        sprintf(msg, "Failed to open file %s for writing.\r\n", arg);
        socket_send_response(sock_cmd, 550, msg);
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, Failed to open file %s for writing.\n", sock_cmd, arg);
        // Close the data connection
        close_data_conn(sock_data, dataLinkEstablished, mutex);
        return 1;
    }

    // Receive and write the data to the file
    char buffer[MAXSIZE];
    ssize_t bytesRead;
    ssize_t bytesWritten = 0;
    while ((bytesRead = recv(sock_data, buffer, sizeof(buffer), 0)) > 0)
    {
        ssize_t itemsWritten = fwrite(buffer, 1, bytesRead, file);
        if (itemsWritten != bytesRead)
        {
            // Error writing data, show error message to client
            char msg[100];
            sprintf(msg, "Error writing data for file %s.\r\n", arg);
            socket_send_response(sock_cmd, 550, msg);
            logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, Error writing data for file %s.\n", sock_cmd, arg);
            // Close the file and data connection
            fclose(file);
            close_data_conn(sock_data, dataLinkEstablished, mutex);
            return 1;
        }
        bytesWritten += bytesRead;
        logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, bytes_written: %d\n", sock_cmd, bytesWritten);
    }

    // Check for errors during the data transfer
    if (bytesRead < 0)
    {
        // Error receiving data, show error message to client
        char msg[100];
        sprintf(msg, "Error receiving data for file %s.\r\n", arg);
        socket_send_response(sock_cmd, 426, msg);
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, Error receiving data for file %s.\n", sock_cmd, arg);
        // Close the file and data connection
        fclose(file);
        close_data_conn(sock_data, dataLinkEstablished, mutex);
        return 1;
    }

    // Close the file
    fclose(file);

    // File upload successful, show success message to client
    char successMsg[100];
    sprintf(successMsg, "File %s uploaded successfully.\r\n", arg);
    socket_send_response(sock_cmd, 226, successMsg);
    logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, File %s uploaded successfully.\n", sock_cmd, arg);

    // Close the data connection
    close_data_conn(sock_data, dataLinkEstablished, mutex);

    return 0;
}

int syst_process(int sock_cmd)
{
    // send the response of accepting the message to client
    char *msg = "UNIX Type: L8\r\n";
    socket_send_response(sock_cmd, 215, msg);
    logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, SYST command successful.\n", sock_cmd);
    return 0;
}

int type_process(int sock_cmd, char *arg, int *transfer_type)
{
    if (strcmp(arg, "I") == 0)
    {
        // send the response of accepting the message to client
        char *msg = "Type set to I.\r\n";
        socket_send_response(sock_cmd, 200, msg);
        logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, Type set to I.\n", sock_cmd);
        *transfer_type = 1;
        return 0;
    }
    else if (strcmp(arg, "A") == 0)
    {
        // send the response of accepting the message to client
        char *msg = "Type set to A.\r\n";
        socket_send_response(sock_cmd, 200, msg);
        logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, Type set to A.\n", sock_cmd);
        *transfer_type = 0;
        return 0;
    }
    else
    {
        // send error message to client with error code
        char msg[100];
        sprintf(msg, "Invalid type: %s. Expected type: I or A\r\n", arg);
        socket_send_response(sock_cmd, 501, msg);
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, Invalid type: %s. Expected type: I or A\n", sock_cmd, arg);
        return 1;
    }
}

int pwd_process(int sock_cmd, char *cwd)
{
    // send the response of accepting the message to client
    char msg[MAXSIZE];
    sprintf(msg, "\"%s\"\r\n", cwd);
    socket_send_response(sock_cmd, 257, msg);
    logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, PWD command successful. Current working directory: %s\n", sock_cmd, cwd);
    return 0;
}

int cwd_process(int sock_cmd, char *arg, char *cwd, char *rootWorkDir)
{
    // Get the absolute path of the directory to be changed to
    char path[MAXSIZE];
    get_absolute_path(path, cwd, rootWorkDir, arg);

    // Check if the directory exists
    if (access(path, F_OK) == -1)
    {
        // Directory does not exist, show error message to client
        char msg[100];
        sprintf(msg, "Directory %s does not exist.\r\n", arg);
        socket_send_response(sock_cmd, 550, msg);
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, Directory %s does not exist.\n", sock_cmd, arg);
        return 1;
    }

    if (isUpperDirectory(path, rootWorkDir) == 1)
    {
        //access control
        char msg[MAXSIZE * 2];
        sprintf(msg, "Directory %s is not in the current working directory.\r\n", path);
        socket_send_response(sock_cmd, 550, msg);
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, Directory %s is not in the current working directory.\n", sock_cmd, arg);
        return 1;
    }

    // Check if the directory is a file
    if (!is_directory(path))
    {
        // Directory is a file, show error message to client
        char msg[100];
        sprintf(msg, "%s is a file.\r\n", arg);
        socket_send_response(sock_cmd, 550, msg);
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, %s is a file.\n", sock_cmd, arg);
        return 1;
    }

    // Check if the directory is readable
    if (access(path, R_OK) == -1)
    {
        // Directory is not readable, show error message to client
        char msg[100];
        sprintf(msg, "Directory %s is not readable.\r\n", arg);
        socket_send_response(sock_cmd, 550, msg);
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, Directory %s is not readable.\n", sock_cmd, arg);
        return 1;
    }

    // Change the current working directory by getting the part after rootworkDir
    // strcpy(cwd, path);
    strcpy(cwd, path + strlen(rootWorkDir));

    // send the response of accepting the message to client
    char successMsg[100];
    sprintf(successMsg, "Directory changed to %s\r\n", cwd);
    socket_send_response(sock_cmd, 250, successMsg);
    logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, Directory changed to %s\n", sock_cmd, cwd);
    return 0;
}

int list_process(int sock_cmd, int sock_data, char *arg, char *cwd, char *rootWorkDir, int *dataLinkEstablished, pthread_mutex_t *mutex)
{
    logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, cwd: %s, rootWorkDir: %s, arg: %s\n", sock_cmd, cwd, rootWorkDir, arg);
    pthread_mutex_lock(mutex);
    if (*dataLinkEstablished == 0)
    {
        // Data link not established, show error message to client
        char *msg = "Data link not established. Please use PORT or PASV command first.\r\n";
        socket_send_response(sock_cmd, 425, msg);
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, Data link not established. Please use PORT or PASV command first.\n", sock_cmd);
        pthread_mutex_unlock(mutex);
        return 1;
    }
    pthread_mutex_unlock(mutex);

    // Data link established, show success message to client
    char msg[100];
    sprintf(msg, "LIST command successful. Listing %s\r\n", arg);
    socket_send_response(sock_cmd, 150, msg);

    // Get the absolute path of the directory to be listed
    char path[MAXSIZE];
    get_absolute_path(path, cwd, rootWorkDir, arg);

    // Check if the directory exists
    if (access(path, F_OK) == -1)
    {
        // Directory does not exist, show error message to client
        char msg[100];
        sprintf(msg, "Directory %s does not exist.\r\n", arg);
        socket_send_response(sock_cmd, 550, msg);
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, Directory %s does not exist.\n", sock_cmd, arg);
        // close the data connection
        close_data_conn(sock_data, dataLinkEstablished, mutex);
        return 1;
    }

    if (isUpperDirectory(path, rootWorkDir) == 1)
    {
        //access control
        char msg[MAXSIZE * 2];
        sprintf(msg, "Directory %s is not in the current working directory.\r\n", path);
        socket_send_response(sock_cmd, 550, msg);
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, Directory %s is not in the current working directory.\n", sock_cmd, arg);
        // close the data connection
        close_data_conn(sock_data, dataLinkEstablished, mutex);
        return 1;
    }

    struct stat pathStat;
    if (stat(path, &pathStat) == 0) {
        if (S_ISDIR(pathStat.st_mode)) {
            listDirectory(path, sock_data, sock_cmd);
        } else {
            printFileDetails(path, pathStat, sock_data, sock_cmd);
        }
    } else {
        char msg[MAXSIZE * 2];
        sprintf(msg, "Failed to retrieve information for %s.\r\n", path);
        socket_send_response(sock_cmd, 550, msg);
    }

    // Close the data connection
    close_data_conn(sock_data, dataLinkEstablished, mutex);

    return 0;
}

int mkd_process(int sock_cmd, char *arg, char *cwd, char *rootWorkDir)
{
    // Get the absolute path of the directory to be created
    char path[MAXSIZE];
    get_absolute_path(path, cwd, rootWorkDir, arg);

    // Check if the directory already exists
    if (access(path, F_OK) != -1)
    {
        // Directory already exists, show error message to client
        char msg[100];
        sprintf(msg, "Directory %s already exists.\r\n", arg);
        socket_send_response(sock_cmd, 550, msg);
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, Directory %s already exists.\n", sock_cmd, arg);
        return 1;
    }

    if (isUpperDirectory(path, rootWorkDir) == 1)
    {
        //access control
        char msg[MAXSIZE * 2];
        sprintf(msg, "Directory %s is not in the current working directory.\r\n", path);
        socket_send_response(sock_cmd, 550, msg);
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, Directory %s is not in the current working directory.\n", sock_cmd, arg);
        return 1;
    }

    // Check if the parent directory is writable
    char parent_path[MAXSIZE];
    get_absolute_path(parent_path, cwd, rootWorkDir, "");
    if (access(parent_path, W_OK) == -1)
    {
        // Parent directory is not writable, show error message to client
        char msg[MAXSIZE * 2];
        sprintf(msg, "Parent directory %s is not writable.\r\n", parent_path);
        socket_send_response(sock_cmd, 550, msg);
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, Parent directory %s is not writable.\n", sock_cmd, parent_path);
        return 1;
    }

    // Create the directory
    if (mkdir(path, 0777) == -1)
    {
        // Failed to create directory, show error message to client
        char msg[100];
        sprintf(msg, "Failed to create directory %s.\r\n", arg);
        socket_send_response(sock_cmd, 550, msg);
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, Failed to create directory %s.\n", sock_cmd, arg);
        return 1;
    }

    // Show success message to client
    char successMsg[100];
    sprintf(successMsg, "Directory %s created successfully.\r\n", arg);
    socket_send_response(sock_cmd, 257, successMsg);
    logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, Directory %s created successfully.\n", sock_cmd, arg);
    return 0;
}

int rmd_process(int sock_cmd, char *arg, char *cwd, char *rootWorkDir)
{
    // Get the absolute path of the directory to be removed
    char path[MAXSIZE];
    get_absolute_path(path, cwd, rootWorkDir, arg);

    // Check if the directory exists
    if (access(path, F_OK) == -1)
    {
        // Directory doesn't exist, show error message to client
        char msg[100];
        sprintf(msg, "Directory %s does not exist.\r\n", arg);
        socket_send_response(sock_cmd, 550, msg);
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, Directory %s does not exist.\n", sock_cmd, arg);
        return 1;
    }

    if(isUpperDirectory(path, rootWorkDir) == 1) {
        //access control
        char msg[MAXSIZE * 2];
        sprintf(msg, "Directory %s is not in the current working directory.\r\n", path);
        socket_send_response(sock_cmd, 550, msg);
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, Directory %s is not in the current working directory.\n", sock_cmd, arg);
        return 1;
    }

    // Check if the directory is writable
    if (access(path, W_OK) == -1)
    {
        // Directory is not writable, show error message to client
        char msg[100];
        sprintf(msg, "Directory %s is not writable.\r\n", arg);
        socket_send_response(sock_cmd, 550, msg);
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, Directory %s is not writable.\n", sock_cmd, arg);
        return 1;
    }

    // Remove the directory
    if (rmdir(path) == -1)
    {
        // Failed to remove directory, show error message to client
        char msg[100];
        sprintf(msg, "Failed to remove directory %s.\r\n", arg);
        socket_send_response(sock_cmd, 550, msg);
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, Failed to remove directory %s.\n", sock_cmd, arg);
        return 1;
    }

    // Show success message to client
    char successMsg[100];
    sprintf(successMsg, "Directory %s removed successfully.\r\n", arg);
    socket_send_response(sock_cmd, 226, successMsg);
    logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, Directory %s removed successfully.\n", sock_cmd, arg);
    return 0;
}

int rnfr_process(int sock_cmd, char *arg, char *cwd, char *rootWorkDir, char *oldname)
{
    // Get the absolute path of the file/directory to be renamed
    char old_path[MAXSIZE];
    get_absolute_path(old_path, cwd, rootWorkDir, arg);

    // Check if the file/directory exists
    if (access(old_path, F_OK) == -1)
    {
        // File/directory doesn't exist, show error message to client
        char msg[100];
        sprintf(msg, "File/directory %s does not exist.\r\n", arg);
        socket_send_response(sock_cmd, 550, msg);
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, File/directory %s does not exist.\n", sock_cmd, arg);
        return 1;
    }

    if (isUpperDirectory(old_path, rootWorkDir) == 1)
    {
        //access control
        char msg[MAXSIZE * 2];
        sprintf(msg, "File/directory %s is not in the current working directory.\r\n", old_path);
        socket_send_response(sock_cmd, 550, msg);
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, File/directory %s is not in the current working directory.\n", sock_cmd, arg);
        return 1;
    }

    // Save the old name to the corresponding address
    strcpy(oldname, old_path);

    // Send success message to client
    socket_send_response(sock_cmd, 350, "RNFR command successful.\r\n");
    logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, RNFR command successful.\n", sock_cmd);

    return 0;
}

int rnto_process(int sock_cmd, char *arg, char *cwd, char *rootWorkDir, char *oldName, int *rnfr_flag)
{
    // Check if the RNFR command was previously executed
    if (*rnfr_flag == 0)
    {
        // RNFR command was not executed, show error message to client
        socket_send_response(sock_cmd, 503, "RNFR must be executed before RNTO.\r\n");
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, RNFR must be executed before RNTO.\n", sock_cmd);
        return 1;
    }

    // Get the absolute path of the new file/directory name
    char new_path[MAXSIZE];
    get_absolute_path(new_path, cwd, rootWorkDir, arg);

    // Check if the new name conflicts with an existing file/directory
    if (access(new_path, F_OK) != -1)
    {
        // New name conflicts with an existing file/directory, show error message to client
        char msg[100];
        sprintf(msg, "File/directory %s already exists.\r\n", arg);
        socket_send_response(sock_cmd, 550, msg);
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, File/directory %s already exists.\n", sock_cmd, arg);
        return 1;
    }

    if (isUpperDirectory(new_path, rootWorkDir) == 1)
    {
        //access control
        char msg[MAXSIZE * 2];
        sprintf(msg, "File/directory %s is not in the current working directory.\r\n", new_path);
        socket_send_response(sock_cmd, 550, msg);
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, File/directory %s is not in the current working directory.\n", sock_cmd, arg);
        // Reset the RNFR flag
        *rnfr_flag = 0;
        return 1;
    }

    // Perform the renaming operation
    if (rename(oldName, new_path) == -1)
    {
        // Failed to rename, show error message to client
        char msg[100];
        sprintf(msg, "Failed to rename file/directory.\r\n");
        socket_send_response(sock_cmd, 550, msg);
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, Failed to rename file/directory %s %s.\n", sock_cmd, oldName, new_path);
        return 1;
    }

    // Send success message to client
    socket_send_response(sock_cmd, 250, "RNTO command successful.\r\n");
    logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, RNTO command successful. File/directory renamed.\n", sock_cmd);

    // Reset the RNFR flag
    *rnfr_flag = 0;

    return 0;
}
