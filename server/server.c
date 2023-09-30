#include "server.h"

int main(int argc, char *argv[])
{
    // pass two arguments: port that would be listened on: -port n -root /path/to/dir
    // and the directory to be served

    if (argc != 4 || strcmp(argv[1], "-port") != 0)
    {
        printf("Usage: %s -port n -root /path/to/dir\n", argv[0]);
        exit(1);
    }

    // Get the port number
    int port = atoi(argv[2]);
    if (port <= 0)
    {
        printf("Invalid port number\n");
        exit(1);
    }

    // Get the directory after -root to be served
    char *rootWorkDir = argv[4];
    if (rootWorkDir == NULL)
    {
        printf("Invalid root directory\n");
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

    pthread_t threads[MAX_THREADS];
    for (int i = 0; i < MAX_THREADS; i++) {
        pthread_create(&threads[i], NULL, worker, (void*)&pool);
    }

    while (1)
    {
        //wait for connection. 
        //if connected, create a new thread to handle the request.
        //if not connected, continue to wait for connection.

        // Accept a connection
        int sock_cmd = socket_accept(sock_listen);
        if(sock_cmd < 0)
        {
            continue; // possibly break; also works.
        }
        ftp_session_arg *arg = (ftp_session_arg *)malloc(sizeof(ftp_session_arg));
        arg->sock_cmd = sock_cmd;

        submit_task(&pool, ftp_session, (void *)arg);
    }
}

void ftp_session(void * arg)
{
    ftp_session_arg *session = (ftp_session_arg *)arg;
    
    // Get the cmd socket descriptor
    int sock_cmd = session->sock_cmd;

    // Send welcome message
    socket_send_response(sock_cmd, 220, "Welcome to my FTP server.\r\n");

    //authentiate user
    //Now only for anonomous user


    // free(arg);
}

