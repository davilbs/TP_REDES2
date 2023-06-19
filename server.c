#include "common.h"

#define MAXPENDING 15
#define NUM_THREADS 15

typedef struct user_data
{
    int usersSocks[NUM_THREADS];
    int usersConn[NUM_THREADS];
    pthread_mutex_t writing;
    pthread_cond_t doneWrite;
    uint8_t nextPos;
} user_data;

// Initialize server variables
void initServer(user_data *ud)
{
    memset(ud->usersSocks, 0, NUM_THREADS);
    memset(ud->usersConn, 0, NUM_THREADS);
    pthread_mutex_init(&ud->writing, NULL);
    pthread_cond_init(&ud->doneWrite, NULL);
    ud->nextPos = 0;
}

// Send error message to client socket
void sendError(int clntSock, const char errCode[])
{
    size_t strLen = strlen(errCode);
    ssize_t numBytes = send(clntSock, errCode, strLen, 0);

    if (numBytes < 0)
        DieSysError("Failed to send exit command");
    else if (numBytes != strLen)
        DieUsrError("Failed to send", "Sent unexpected number of bytes");
}

// Send message from idSend to idRecv, with code idMsg and content buffer
int sendMsgTo(message_data msg, user_data *ud)
{
    char *buffer;
    buffer = (char *)calloc(3 + strlen(msg.body), sizeof(char));

    uint8_t header1 = 0;
    header1 += (msg.idMsg << 4);
    header1 += msg.idSend;
    buffer[0] = header1;
    buffer[1] = (msg.idRecv << 4);
    strcat(buffer, buffer);

    size_t strLen = strlen(buffer);
    ssize_t bytesSent = send(ud->usersSocks[msg.idRecv], buffer, strLen, 0);
}

// Iterate through all users connected and send message to all
void sendMsgAll(message_data msg, user_data *ud)
{
    for (int i = 0; i < NUM_THREADS; i++)
    {
        if (ud->usersConn[i] == 1)
            sendMsgTo(msg, ud);
    }
}

// Adds client to list of clients
int addClnt(int clntSock, user_data *ud)
{
    // Check if it's full
    pthread_mutex_lock(&ud->writing);
    if (ud->nextPos == NUM_THREADS)
        return -1;
        
    printf("User %d added\n", (ud->nextPos + 1));
    ud->usersSocks[ud->nextPos] = clntSock;
    ud->usersConn[ud->nextPos] = 1;

    char msg[30];
    memset(msg, 0, 30);
    strcat(msg, "User ");
    sprintf(msg, "%d", ud->nextPos + 1);
    strcat(msg, " joined the group!");

    // Update next available position
    for (; ud->nextPos < NUM_THREADS; ++ud->nextPos)
    {
        if (ud->usersConn[ud->nextPos] == 0)
            break;
    }
    pthread_mutex_unlock(&ud->writing);

    // Broadcast the join message
    sendMsgAll(nextPos + 1, MSG, msg);

    return 0;
}

/*
Closes the connection with the client and clears the index
*/
int remClnt(uint8_t idSend)
{
    // Check if user id is valid for removal
    if ((idSend < 0) || (idSend > 14) || (usersConn[idSend] == 0))
        return -1;

    // Close connection and reset entry value
    close(usersSocks[idSend]);
    usersConn[idSend] = 0;

    return 0;
}

void listClnt(const char buffer[])
{
}

void *handleClient(void *clntSockptr)
{
    char buffer[BUFFERSIZE];
    int clntSock = *(int *)clntSockptr;
    ssize_t numBytesRcvd = recv(clntSock, buffer, BUFFERSIZE, 0);
    if (numBytesRcvd < 0)
        DieSysError("Failed to receive message");
    uint8_t idMsg = 0, idSend, idRecv;
    for (;;)
    {
        idMsg = (buffer[0] >> 4);
        idSend = (buffer[0] & (u_int8_t)NUM_THREADS);
        idRecv = (buffer[1] >> 4);
        switch (idMsg)
        {
        case REQ_ADD:
            if (addClnt(clntSock) < 0)
            {
                close(clntSock);
                return 0;
            }
        case REQ_REM:
            if (remClnt(idSend) < 0)
                sendError(clntSock, "02");
            break;
        case MSG:
            if (idRecv == 0)
                sendMsgAll(idSend, MSG, buffer);
            else
            {
                if (sendMsgTo(idSend, idRecv, MSG, buffer) < 0)
                    sendError(clntSock, "03");
            }
            break;
        default:
            break;
        }
        idMsg = 0;
        idSend = 0;
        idRecv = 0;
        memset(buffer, 0, BUFFERSIZE);
    }

    close(clntSock);
}

int main(int argc, char *argv[])
{
    // Parse arguments
    if (argc != 3)
        DieUsrError("Invalid number of arguments", "Ipv Port");

    char *version = argv[1];
    in_port_t port = atoi(argv[2]);

    // Check IP version
    int servSock;
    int af, proto, addrany;
    if (strcmp(version, "v4") == 0)
    {
        af = AF_INET;
        proto = IPPROTO_IP;
    }
    else if (strcmp(version, "v6") == 0)
    {
        af = AF_INET6;
        proto = IPPROTO_IPV6;
    }
    else
        DieUsrError("Invalid IP version", "use v4 or v6");

    // Open network socket
    servSock = socket(af, SOCK_STREAM, proto);
    if (servSock < 0)
        DieSysError("Failed to open socket");

    struct sockaddr_in servAddr;
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = af;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(port);

    if (bind(servSock, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0)
        DieSysError("Failed to bind to socket");

    if (listen(servSock, MAXPENDING) < 0)
        DieSysError("Failed to listen on port");

    initServer();

    pthread_t threads[NUM_THREADS];
    int rc;
    // Main server loop
    for (;;)
    {
        struct sockaddr_in clntAddr;
        socklen_t clntAddrLen = sizeof(clntAddr);

        int clntSock = accept(servSock, (struct sockaddr *)&clntAddr, &clntAddrLen);
        if (clntSock < 0)
            DieSysError("Failed to accept connection");

        char clntName[INET_ADDRSTRLEN];
        if (inet_ntop(af, &clntAddr.sin_addr.s_addr, clntName, sizeof(clntName)) != NULL)
        {
            if (nextPos == NUM_THREADS)
                sendError(clntSock, "01");
            else
            {
                rc = pthread_create(&threads[nextPos], NULL, handleClient, (void *)&clntSock);
                if (rc)
                    printf("Failed to create thread\n");
            }
        }
        else
            puts("Unable to get client address");
    }

    pthread_exit(NULL);
}