#include <stdio.h>                  //ǥ�� ����� ���
#include <stdlib.h>                 //�޸𸮰���, ���ڿ� ó��, ���α׷� ���� �� �۾��� ���Ͽ�
#include <string.h>                 //���ڿ� ó���� ���Ͽ�
#include <unistd.h>                 // ���μ��� ���� �� ������� ���Ͽ�
#include <sys/un.h>                 //���� ����� ���Ͽ�
#include <pthread.h>                //������ ����� ���Ͽ�
#include <sys/types.h>              //���� ����� ���Ͽ�
#include <arpa/inet.h>              //����� ���Ͽ�
#include <sys/socket.h>             //���� ����� ���Ͽ�
#include <time.h>                   //�ð� ����� ���Ͽ�
#include <fcntl.h>
#define BUF_SIZE 256                // ���ۻ����� ����
#define NAME_SIZE 20                // client ������ ���� ����
void *send_msg(void *arg);          // �������� �޽��� �����ϱ� ���� �Լ�
void *recv_msg(void *arg);          // �����κ��� ���޹��� �޽����� ����ϱ� ���� �Լ�
void error_handling(char *msg);     // ���Ͽ��� �� �߻��ϴ� �������� ����ϱ� ���� �Լ�


//unix
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

typedef struct _Sockets
{
    int inet_sock;
    int unix_sock;
} Sockets;

char msg[BUF_SIZE];                 // �޽����� ���޹��� �迭 ����
int ret;
char only_name_msg[NAME_SIZE - 10]; // �̸� ������ ���� ���ڿ��� �Է¹޾��� ���� ���� ���� ũ���� ����
int server_fd, client_fd;
int main(int argc, char *argv[])
{   
    /////////////////unix
    int server_fd, client_fd;
    struct sockaddr_un server_addr, client_addr;
    int ret;

    // remove the socket file if it already exists
#ifdef SOCKET_NAME
    remove(SOCKET_NAME);
#endif
    // create a new socket
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // set the server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
#ifdef SOCKET_NAME
    strncpy(server_addr.sun_path, SOCKET_NAME, sizeof(server_addr.sun_path));
#endif
   
    // bind the socket to the address
    ret = bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (ret == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    // listen for incoming connections
    ret = listen(server_fd, 1);
    if (ret == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("[Info] Unix socket : waiting for conn req\n");

    // accept a new client connection
    socklen_t client_len = sizeof(client_addr);
    client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd == -1) {
        perror("accept");
        exit(EXIT_FAILURE);
    }
    int flags = fcntl(client_fd, F_GETFL, 0);
	fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);
    printf("[Info] Unix socket : Client connected\n");
    
    /////////////////////������� unix
    int sock;                         // ���Ͽ����� ���� �޼ҵ� ���� �� ���ϰ��� ������ ����(�������� start_routine�� return ���� ������ �ּ�)
    struct sockaddr_in serv_addr;     // ������ �ּҸ� ������ ��ü
    pthread_t snd_thread, rcv_thread; // ������ ������� �޽����� �޴� �����带 ������ �ĺ��ϵ����ߴ�.
    void *thread_return;              // ������ �Լ��� ��ȯ�ϴ� ���� �����ϱ� ���� ������ � Ÿ���� �����͵� ������ �� �ֵ��� �����Ͽ���.
    if (argc != 3)
    { // client �ڵ带 ������ �� ip, port, name�� ���� �Է��ؼ� �������� ������ ����Ǵ� if���̴�.
        printf("Usage : %s <IP> <PORT> \n", argv[0]);
    }
    sock = socket(PF_INET, SOCK_STREAM, 0);   // ������ �����Ѵ�.
    memset(&serv_addr, 0, sizeof(serv_addr)); // ������ �ּҸ� �ʱ�ȭ �� �ּ�ü��, �ּ�, ��Ʈ��ȣ�� �Է��Ѵ�.
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
    { // ������ �����Ѵ�.
        error_handling("connect() error");
    }
    Sockets sockets;
    sockets.inet_sock = sock;
    sockets.unix_sock = client_fd;

    pthread_create(&snd_thread, NULL, send_msg, (void *)&sockets); // �޴� �����带 �����Ѵ�.
    pthread_create(&rcv_thread, NULL, recv_msg, (void *)&sock); // ������ �����带 �����Ѵ�.
    pthread_join(snd_thread, &thread_return);                   // ������ �������� ������ ���� ������ ����Ѵ�.
    pthread_join(rcv_thread, &thread_return);                   // ������ �������� ������ ���� ������ ����Ѵ�.

    close(sock); // �������� ������ �� ������ ������ �ݴ´�.
    return 0;    // �� �Լ��� �����Ѵ�.
}
void *send_msg(void *arg)
{                                            // �޴� ������ ���� �Լ�
    Sockets sockets = *((Sockets *)arg);                // �����͸� ������ ������ ���� ��ũ����
    int sock = sockets.inet_sock;
    int client_fd = sockets.unix_sock;
    
    char name_msg[NAME_SIZE + BUF_SIZE];     // �ڱ��� �̸��� �ڱⰡ �ۼ��� ���� ���� ������ ���� ũ���� ���۸� ����
    while (1)
    {
        // memset(msg, 0, sizeof(msg));
        ret = recv(client_fd, msg, BUF_SIZE, 0);
        fputs(msg,stdout);
        //fgets(msg, BUF_SIZE, stdin); // Ŭ���̾�Ʈ�κ��� �Է¹��� ���� msg�� ����
        if (ret <= 0) continue;

        if (!strcmp(msg, "3\n"))
        { // �Է¹��� ���� q Ȥ�� Q��� �����ϵ��� �Ѵ�.
            //write(sock, msg, strlen(msg));
            send(sock, msg, strlen(msg),0);
            close(sock);
            exit(0);
        }
        else
        { // �������� �Է����� �ʾ��� �� ���������� �ٸ� Ŭ���̾�Ʈ��� ä���� �� �ְ� �Ѵ�.
            
            write(sock, msg, strlen(msg));
        }
        memset(msg, 0, BUF_SIZE);
    }
    return NULL;
}
void *recv_msg(void *arg)
{                                        // ������ ���� �޽����� �޴� �������� ���� �Լ�
    int sock = *((int *)arg);            // �����͸� ���� ������ ���� ��ũ����
    char name_msg[NAME_SIZE + BUF_SIZE]; // �ٸ� Ŭ���̾�Ʈ�� �̸��� ���� ��ģ ũ�⸸ŭ ���� ����
    int str_len;                         // read�޼ҵ��� ���ϰ��� ������ ����
    while (1)
    {
        str_len = read(sock, name_msg, NAME_SIZE + BUF_SIZE - 1); // �����κ��� �޽����� �޴´�.
        if (str_len == -1)
        { // read�� ������ ��� return�Ͽ� ����
            return (void *)-1;
        }
        name_msg[str_len] = 0;   // ���� ���� 0�� �־� ���ڿ��� ������ �� �� �ֵ��� �Ѵ�.
        fputs(name_msg, stdout); // �޽����� ���
        //ret = send(client_fd,msg,strlen(msg),0);
    }
    return NULL;
}
void error_handling(char *msg)
{ // ���� ���� �������� �ذ��ϱ� ���� �Լ�
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}
