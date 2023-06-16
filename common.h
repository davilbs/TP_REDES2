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
char *buildMsg(u_int8_t idMsg, u_int8_t idSender, u_int8_t idReceiver, char *msg)
{
    char buffer[BUFFERSIZE];
    memset(buffer, 0, BUFFERSIZE);

    char header1 = 0, header2 = 0;
    header1 += (idMsg << 4);
    header1 += idSender;
    header2 += (idReceiver << 4);
    buffer[0] = header1;
    buffer[1] = header2;

    strcat(buffer, msg);

    return buffer;
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
int nextPos;

// Adds client to list of clients
void addClnt(int clntSock)
{
    // Check if there is an available spot
    if (nextPos = 15)
        sendMsgTo(clntSock, ERROR, "01");
    printf("User %d added", nextPos);
    usersSocks[nextPos] = clntSock;
    usersConn[nextPos] = 1;

    // Broadcast the join message
    char msg[30];
    memset(msg, 0, 30);
    strcat(msg, "User ");
    sprintf(msg, "%d", nextPos);
    strcat(msg, " joined the group!");
    sendMsgAll(nextPos, MSG, msg);

    // Update next available position
    for (int i = (nextPos + 1); i <= 15; i++)
    {
        if ((i == 15) || (usersConn[i] == 0))
        {
            nextPos = i;
            break;
        }
    }
}

/*
Closes the connection with the client and clears the index
*/
void remClnt(uint8_t idSend)
{
    close(usersSocks[idSend]);
    usersConn[idSend] = 0;
}

void listClnt(const char buffer[])
{
}

void sendMsgAll(uint8_t idSend, const char buffer[])
{
}

void sendMsgAll(uint8_t idSend, uint8_t idRecv, const char buffer[])
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
        char idRecv = (buffer[1] >> 4);
        switch (idMsg)
        {
        case REQ_ADD:
            addClnt(clntSock);
            break;
        case REQ_REM:
            remClnt(idSend);
            break;
        case MSG:
            if (idRecv == 0)
                sendMsgAll(idSend, buffer);
            else
                sendMsgTo(idSend, idRecv, buffer);
            break;
        default:
            printf("Unknown command id issued - %d\n", idMsg);
            break;
        }
    }

    close(clntSock);
}