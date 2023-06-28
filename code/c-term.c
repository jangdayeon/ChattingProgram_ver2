#include <stdio.h>                  //표준 입출력 기능
#include <stdlib.h>                 //메모리관리, 문자열 처리, 프로그램 종료 등 작업을 위하여
#include <string.h>                 //문자열 처리를 위하여
#include <unistd.h>                 // 프로세스 관리 및 입출력을 위하여
#include <sys/un.h>                 //소켓 사용을 위하여
#include <pthread.h>                //쓰레드 사용을 위하여
#include <sys/types.h>              //소켓 사용을 위하여
#include <arpa/inet.h>              //통신을 위하여
#include <sys/socket.h>             //소켓 사용을 위하여
#include <time.h>                   //시간 출력을 위하여
#include <fcntl.h>
#define BUF_SIZE 256                // 버퍼사이즈 정의
#define NAME_SIZE 20                // client 네임을 위한 버퍼
void *send_msg(void *arg);          // 서버에게 메시지 전달하기 위한 함수
void *recv_msg(void *arg);          // 서버로부터 전달받은 메시지를 출력하기 위한 함수
void error_handling(char *msg);     // 소켓연결 중 발생하는 에러들을 출력하기 위한 함수


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

char msg[BUF_SIZE];                 // 메시지를 전달받을 배열 선언
int ret;
char only_name_msg[NAME_SIZE - 10]; // 이름 변경을 위해 문자열을 입력받았을 때를 위한 작은 크기의 버퍼
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
    
    /////////////////////여기까지 unix
    int sock;                         // 소켓연결을 위한 메소드 실행 시 리턴값을 저장할 변수(스레드의 start_routine의 return 값을 저장할 주소)
    struct sockaddr_in serv_addr;     // 서버의 주소를 저장할 객체
    pthread_t snd_thread, rcv_thread; // 보내는 스레드와 메시지를 받는 스레드를 나누어 식별하도록했다.
    void *thread_return;              // 스레드 함수가 반환하는 값을 저장하기 위한 변수로 어떤 타입의 포인터든 저장할 수 있도록 선언하였다.
    if (argc != 3)
    { // client 코드를 실행할 때 ip, port, name을 같이 입력해서 실행하지 않으면 실행되는 if문이다.
        printf("Usage : %s <IP> <PORT> \n", argv[0]);
    }
    sock = socket(PF_INET, SOCK_STREAM, 0);   // 소켓을 생성한다.
    memset(&serv_addr, 0, sizeof(serv_addr)); // 서버의 주소를 초기화 후 주소체계, 주소, 포트번호를 입력한다.
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
    { // 소켓을 연결한다.
        error_handling("connect() error");
    }
    Sockets sockets;
    sockets.inet_sock = sock;
    sockets.unix_sock = client_fd;

    pthread_create(&snd_thread, NULL, send_msg, (void *)&sockets); // 받는 스레드를 생성한다.
    pthread_create(&rcv_thread, NULL, recv_msg, (void *)&sock); // 보내는 스레드를 생성한다.
    pthread_join(snd_thread, &thread_return);                   // 생성된 스레드의 동작이 끝날 때까지 대기한다.
    pthread_join(rcv_thread, &thread_return);                   // 생성된 스레드의 동작이 끝날 때까지 대기한다.

    close(sock); // 스레드의 동작이 다 끝나면 소켓을 닫는다.
    return 0;    // 이 함수를 종료한다.
}
void *send_msg(void *arg)
{                                            // 받는 스레드 시작 함수
    Sockets sockets = *((Sockets *)arg);                // 데이터를 전송할 소켓의 파일 디스크립터
    int sock = sockets.inet_sock;
    int client_fd = sockets.unix_sock;
    
    char name_msg[NAME_SIZE + BUF_SIZE];     // 자기의 이름과 자기가 작성한 글을 같이 보내기 위한 크기의 버퍼를 선언
    while (1)
    {
        // memset(msg, 0, sizeof(msg));
        ret = recv(client_fd, msg, BUF_SIZE, 0);
        fputs(msg,stdout);
        //fgets(msg, BUF_SIZE, stdin); // 클라이언트로부터 입력받은 값을 msg에 저장
        if (ret <= 0) continue;

        if (!strcmp(msg, "3\n"))
        { // 입력받은 값이 q 혹은 Q라면 종료하도록 한다.
            //write(sock, msg, strlen(msg));
            send(sock, msg, strlen(msg),0);
            close(sock);
            exit(0);
        }
        else
        { // 선택지를 입력하지 않았을 때 정상적으로 다른 클라이언트들과 채팅할 수 있게 한다.
            
            write(sock, msg, strlen(msg));
        }
        memset(msg, 0, BUF_SIZE);
    }
    return NULL;
}
void *recv_msg(void *arg)
{                                        // 서버로 부터 메시지를 받는 스레드의 시작 함수
    int sock = *((int *)arg);            // 데이터를 받을 소켓의 파일 디스크립터
    char name_msg[NAME_SIZE + BUF_SIZE]; // 다른 클라이언트의 이름과 글을 합친 크기만큼 버퍼 생성
    int str_len;                         // read메소드의 리턴값을 저장할 변수
    while (1)
    {
        str_len = read(sock, name_msg, NAME_SIZE + BUF_SIZE - 1); // 서버로부터 메시지를 받는다.
        if (str_len == -1)
        { // read를 실패할 경우 return하여 종료
            return (void *)-1;
        }
        name_msg[str_len] = 0;   // 버퍼 끝에 0을 주어 문자열의 끝임을 알 수 있도록 한다.
        fputs(name_msg, stdout); // 메시지를 출력
        //ret = send(client_fd,msg,strlen(msg),0);
    }
    return NULL;
}
void error_handling(char *msg)
{ // 소켓 관련 에러들을 해결하기 위한 함수
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}
