#include "server.h"

Logger logger;

int main(int argc, char *argv[])
{
    // Open the log file
    openLogFile(&logger);

    // pass two arguments: port that would be listened on: -port n -root /path/to/dir
    // and the directory to be served

    // Parse the arguments
    if (argc < 3 || strcmp(argv[1], "-port") != 0) {
        logMessage(&logger, LOG_LEVEL_ERROR, "Usage: %s -port n [-root /path/to/dir]\n", argv[0]);
        exit(1);
    }

    // Get the port number
    int port = atoi(argv[2]);
    if (port <= 0) {
        logMessage(&logger, LOG_LEVEL_ERROR, "Invalid port number\n");
        exit(1);
    }

    // Set the default root directory if not provided
    char *rootWorkDir = "/tmp";
    if (argc >= 5 && strcmp(argv[3], "-root") == 0) {
        rootWorkDir = argv[4];
    }

    logMessage(&logger, LOG_LEVEL_INFO, "port: %d, rootWorkDir: %s\n", port, rootWorkDir);

    // Create a socket
    int sock_listen = socket_create(port);
    if (sock_listen < 0)
    {
        exit(1);
    }

    // Create a thread pool and init it
    ThreadPool pool;
    init_pool(&pool);
    logMessage(&logger, LOG_LEVEL_INFO, "Thread pool initialized\n");

    pthread_t threads[MAX_THREADS];
    for (int i = 0; i < MAX_THREADS; i++)
    {
        pthread_create(&threads[i], NULL, worker, (void *)&pool);
    }
    logMessage(&logger, LOG_LEVEL_INFO, "Thread workers created\n");

    while (1)
    {
        // wait for connection.
        // if connected, create a new thread to handle the request.
        // if not connected, continue to wait for connection.

        // Accept a connection
        int sock_cmd = socket_accept(sock_listen);
        if (sock_cmd < 0)
        {
            logMessage(&logger, LOG_LEVEL_ERROR, "Error accepting connection\n");
            continue; // possibly break; also works.
        }
        logMessage(&logger, LOG_LEVEL_INFO, "Connection accepted\n");

        ftp_session_arg *arg = (ftp_session_arg *)malloc(sizeof(ftp_session_arg));
        arg->sock_cmd = sock_cmd;
        strcpy(arg->rootWorkDir, rootWorkDir);

        submit_task(&pool, ftp_session, (void *)arg);
    }

    // Close the log file
    closeLogFile(&logger);
    return 0;
}

int ftp_login(int sock_cmd, char *password)
{
    char buf[MAXSIZE];
    char *user_given = "anonymous";
    char username[MAXSIZE];

    while (1)
    {
        if ((socket_recv_data(sock_cmd, buf, sizeof(buf))) == -1)
        {
            #ifdef DEBUG
            perror("recv login username data error\n");
            #endif
            logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, recv login username data error\n", sock_cmd);
            exit(1);
        }
        
        //check whether starts with USER
        //if not
        if(strncmp(buf, "USER", 4) != 0)
        {
            if(strncmp(buf, "QUIT", 4) == 0 || strncmp(buf, "ABOR", 4) == 0)
            {
                socket_send_response(sock_cmd, 221, "Goodbye.\r\n");
                logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, quit\n", sock_cmd);
                return 0;
            }
            socket_send_response(sock_cmd, 500, "Invalid command. Example: USER: your_user_name.\r\n");
            logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, Invalid command. Expected USER.\n", sock_cmd);
            continue;
        }

        // now it starts with USER, retrieve the username
        int n = 5;
        int i = 0;
        while(buf[n] != '\n' && buf[n] != '\r')
        {
            username[i++] = buf[n++];
        }
        username[i] = 0;
        #ifdef DEBUG
        printf("username: %s\n", username);
        #endif
        logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, username: %s\n", sock_cmd, username);
        
        //return 331 code to ask for passwd
        socket_send_response(sock_cmd, 331, "Please specify the password.\r\n");

        // We will check whether the username is anonymous after we get the password 
        // taking account of the issues of security.

        // receive password and check whether starts with PASS
        if ((socket_recv_data(sock_cmd, buf, sizeof(buf))) == -1)
        {
            #ifdef DEBUG
            perror("recv login passwd data error\n");
            #endif
            logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, recv login passwd data error\n", sock_cmd);
            exit(1);
        }
        if(strncmp(buf, "PASS", 4) != 0)
        {
            if(strncmp(buf, "QUIT", 4) == 0 || strncmp(buf, "ABOR", 4) == 0)
            {
                socket_send_response(sock_cmd, 221, "Goodbye.\r\n");
                logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, quit\n", sock_cmd);
                return 0;
            }
            socket_send_response(sock_cmd, 500, "Invalid command. Example: PASS your_password. Please resubmit your username and passwd.\r\n");
            logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, Invalid command. Expected PASS.\n", sock_cmd);
            continue;
        }

        //Check whether the username is anonymous firstly
        if(strncmp(username, user_given, strlen(user_given)) != 0)
        {
            socket_send_response(sock_cmd, 530, "Only anonymous login is supported.\r\n");
            logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, Only anonymous login is supported.\n", sock_cmd);
            continue;
        }

        // now it starts with PASS, retrieve the password
        n = 5;
        i = 0;
        while(buf[n] != '\n' && buf[n] != '\r')
        {
            password[i++] = buf[n++];
        }
        password[i] = 0;
        #ifdef DEBUG
        printf("password: %s\n", password);
        #endif
        logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, identity: %s\n", sock_cmd, password);

        //Check success
        socket_send_response(sock_cmd, 230, "Login successful.\r\n");
        logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, Login successful.\n", sock_cmd);
        break;
    }
    return 1;
}

