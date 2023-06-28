#include <stdio.h>      
#include <stdlib.h>     
#include <string.h>     
#include <unistd.h>   
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <sys/un.h>   
#include <pthread.h> 
#include <signal.h> 
#include <fcntl.h>    
#define BUF_SIZE 256   
#define NAME_SIZE 20
#define MAX_CLNT 15                            
void *handle_clnt(void *arg);                  
void send_msg(int roomNum, char *msg, int len); 
void error_handling(char *msg);                 
void handler();                                 
char name[NAME_SIZE] = "[DEFAULT]";             
int clnt_cnt = 0;                               
int clnt_socks[MAX_CLNT];                       

int chatRoom_cnt = 0;           
int chatRoom_num[MAX_CLNT] = { -1, };     
pthread_mutex_t mutx;            
int chattingroom[3] = {0, 0, 0}; 
int main(int argc, char *argv[])
{
    int serv_sock, clnt_sock;              
    struct sockaddr_in serv_adr, clnt_adr; 
    int clnt_adr_sz;                     
    pthread_t t_id;                     
    if (argc != 2)        
    {
        printf("Usage : %s <port> \n", argv[0]);
        exit(1);
    }
    pthread_mutex_init(&mutx, NULL);      
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    memset(&serv_adr, 0, sizeof(serv_adr));  
    serv_adr.sin_family = AF_INET;         
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1])); 
    printf("<<<< Chat server >>>>\n");  
    printf("Server Port  : %d\n", atoi(argv[1]));
    printf("Server State  : Good\n");
    printf("Max Client : %d\n", MAX_CLNT);
    printf("<<<<   Log   >>>>\n");
    if (bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
    {
        error_handling("bind() error");
    }
    if (listen(serv_sock, 5) == -1)
    {
        error_handling("listen() error");
    }

    signal(SIGINT, handler); 
    while (1)           
    {
        clnt_adr_sz = sizeof(clnt_adr);                                           
        clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_adr, &clnt_adr_sz); 

        if (clnt_cnt >= MAX_CLNT) 
        {
            printf("사용자 제한 수는 %d입니다. 다시 시도해주십시오. \n", MAX_CLNT); 
        }
        else
        {

            pthread_mutex_lock(&mutx);
            int flags = fcntl(clnt_sock, F_GETFL, 0);
            fcntl(clnt_sock, F_SETFL, flags | O_NONBLOCK);               
            clnt_socks[clnt_cnt++] = clnt_sock;                           
            chatRoom_num[chatRoom_cnt++] = -1;                            
            pthread_mutex_unlock(&mutx);                                
            pthread_create(&t_id, NULL, handle_clnt, (void *)&clnt_sock); 
            pthread_detach(t_id);                                        
            printf("chatter (%d/%d)\n", clnt_cnt, MAX_CLNT);
            printf("[MAIN] 새로운 사용자가 접속했습니다 : %d\n", clnt_sock);
            
        }
    }
    close(serv_sock);
    return 0;
}
void *handle_clnt(void *arg) 
{
    int clnt_sock = *((int *)arg);  
    int str_len = 0, i;            
    int str_len2 = 0;        
    char msg[BUF_SIZE];         
    int roomNum = chatRoom_cnt - 1;
    
    sprintf(msg, "<MENU>\n1.채팅방 목록 보기\n2.채팅방 참여하기(사용법 :2 <채팅방 번호>)\n3.프로그램 종료\n(0을 입력하면 메뉴가 다시 표시됩니다)\n");
    write(clnt_sock, msg, sizeof(msg));
    memset(msg, 0, sizeof(msg));
    while ((str_len = read(clnt_sock, msg, sizeof(msg))) != 0) 
    {
        if (str_len > 0)
        {
            printf("[MAIN] 사용자 %d 메시지: %s\n", clnt_sock, msg);
            if (!strcmp(msg, "0\n"))
            { 
                sprintf(msg, "<MENU>\n1.채팅방 목록 보기\n2.채팅방 참여하기(사용법 :2 <채팅방 번호>)\n3.프로그램 종료\n(0을 입력하면 메뉴가 다시 표시됩니다)\n");
                write(clnt_sock, msg, sizeof(msg)); 
                memset(msg, 0, sizeof(msg));       
            }
            else if (!strcmp(msg, "1\n"))
            {
                int j = sprintf(msg, "<ChatRoom info>\n");
                for (int i = 0; i < 3; i++)
                {
                    j += sprintf(msg + j, "[ID: %d] Chatroom-%d (%d/5)\n", i, i, chattingroom[i]);
                }
                write(clnt_sock, msg, sizeof(msg));
                memset(msg, 0, sizeof(msg));
            }
            else if (msg[0] == '2')
            { 
                roomNum =  msg[2] - 48;
                chatRoom_num[clnt_sock - 4] = roomNum; 
                if (chattingroom[roomNum] > 4)
                {
                    printf("채팅방 제한 인원이 꽉찼습니다. 다음에 다시 시도해주십시오.\n");
                    chatRoom_num[clnt_sock - 4] =-1;
                    continue;
                }
                chattingroom[roomNum] += 1; 
                printf("[MAIN] 사용자 %d가 채팅방 %d에 참여합니다\n", clnt_sock, roomNum 
                );
                printf("[Ch.%d] 새로운 참가자 : %d\n", roomNum, clnt_sock);
                memset(msg, 0, sizeof(msg));
                char name_msg[NAME_SIZE + BUF_SIZE];
                sprintf(name, "[%d]", clnt_sock);
               
                while ((str_len2 = read(clnt_sock, msg, sizeof(msg))) != 0)
                {
                    if (str_len2 > 0)
                    {
                        printf("[Ch.%d] 사용자 %d의 메시지 : %s", roomNum, clnt_sock, msg);
                        if (!strcmp(msg, "q\n"))
                        {
                            printf("[Ch.%d] 사용자 %d를 채팅방에서 제거합니다.\n", chatRoom_num[roomNum], clnt_sock);
                            chattingroom[chatRoom_num[roomNum]] -= 1;
                            chatRoom_num[roomNum] = -1;
                            printf("[MAIN] 채팅방 탈퇴 사용자 탐지 : %d\n", clnt_sock);
                            memset(msg, 0, sizeof(msg));
                            break;
                        }
                        
                        sprintf(name_msg, "[%d]%s\n", clnt_sock, msg);
                        send_msg(roomNum, name_msg, strlen(name_msg)); 
                        memset(msg, 0, sizeof(msg));
                    }
                }
            }
        }
        else if (!strcmp(msg, "3\n"))
        {                                 
            pthread_mutex_lock(&mutx);   
            for (i = 0; i < clnt_cnt; i++) 
            {
                if (clnt_sock == clnt_socks[i]) 
                {
                    while (i++ < clnt_cnt - 1)
                    {
                        clnt_socks[i] = clnt_socks[i + 1];
                    }
                    printf("[MAIN] %d 사용자와의 접속을 해제합니다.\n", clnt_sock);
                    break;
                }
            }
            clnt_cnt--;                 
            pthread_mutex_unlock(&mutx); 
            close(clnt_sock);          
            return NULL;               
        }
        
    }
}
void send_msg(int roomNum, char *name_msg, int len)
{
    int i;                       
    pthread_mutex_lock(&mutx);  
    for (i = 0; i < clnt_cnt; i++)
    {
        if (chatRoom_num[i] == roomNum)
        {                              
            write(clnt_socks[i], name_msg, len);
        }
    }
    memset(name_msg, 0, sizeof(name_msg));
    pthread_mutex_unlock(&mutx); 
}
void error_handling(char *msg) 
{
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}
void handler()
{
    printf("(시그널 핸들러) 마무리 작업 시작!\n");
    exit(EXIT_SUCCESS);
}
