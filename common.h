#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>

#define BUFFERSIZE 500
#define REQ_ADD 1
#define REQ_REM 2
#define RES_LIST 4
#define MSG 6
#define ERROR 7
#define OK 8

typedef struct message_data {
    u_int8_t idMsg;
    u_int8_t idSend;
    u_int8_t idRecv;
    char *body;
} message_data;

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
    // Write header for message
    char header1 = 0;
    header1 += (idMsg << 4);
    header1 += idSender;
    msg[0] = header1;
    msg[1] = (idReceiver << 4);    
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