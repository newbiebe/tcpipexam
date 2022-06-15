#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define BUF_SIZE 100
#define NAME_SIZE 20

//호출
void *send_msg(void *arg);
void *recv_msg(void *arg);
void error_handling(char *msg);

char name[NAME_SIZE]="[DEFAULT]";
char msg[BUF_SIZE];

int main(int argc, char *argv[])
{
       //필요 함수 정의
        int sock;
        struct sockaddr_in serv_addr;
        pthread_t snd_thread,rcv_thread;
        void * thread_return;
        if(argc!=4){
                printf("Usage : %s <IP> <port> <name>\n", argv[0]);
                exit(1);
        }

        sprintf(name, "[%s]", argv[3]); //받은 값 3개 출력
        sock=socket(PF_INET,SOCK_STREAM,0);//소켓 생성

        //주소 정보 초기화
        memset(&serv_addr,0,sizeof(serv_addr));
        serv_addr.sin_family=AF_INET;
        serv_addr.sin_addr.s_addr=inet_addr(argv[1]);
        serv_addr.sin_port=htons(atoi(argv[2]));

        //주소 정보 할당
        if(connect(sock,(struct sockaddr*)&serv_addr, sizeof(serv_addr))==-1)
                error_handling("connect() error");
        
        //쓰레드 생성 소멸
        pthread_create(&snd_thread,NULL,send_msg,(void*)&sock); //보내는 쓰레드 생성
        pthread_create(&rcv_thread,NULL,recv_msg,(void*)&sock);  //받는 쓰레드 생성
        pthread_join(snd_thread,&thread_return);  //쓰레드 종료되는걸 기다림
        pthread_join(rcv_thread,&thread_return);
        close(sock); //소켓을 닫는다
        return 0;
}

void *send_msg(void *arg)       //소켓 메인에 전달
{
        int sock=*((int*)arg);
        char name_msg[NAME_SIZE+BUF_SIZE];

        while(1)
        {
            fgets(msg,BUF_SIZE,stdin);
            if(!strcmp(msg,"q\n")||!strcmp(msg,"Q\n"))  //q혹은 Q를 입력 받을시 disconnect
            {
                    close(sock);
                    exit(0);
            }
            sprintf(name_msg,"%s %s",name,msg);
            write(sock,name_msg,strlen(name_msg));
        }
        return NULL;
}

void * recv_msg(void *arg) //메인 쓰레드 읽기
{
        int sock=*((int*)arg);
        char name_msg[NAME_SIZE+BUF_SIZE];
        int str_len;
        while(1)
        {
                str_len=read(sock,name_msg,NAME_SIZE+BUF_SIZE-1);
                if(str_len==-1)
                        return (void*)-1;
                name_msg[str_len]=0;
                fputs(name_msg,stdout);
        }
        return NULL;
}

void error_handling(char *msg) //에러 처리
{
        fputs(msg,stderr);
        fputc('\n',stderr);
        exit(1);
}