void ftp_session(void *arg)
{
    
    char password[MAXSIZE]; //identity of the user
    ftp_session_arg *session = (ftp_session_arg *)arg;

    // Get the cmd socket descriptor
    int sock_cmd = session->sock_cmd;
    // Get the root directory to be served
    char *rootWorkDir = session->rootWorkDir;
    // Get the thread pool
    ThreadPool *pool = session->pool;

    int dataLinkEstablished = 0; //flag to indicate whether the data link is established
    int passive_mode = 0; //flag to indicate whether the passive mode is on
    int sock_data; //socket descriptor of the data connection
    pthread_mutex_t mutex_data; //mutex for the data connection
    //init mutex
    pthread_mutex_init(&mutex_data, NULL);

    //logMessage with socket descriptor
    logMessage(&logger, LOG_LEVEL_INFO, "ftp session started with socket descriptor %d\n", sock_cmd);

    // Send welcome message
    socket_send_response(sock_cmd, 220, "Welcome to my FTP server.\r\n");

    // authentiate user
    // Now only for anonomous user
    if(ftp_login(sock_cmd, password) == 0)
    {
        close(sock_cmd);
        logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, ftp session ended\n", sock_cmd);
        return;
    }

    // Now we have logged in, we can start to process the commands
    char buf[MAXSIZE];
    char cmd[MAXSIZE] = {0};
    char arg[MAXSIZE] = {0};
    char cwd[MAXSIZE] = {0};
    char path[MAXSIZE] = {0};

    get_absolute_path(cwd, rootWorkDir, "");

    while(1) 
    {
        // Receive the command
        if ((socket_recv_data(sock_cmd, buf, sizeof(buf))) == -1)
        {
            #ifdef DEBUG
            perror("recv cmd data error\n");
            #endif
            logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, recv cmd data error\n", sock_cmd);
            exit(1);
        }
        
        logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, identity: %s, cmd: %s\n", sock_cmd, password, buf);
        
        // Parse the command
        parse_command(sock_cmd, buf, cmd, arg);

        //check whether the command is QUIT
        if(strncmp(cmd, "QUIT", 4) == 0 || strncmp(cmd, "ABOR", 4) == 0)
        {
            socket_send_response(sock_cmd, 221, "Goodbye.\r\n");
            logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, quit\n", sock_cmd);
            break;
        }
        
        // send Process the command to thread pool
        process_command_arg *arg = (process_command_arg *)malloc(sizeof(process_command_arg));
        arg->sock_cmd = sock_cmd;
        arg->dataLinkEstablished = &dataLinkEstablished;
        arg->sock_data = &sock_data;
        arg->mutex_data = &mutex_data;
        arg->passive_mode = &passive_mode;
        strcpy(arg->cmd, cmd);
        strcpy(arg->arg, arg);
        strcpy(arg->cwd, cwd);
        strcpy(arg->rootWorkDir, rootWorkDir);
        submit_task(pool, process_command, (void *)arg);
        
    }

    close(sock_cmd);
    if(dataLinkEstablished)
    {
        pthread_mutex_lock(&mutex_data);
        close(sock_data);
        pthread_mutex_unlock(&mutex_data);
        logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, data connection closed\n", sock_cmd);
    }
    pthread_mutex_destroy(&mutex_data);
    logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, ftp session ended\n", sock_cmd);
    free(arg);
}

