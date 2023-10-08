#include <iostream>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>





using namespace std;

#define SERVER_PORT 1163
#define MAX_LINE 256

int main(int argc, char* argv[]) {
    struct hostent* hp;
    struct sockaddr_in srv;
    char* host;
    char buf[MAX_LINE];
    int nClientSocket;
    int len;
    int nRet;


    // Check if user input host
    if (argc == 2) {
        host = argv[1];
    }
    else {
        cout << "Error > Did not give host argument\nUsage: ./client <host>\n\n";
        exit(EXIT_FAILURE);
    }


    // Translate host name into peer's IP address
    hp = gethostbyname(host);
    if (!hp) {
        cout << "Host Unknown\n\n";
        exit(EXIT_FAILURE);
    }
    else {
        cout << "Host Known\n\n";
    }


    // active open
    nClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (nClientSocket < 0) {
        cout << "Socket not Opened\n";
        exit(EXIT_FAILURE);
    }
    else {
        cout << "Socket Opened: " << nClientSocket << endl;
    }


    // Build address data structure
    bzero((char*)&srv, sizeof(srv));
    srv.sin_family = AF_INET;
    bcopy(hp->h_addr, (char*)&srv.sin_addr, hp->h_length);
    srv.sin_port = htons(SERVER_PORT);
    memset(&srv.sin_zero, 0, 8);


    nRet = connect(nClientSocket, (struct sockaddr*)&srv, sizeof(srv));
    if (nRet < 0)
    {
        cout << "Connection failed\n";
        close(nClientSocket);
        cout << "Closed socket: " << nClientSocket << endl;
        exit(EXIT_FAILURE);
    }
    else {
        cout << "Connected to the server\n";
        char buf[255] = { 0 };

        // Receive is a blocking call
        recv(nClientSocket, buf, 255, 0);
        cout << buf << endl;

        while (cout << "CLIENT> ") {
            fgets(buf, 256, stdin);
            char lastCommand[256];
            strncpy(lastCommand, buf, sizeof(buf));

            send(nClientSocket, buf, 256, 0);
            recv(nClientSocket, buf, 256, 0);
            cout << "CLIENT> " << buf << endl << endl;

            if (strcmp(lastCommand, "QUIT\n") == 0) {
                close(nClientSocket);
                cout << "Closed socket: " << nClientSocket << endl;
                exit(EXIT_SUCCESS);
            }
        }
    }
}
