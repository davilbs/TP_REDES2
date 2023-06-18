#include "common.h"

#define MAXPENDING 15

int usersSocks[15];
int usersConn[15];
uint8_t nextPos;

// Initialize server variables
void initServer()
{
    memset(usersSocks, 0, 15);
    memset(usersConn, 0, 15);
    nextPos = 0;
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
int sendMsgTo(uint8_t idSend, uint8_t idRecv, uint8_t idMsg, const char buffer[])
{
    char *msg;
    msg = (char *)calloc(2 + strlen(buffer), sizeof(char));
    uint8_t header1 = 0, header2 = 0;
    header1 += (idMsg << 4);
    header1 += idSend;
    header2 += (idRecv << 4);
}

// Iterate through all users connected and send message to all
void sendMsgAll(uint8_t idSend, uint8_t idMsg, const char buffer[])
{
    for (int i = 0; i < 15; i++)
    {
        if (usersConn[i] == 1)
            sendMsgTo(idSend, i, MSG, buffer);
    }
}

// Adds client to list of clients
void addClnt(int clntSock)
{
    // Check if it's full
    if (nextPos = 15)
        return sendError(clntSock, "01");

    printf("User %d added", nextPos + 1);
    usersSocks[nextPos] = clntSock;
    usersConn[nextPos] = 1;

    // Broadcast the join message
    char msg[30];
    memset(msg, 0, 30);
    strcat(msg, "User ");
    sprintf(msg, "%d", nextPos + 1);
    strcat(msg, " joined the group!");
    sendMsgAll(nextPos + 1, MSG, msg);

    // Update next available position
    for (; nextPos < 15; ++nextPos)
    {
        if (usersConn[nextPos] == 0)
            break;
    }
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

void handleClient(int clntSock)
{
    char buffer[BUFFERSIZE];
    ssize_t numBytesRcvd = recv(clntSock, buffer, BUFFERSIZE, 0);
    if (numBytesRcvd < 0)
        DieSysError("Failed to receive message");
    for (;;)
    {
        uint8_t idMsg = (buffer[0] >> 4);
        uint8_t idSend = (buffer[0] & (u_int8_t)15);
        uint8_t idRecv = (buffer[1] >> 4);
        printf("IdSend: %d\nIdMsg: %d\nIdRecv: %d\n", idSend, idMsg, idRecv);

        // switch (idMsg)
        // {
        // case REQ_ADD:
        //     addClnt(clntSock);
        //     break;
        // case REQ_REM:
        //     if(remClnt(idSend) < 0)
        //         sendError(clntSock, "02");
        //     break;
        // case MSG:
        //     if (idRecv == 0)
        //         sendMsgAll(idSend, MSG, buffer);
        //     else
        //     {
        //         if (sendMsgTo(idSend, idRecv, MSG, buffer) < 0)
        //             sendError(clntSock, "03");
        //     }
        //     break;
        // default:
        //     printf("Unknown command id issued - %d\n", idMsg);
        //     break;
        // }
    }

    close(clntSock);
}

int main(int argc, char *argv[]) {
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
            handleClient(clntSock);
        else
            puts("Unable to get client address");
    }

}