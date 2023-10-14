#include "server.h"

Logger logger;

int main(int argc, char *argv[])
{
    // Open the log file
    openLogFile(&logger);

    // pass two arguments: port that would be listened on: -port n -root /path/to/dir
    // and the directory to be served

    // Parse the arguments
    const char *defaultRoot = "/tmp";
    int defaultPort = 21;

    const char *rootWorkDir = defaultRoot;
    int port = defaultPort;

    for (int i = 1; i < argc; i += 2) {
        if (strcmp(argv[i], "-root") == 0) {
            if (i + 1 < argc) {
                size_t rootLen = strlen(argv[i + 1]);
                rootWorkDir = (char *) malloc(rootLen + 1);  // Allocate memory for root
                strcpy((char *) rootWorkDir, argv[i + 1]);    // Copy value using strcpy
            } else {
                logMessage(&logger, LOG_LEVEL_ERROR, "Error: -root requires a value.\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-port") == 0) {
            if (i + 1 < argc) {
                port = atoi(argv[i + 1]);
            } else {
                logMessage(&logger, LOG_LEVEL_ERROR, "Error: -port requires a value.\n");
                return 1;
            }
        }
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
        arg->pool = &pool;
        strcpy(arg->rootWorkDir, rootWorkDir);

        submit_task(&pool, ftp_session, (void *)arg);
        logMessage(&logger, LOG_LEVEL_INFO, "Task submitted\n");
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
    int transfer_type = 1; //flag to indicate the transfer type
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
    char args[MAXSIZE] = {0};
    char cwd[MAXSIZE] = "/";
    // char path[MAXSIZE] = {0};
    char rnfr_old_path[MAXSIZE] = {0};
    int rnfr_flag = 0;

    // get_absolute_path(cwd, rootWorkDir, "");

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
        parse_command(sock_cmd, buf, cmd, args);

        //check whether the command is QUIT
        if(strncmp(cmd, "QUIT", 4) == 0 || strncmp(cmd, "ABOR", 4) == 0)
        {
            socket_send_response(sock_cmd, 221, "Goodbye.\r\n");
            logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, quit\n", sock_cmd);
            break;
        }
        
        // send Process the command to thread pool
        process_command_arg *arg1 = (process_command_arg *)malloc(sizeof(process_command_arg));
        arg1->sock_cmd = sock_cmd;
        arg1->dataLinkEstablished = &dataLinkEstablished;
        arg1->sock_data = &sock_data;
        arg1->mutex_data = &mutex_data;
        arg1->passive_mode = &passive_mode;
        arg1->transfer_type = &transfer_type;
        arg1->rnfr_flag = &rnfr_flag;
        arg1->rnfr_old_path = rnfr_old_path;
        arg1->pool = pool;
        arg1->cwd = cwd;
        arg1->rootWorkDir = rootWorkDir;
        strcpy(arg1->cmd, cmd);
        strcpy(arg1->arg, args);
        // strcpy(arg1->cwd, cwd);
        // strcpy(arg1->rootWorkDir, rootWorkDir);

        logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, Task %s submitted\n", sock_cmd, cmd);
        submit_task(pool, process_command, (void *)arg1);
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

void parse_command(int socket_cmd, char *buf, char *cmd, char *arg)
{
    // Parse the command
    int i = 0;
    while (buf[i] != ' ' && buf[i] != 0 && buf[i] != '\n' && buf[i] != '\r')
    {
        cmd[i] = buf[i];
        i++;
    }
    cmd[i] = 0;
    i++;
    int j = 0;
    int str_len = strlen(buf);
    while (i < str_len && buf[i] != '\n' && buf[i] != 0 && buf[i] != '\r')
    {
        arg[j] = buf[i];
        i++;
        j++;
    }
    arg[j] = 0;
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
    int *transfer_type = argp->transfer_type;
    char *rnfr_old_path = argp->rnfr_old_path;
    int *rnfr_flag = argp->rnfr_flag;
    ThreadPool *pool = argp->pool;
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
        if(port_process(sock_cmd, sock_data, arg, dataLinkEstablished, mutex_data) != 0)
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
        if(pasv_process(sock_cmd, sock_data, dataLinkEstablished, mutex_data, passive_mode, pool) != 0)
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
        if(retr_process(sock_cmd, *sock_data, arg, cwd, rootWorkDir, dataLinkEstablished, mutex_data) != 0)
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
        if(stor_process(sock_cmd, *sock_data, arg, cwd, rootWorkDir, dataLinkEstablished, mutex_data) != 0)
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
    // else if cmd is TYPE, we need to set the transfer type
    else if(strncmp(cmd, "TYPE", 4) == 0)
    {
        if(type_process(sock_cmd, arg, transfer_type) != 0)
        {
            logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, TYPE command failed.\n", sock_cmd);
            return;
        }
        else 
        {
            logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, TYPE command successful.\n", sock_cmd);
            return;
        }
    }
    // else if cmd is PWD, we need to send the current working directory
    else if(strncmp(cmd, "PWD", 3) == 0)
    {
        if(pwd_process(sock_cmd, cwd) != 0)
        {
            logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, PWD command failed.\n", sock_cmd);
            return;
        }
        else 
        {
            logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, PWD command successful.\n", sock_cmd);
            return;
        }
    }
    // else if cmd is CWD, we need to change the current working directory
    else if(strncmp(cmd, "CWD", 3) == 0)
    {
        if(cwd_process(sock_cmd, arg, cwd, rootWorkDir) != 0)
        {
            logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, CWD command failed.\n", sock_cmd);
            return;
        }
        else 
        {
            logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, CWD command successful.\n", sock_cmd);
            return;
        }
    }
    // else if cmd is LIST, we need to list the files in the current working directory
    else if(strncmp(cmd, "LIST", 4) == 0)
    {
        if(list_process(sock_cmd, *sock_data, arg, cwd, rootWorkDir, dataLinkEstablished, mutex_data) != 0)
        {
            logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, LIST command failed.\n", sock_cmd);
            return;
        }
        else 
        {
            logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, LIST command successful.\n", sock_cmd);
            return;
        }
    }
    // else if cmd is MKD, we need to create a directory
    else if(strncmp(cmd, "MKD", 3) == 0)
    {
        if(mkd_process(sock_cmd, arg, cwd, rootWorkDir) != 0)
        {
            logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, MKD command failed.\n", sock_cmd);
            return;
        }
        else 
        {
            logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, MKD command successful.\n", sock_cmd);
            return;
        }
    }
    // else if cmd is RMD, we need to remove a directory
    else if(strncmp(cmd, "RMD", 3) == 0)
    {
        if(rmd_process(sock_cmd, arg, cwd, rootWorkDir) != 0)
        {
            logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, RMD command failed.\n", sock_cmd);
            return;
        }
        else 
        {
            logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, RMD command successful.\n", sock_cmd);
            return;
        }
    }
    // else if cmd is RNFR, we need to rename a file
    else if(strncmp(cmd, "RNFR", 4) == 0)
    {
        if(rnfr_process(sock_cmd, arg, cwd, rootWorkDir, rnfr_old_path) != 0)
        {
            logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, RNFR command failed.\n", sock_cmd);
            return;
        }
        else 
        {
            *rnfr_flag = 1;
            logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, RNFR command successful.\n", sock_cmd);
            return;
        }
    }
    // else if cmd is RNTO, we need to rename a file
    else if(strncmp(cmd, "RNTO", 4) == 0)
    {
        if(rnto_process(sock_cmd, arg, cwd, rootWorkDir, rnfr_old_path, rnfr_flag) != 0)
        {
            logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, RNTO command failed.\n", sock_cmd);
            return;
        }
        else 
        {
            *rnfr_flag = 0;
            rnfr_old_path[0] = 0;
            logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, RNTO command successful.\n", sock_cmd);
            return;
        }
    }
    else
    {
        socket_send_response(sock_cmd, 500, "Invalid command.\r\n");
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, Invalid command.\n", sock_cmd);
    }


    if(*rnfr_flag == 1 && strncmp(cmd, "RNFR", 4) != 0)
    {
        *rnfr_flag = 0;
        rnfr_old_path[0] = 0;
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, RNFR command failed as the next cmd is not RNTO.\n", sock_cmd);
    }

    free(argp);
}
