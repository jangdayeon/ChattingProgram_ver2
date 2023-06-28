

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>

#define BUFFER_SIZE 256

#ifdef CLIENT1
    #define SOCKET_NAME "./sock1"
#endif
#ifdef CLIENT2
    #define SOCKET_NAME "./sock2"
#endif
#ifdef CLIENT3
    #define SOCKET_NAME "./sock3"
#endif
#ifdef CLIENT4
    #define SOCKET_NAME "./sock4"
#endif

int main() {
    int client_fd;
    struct sockaddr_un server_addr;
    char buffer[BUFFER_SIZE];
    int ret;

    // create a new socket
    client_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // set the server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
#ifdef SOCKET_NAME
    strncpy(server_addr.sun_path, SOCKET_NAME, sizeof(server_addr.sun_path) - 1);
#endif
    
    // connect to the server
    ret = connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (ret == -1) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    printf("[Info] Unix socket : connected to the server\n");
	int flags = fcntl(client_fd, F_GETFL,0);
	fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);
    // read input from the terminal and send it to the server
    while (1) {
	printf("> Enter: \n");
        memset(buffer, 0, sizeof(buffer));
        ret = read(STDIN_FILENO, buffer, BUFFER_SIZE);
        if (ret == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }
        ret = send(client_fd, buffer, ret, 0);
        if (ret == -1) {
            perror("send");
            exit(EXIT_FAILURE);
        }

        // check if the client wants to quit
        if (strcmp(buffer, "3\n") == 0) {
            printf("Terminate. . .\n");
            break;
        }
	 
    }

    close(client_fd);
    printf(":Success\n[Info] Closing socket\n");
    return 0;
}
