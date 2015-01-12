#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/socket.h>
#include<sys/epoll.h>
#include<fcntl.h>
#include<errno.h>

#define MAX_EVENTS 10
#define BUF_SIZE 1024

void setnonblocking(int fd);
void error_handling(char *message);

int main(int argc, char *argv[])
{
	int serv_sock; //server socket 생성 후 반환된 fd값
	int clnt_sock; 

	struct sockaddr_in serv_addr;//server 주소값 할당에 필요한 구조체
	struct sockaddr_in clnt_addr;
	socklen_t clnt_addr_size;

	struct epoll_event ev, events[MAX_EVENTS];
	int epollfd, n, nfds;
	int str_len = -1; //read한 캐릭터 수

	char message[BUF_SIZE];

	if(argc != 2)
	{
		printf("Usage: %s <port>\n", argv[0]);
		exit(1);
	}

	serv_sock = socket(PF_INET, SOCK_STREAM, 0); //서버 소켓 생성
	if(serv_sock == -1)
		error_handling("socket() error");

	memset(&serv_addr, 0, sizeof(serv_addr)); //bind() 주소 할당을 위한 준비
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(atoi(argv[1]));

	if(bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
		error_handling("bind() error");

	if(listen(serv_sock, 5) == -1) // 연결 대기 상태, 크기 5 큐 생성
		error_handling("listen() error");

	clnt_addr_size = sizeof(clnt_addr);

	epollfd = epoll_create(MAX_EVENTS); //MAX_EVENTS만큼의 커널폴링공간(?) 즉 이벤트 저장을 위한 공간 할당
	if(epollfd == -1)
		error_handling("epoll_create() error");

	events = malloc(sizeof(struct epoll_event)*MAX_EVENTS);//epoll초기화

	ev.events = EPOLLIN;
	ev.data.fd = serv_sock;

	if(epoll_ctl(epollfd, EPOLL_CTL_ADD, serv_sock, &ev) == -1)
		error_handling("epoll_wait() error");
		//serv_sock에 대해 EPOLL_CTL_ADD 작업을 한 후 그것을 ev에 저장

	for(;;)
	{
		if(nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1) == -1)
			//발생한 사건의 개수
			error_handling("epoll_wait() error");

		for(n = 0; n < nfds; n++)
		{
			if(events[n].data.fd == serv_sock)
			{
				clnt_sock == accept(serv_sock, (struct sockaddr*)&clnt_addr, clnt_addr_size);
				
				if(clnt_sock == -1)
					error_handling("accept() error");

				setnonblocking(clnt_sock);
				ev.events = EPOLLIN|EPOLLET;
				ev.data.fd = clnt_sock;
				epoll_ctl(epollfd, EPOLL_CTL_ADD, clnt_sock, &ev);
				epoll_ctl(epollfd, EPOLL_CTL_ADD, 0, &ev);
				printf("Connected client: %d \n", clnt_sock);
			}
			else if(events[n].data.fd == clnt_sock)
			{
		
				str_len = read(events[n].data.fd, message, BUF_SIZE);
					if(str_len < 0)
					{
						if(str_len == -1 && errno == EAGAIN)
							continue;
						error_handling("read error()");
					}
					else if(str_len == 0)
					{
						epoll_ctl(epollfd, EPOLL_CTL_DEL, events[n].data.fd, &ev);
						close(events[n].data.fd);

					}
					else
					{
						if(message[str_len-1] == 0)
						{
							printf("%s", message);
						}
						else 
						{
							error_handling("message error");
						}
					}

			}
			else //fd가 0
			{
				fgets(message, BUF_SIZE, stdin);
			
			}
		}
	}
	close(serv_sock);
	close(epollfd);
	return 0;
}




void setnonblocking(int fd)
{
	int flag = fcntl(fd, F_GETFL, 0);
	fnctl(fd, F_SETFL, flag|O_NONBLOCK);
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