int parse_command(int socket_cmd, char *buf, char *cmd, char *arg)
{
    // Parse the command
    int i = 0;
    while (buf[i] != ' ' && buf[i] != 0)
    {
        cmd[i] = buf[i];
        i++;
    }
    cmd[i] = 0;
    i++;
    int j = 0;
    while (buf[i] != '\n' && buf[i] != 0 && buf[i] != '\r')
    {
        arg[j] = buf[i];
        i++;
        j++;
    }
    arg[j] = 0;
    #ifdef DEBUG
    printf("cmd: %s, arg: %s\n", cmd, arg);
    #endif
    logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, cmd: %s, arg: %s\n", socket_cmd, cmd, arg);

}

void process_command(void *args)
{
    //parse the arguments
    process_command_arg *argp = (process_command_arg *)args;
    int sock_cmd = argp->sock_cmd;
    char *cmd = argp->cmd;
    char *arg = argp->arg;
    char *cwd = argp->cwd;
    char *rootWorkDir = argp->rootWorkDir;
    int *dataLinkEstablished = argp->dataLinkEstablished;
    int *sock_data = argp->sock_data;
    int *passive_mode = argp->passive_mode;
    pthread_mutex_t *mutex_data = argp->mutex_data;

    //check the command whether is null
    if(cmd[0] == 0)
    {
        socket_send_response(sock_cmd, 500, "Invalid command.\r\n");
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, Invalid command.\n", sock_cmd);
        return;
    }

    // if cmd is PORT, we need to establish a data connection
    if(strncmp(cmd, "PORT", 4) == 0)
    {
        if(port_process(sock_cmd, sock_data, arg, dataLinkEstablished, mutex_data, passive_mode) != 0)
        {
            logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, PORT command failed.\n", sock_cmd);
            return;
        }
        else 
        {
            logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, PORT command successful.\n", sock_cmd);
            return;
        }
    }
    // else if cmd is PASV, we need to establish a data connection
    else if(strncmp(cmd, "PASV", 4) == 0)
    {
        if(pasv_process(sock_cmd, sock_data, arg, dataLinkEstablished, mutex_data, passive_mode) != 0)
        {
            logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, PASV command failed.\n", sock_cmd);
            return;
        }
        else 
        {
            logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, PASV command successful.\n", sock_cmd);
            return;
        }
    }
    // else if cmd is RETR, we need to send a file
    else if(strncmp(cmd, "RETR", 4) == 0)
    {
        if(retr_process(sock_cmd, sock_data, arg, cwd, rootWorkDir, dataLinkEstablished, mutex_data, passive_mode) != 0)
        {
            logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, RETR command failed.\n", sock_cmd);
            return;
        }
        else 
        {
            logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, RETR command successful.\n", sock_cmd);
            return;
        }
    }
    // else if cmd is STOR, we need to receive a file
    else if(strncmp(cmd, "STOR", 4) == 0)
    {
        if(stor_process(sock_cmd, sock_data, arg, cwd, rootWorkDir, dataLinkEstablished, mutex_data, passive_mode) != 0)
        {
            logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, STOR command failed.\n", sock_cmd);
            return;
        }
        else 
        {
            logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, STOR command successful.\n", sock_cmd);
            return;
        }
    }
    // else if cmd is SYST, we need to send the system information
    else if(strncmp(cmd, "SYST", 4) == 0)
    {
        if(syst_process(sock_cmd) != 0)
        {
            logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, SYST command failed.\n", sock_cmd);
            return;
        }
        else 
        {
            logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, SYST command successful.\n", sock_cmd);
            return;
        }
    }

    




    free(argp);
}
