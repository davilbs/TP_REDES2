#include "common.h"
#define NUM_THREADS 2

// Joins the chat server. Checks for empty spot.
void join(int sock)
{
    // Try to join chat
    uint8_t idMsg = 0, idSend = 0, idRecv = 0;
    char msg[BUFFERSIZE];
    memset(msg, 0, BUFFERSIZE);
    idMsg = 1;
    buildHeader(idMsg, idSend, idRecv, msg);
    size_t strLen = strlen(msg);
    ssize_t numBytes = send(sock, msg, strLen, 0);

    char buffer[BUFFERSIZE];
    memset(buffer, 0, BUFFERSIZE);
    ssize_t numBytesRcvd = recv(sock, buffer, BUFFERSIZE, 0);
    if (numBytesRcvd < 0)
        DieSysError("Failed to receive message");

    if (strcmp(buffer, "01") == 0)
    {
        printf("User limit exceeded");
        exit(0);
    }
    else
        printf("%s\n", buffer);
}

/*
 Reads commands from command prompt
 - close connection 
 - list users
 - send to IdUser "Message"
 - send all "Message"
*/
void *readCommands(void *sockptr)
{
    char *input = NULL, msg[BUFFERSIZE];
    memset(msg, 0, BUFFERSIZE);
    int sock = *(int *)sockptr;
    size_t inputSize;
    for (;;)
    {
        memset(input, 0, strlen(input));
        if (getline(&input, &inputSize, stdin) < 0)
            exit(1);

        if (strcmp(input, "close connection") == 0)
        {
            sendExit(sock);
            exit(0);
        }
        else if (strcmp(input, "list users") == 0)
        {
            buildHeader(4, 0, 0, msg);
            sendList(sock);
        }
        else if (memcmp(input, "send to", strlen("send to")) == 0)
        {
            sendMsgTo();
        }
        else if (memcmp(input, "send all", strlen("send all")) == 0)
        {
            sendMsgAll();
        }
    }
}

void *parseMessages(void *sockptr)
{
    int sock = *(int *)sockptr;
    char buffer[BUFFERSIZE];
    for (;;)
    {
        memset(buffer, 0, BUFFERSIZE);
        ssize_t numBytesRcvd = recv(sock, buffer, BUFFERSIZE, 0);
        if (numBytesRcvd < 0)
            DieSysError("Failed to receive message");
        else if (numBytesRcvd > 0)
            printf("%s\n", buffer);
    }
}

int main(int argc, char *argv[])
{
    // Parse arguments
    if (argc != 3)
        DieUsrError("Invalid number of arguments!", "IPv address port");

    char *address = argv[1];
    int ipv = checkVersion(address);
    int proto = (ipv == 2) ? 0 : 41;
    if (ipv == 0)
        DieUsrError("Invalid address format", "Use IPv4 or IPv6 format");

    // Stablish connection
    int sock = socket(ipv, SOCK_STREAM, proto);
    if (sock < 0)
        DieSysError("failed to open socket");

    struct sockaddr_in servAddr;
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = ipv;

    int rtnVal = inet_pton(ipv, address, &servAddr.sin_addr.s_addr);
    if (rtnVal == 0)
        DieUsrError("Ip string to address failed", "Invalid address string");
    else if (rtnVal < 0)
        DieSysError("Ip string to address failed");

    in_port_t port = atoi(argv[2]);
    servAddr.sin_port = htons(port);

    if (connect(sock, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0)
        DieSysError("Connection failed");

    // Try to join chat
    join(sock);

    // Initialize threads
    pthread_t threads[NUM_THREADS];
    int rc;
    rc = pthread_create(&threads[0], NULL, parseMessages, (void *)&sock);
    if (rc)
        DieSysError("Failed to create parsing thread");

    rc = pthread_create(&threads[1], NULL, readCommands, (void *)&sock);
    if (rc)
        DieSysError("Failed to create parsing thread");

    pthread_exit(NULL);
    printf("Finished main\n");
    exit(0);
}