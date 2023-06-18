#include "common.h"

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

    // Main client loop
    char msg[BUFFERSIZE];
    for (;;)
    {
        uint8_t idMsg = 3;
        uint8_t idSend = 3;
        uint8_t idRecv = 3;
        uint8_t header1 = 0;
        header1 += (idMsg << 4);
        header1 += idSend;
        memset(msg, 0, BUFFERSIZE);
        msg[0] = header1;
        msg[1] = (idRecv << 4);
        size_t strLen = strlen(msg);
        ssize_t numBytes = send(sock, msg, strLen, 0);
    }
    exit(0);
}