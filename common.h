#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

#define BUFFERSIZE 500
#define REQ_ADD 1
#define REQ_REM 2
#define RES_LIST 4
#define MSG 6
#define ERROR 7
#define OK 8

/*
Message format
[0000] [0000] [0000] [0000] [498*8]
IdMsg  IdSnd  IdRcv  Hdend  Message
[0001] [0001] [0000] [0000] [any]
  ||     ||     ||
[1000] [1111] [1111]
IdMsg from [0001] to [1000] allows for 8 message types
IdSnd from [0001] to [1111] allows 15 users
IdRcv [0000] means no receiver is assigned, i.e. message to all
Hdend marks end of header
Message body can have up to 498 bytes.
*/
void buildHeader(u_int8_t idMsg, u_int8_t idSender, u_int8_t idReceiver, char *msg)
{
    char *buffer;
    buffer = (char *)calloc(strlen(msg) + 2, sizeof(char));

    // Write header for message
    char header1 = 0;
    header1 += (idMsg << 4);
    header1 += idSender;
    buffer[0] = header1;
    buffer[1] = (idReceiver << 4);

    strcat(buffer, msg);
    msg = buffer;
}

void DieSysError(const char *msg)
{
    perror(msg);
    exit(1);
}

void DieUsrError(const char *msg, const char *detail)
{
    fputs(msg, stderr);
    fputs(": ", stderr);
    fputs(detail, stderr);
    fputc('\n', stderr);
    exit(1);
}

// Server auxiliary functions
// Checks the IP version for the connection
int checkVersion(const char *address)
{
    int pos = 0;
    while (address[pos] != '\0')
    {
        if (address[pos] == '.')
            return 2;
        if (address[pos] == ':')
            return 10;
        pos++;
    }
    return 0;
}

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