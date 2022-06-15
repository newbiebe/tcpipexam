#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h> 

#define BUF_SIZE 100 //버퍼 사이즈 define
#define MAX_CLNT 256 //클라이언트 최대 인원수 define

//호출
void *handle_clnt(void *arg);
void send_msg(char *msg, int len);
void error_handling(char *msg);

int clnt_cnt=0;             //클라이언트인원 함수
int clnt_socks[MAX_CLNT];
pthread_mutex_t mutx; //뮤텍스: 한 프로세스의 내부에서 여러 스레드의 임계 구역 제어를 위해 사용하는 객

int main(int argc, char *argv[])
{
        //필요 함수 정의
        int serv_sock, clnt_sock;
        struct sockaddr_in serv_adr, clnt_adr;
        int clnt_adr_sz;
        pthread_t t_id;
        if(argc!=2){                                        //전달된 정보개수가 2개가 아닌경우 출력
                printf("Usage : %s <port>\n", argv[0]);
                exit(1);
        }
    
        pthread_mutex_init(&mutx, NULL); //뮤텍스 생성
        serv_sock=socket(PF_INET,SOCK_STREAM,0); //서버소켓 (리스닝 소켓) 생성
        
        //주소 정보 초기화
        memset(&serv_adr,0,sizeof(serv_adr));  //구조체 변수  addr의 모든 멤버를 0으로 초기화
        serv_adr.sin_family=AF_INET; //주소 체계 지정
        serv_adr.sin_addr.s_addr=htonl(INADDR_ANY); //문자열 기반 IP주소 초기화
        serv_adr.sin_port=htons(atoi(argv[1])); //문자열 기반 Port번호 초기화

        //주소 정보 할당
        if(bind(serv_sock,(struct sockaddr*)&serv_adr, sizeof(serv_adr))==-1) //각 소켓에 IP주소와 포트를 할당
                error_handling("bind() error");
        if(listen(serv_sock,5)==-1)
                error_handling("listen() error");

        while(1)
        {
                clnt_adr_sz=sizeof(clnt_adr);
                clnt_sock=accept(serv_sock,(struct sockaddr*)&clnt_adr,&clnt_adr_sz); //소켓의 연결을 받아들임

                pthread_mutex_lock(&mutx); 
                //임계 영역 시작> 임계영역(둘 이상의 쓰레드가 동시에 실행하면 문제를 일으키는 영역)
                clnt_socks[clnt_cnt++]=clnt_sock;
                pthread_mutex_unlock(&mutx);
                //임계 영역 끝
                pthread_create(&t_id,NULL,handle_clnt,(void*)&clnt_sock); // 쓰레드 생성
                pthread_detach(t_id);//쓰레드 분리
                printf("Connected client IP: %s \n", inet_ntoa(clnt_adr.sin_addr));
        }
        close(serv_sock);
        return 0;
}

void *handle_clnt(void *arg)
{
        int clnt_sock=*((int*)arg);
        int str_len=0, i;
        char msg[BUF_SIZE];

        while((str_len=read(clnt_sock, msg, sizeof(msg)))!=0)
                send_msg(msg, str_len);
        
        pthread_mutex_lock(&mutx);
        //임계영역시작
        for(i=0;i<clnt_cnt;i++) //접속 되어있지 않는 클라이언트 제거
        {
                if(clnt_sock==clnt_socks[i])
                {
                        while(i++<clnt_cnt-1)
                                clnt_socks[i]=clnt_socks[i+1];
                        break;
                }
        }
        clnt_cnt--;
        pthread_mutex_unlock(&mutx);
        //임계 영역 끝
        close(clnt_sock); //파일 또는 소켓의 파일 디스크립터 전달
        return NULL;
}

void send_msg(char *msg,int len) // 모두에게보내는 메세지
{
        int i;
        pthread_mutex_lock(&mutx);
        //임계 영역 시작
        for(i=0;i<clnt_cnt;i++)               //숫자만큼 있는 클라이언트에게 모두 메세지를 보냄
                write(clnt_socks[i],msg,len);
        pthread_mutex_unlock(&mutx);
        //임계 영역 끝
}

void error_handling(char *msg) //에러 처리
{
        fputs(msg,stderr);
        fputc('\n',stderr);
        exit(1);
}
