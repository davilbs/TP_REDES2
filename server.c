#include "common.h"

#define MAXPENDING 15

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