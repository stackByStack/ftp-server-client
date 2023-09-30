#include "server.h"

Logger logger;

int main(int argc, char *argv[])
{
    // Open the log file
    openLogFile(&logger);

    // pass two arguments: port that would be listened on: -port n -root /path/to/dir
    // and the directory to be served

    if (argc != 4 || strcmp(argv[1], "-port") != 0)
    {
        logMessage(&logger, LOG_LEVEL_ERROR, "Usage: %s -port n -root /path/to/dir\n", argv[0]);
        exit(1);
    }

    // Get the port number
    int port = atoi(argv[2]);
    if (port <= 0)
    {
        logMessage(&logger, LOG_LEVEL_ERROR, "Invalid port number\n");
        exit(1);
    }

    // Get the directory after -root to be served
    char *rootWorkDir = argv[4];
    if (rootWorkDir == NULL)
    {
        logMessage(&logger, LOG_LEVEL_ERROR, "Invalid root directory\n");
        exit(1);
    }

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
        if ((recv_data(sock_cmd, buf, sizeof(buf))) == -1)
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
            if(strncmp(buf, "QUIT", 4) == 0)
            {
                send_response(sock_cmd, 221, "Goodbye.\r\n");
                logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, quit\n", sock_cmd);
                return 0;
            }
            send_response(sock_cmd, 500, "Invalid command. Example: USER: your_user_name.\r\n");
            logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, Invalid command. Expected USER.\n", sock_cmd);
            continue;
        }

        // now it starts with USER, retrieve the username
        int n = 5;
        int i = 0;
        while(buf[n] != 0)
        {
            username[i++] = buf[n++];
        }
        username[i] = 0;
        #ifdef DEBUG
        printf("username: %s\n", username);
        #endif
        logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, username: %s\n", sock_cmd, username);
        
        //return 331 code to ask for passwd
        send_response(sock_cmd, 331, "Please specify the password.\r\n");

        // We will check whether the username is anonymous after we get the password 
        // taking account of the issues of security.

        // receive password and check whether starts with PASS
        if ((recv_data(sock_cmd, buf, sizeof(buf))) == -1)
        {
            #ifdef DEBUG
            perror("recv login passwd data error\n");
            #endif
            logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, recv login passwd data error\n", sock_cmd);
            exit(1);
        }
        if(strncmp(buf, "PASS", 4) != 0)
        {
            if(strncmp(buf, "QUIT", 4) == 0)
            {
                send_response(sock_cmd, 221, "Goodbye.\r\n");
                logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, quit\n", sock_cmd);
                return 0;
            }
            send_response(sock_cmd, 500, "Invalid command. Example: PASS your_password. Please resubmit your username and passwd.\r\n");
            logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, Invalid command. Expected PASS.\n", sock_cmd);
            continue;
        }

        //Check whether the username is anonymous firstly
        if(strncmp(username, user_given, strlen(user_given)) != 0)
        {
            send_response(sock_cmd, 530, "Only anonymous login is supported.\r\n");
            logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, Only anonymous login is supported.\n", sock_cmd);
            continue;
        }

        // now it starts with PASS, retrieve the password
        n = 5;
        i = 0;
        while(buf[n] != 0)
        {
            password[i++] = buf[n++];
        }
        password[i] = 0;
        #ifdef DEBUG
        printf("password: %s\n", password);
        #endif
        logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, identity: %s\n", sock_cmd, password);

        //Check success
        send_response(sock_cmd, 230, "Login successful.\r\n");
        logMessage(&logger, LOG_LEVEL_INFO, "sd: %d, Login successful.\n", sock_cmd);
        break;
    }
    return 1;
}

void ftp_session(void *arg)
{
    
    char password[MAXSIZE];
    ftp_session_arg *session = (ftp_session_arg *)arg;

    // Get the cmd socket descriptor
    int sock_cmd = session->sock_cmd;
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

    // free(arg);
}
