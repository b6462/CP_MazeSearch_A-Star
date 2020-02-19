#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<fcntl.h>

#define PORT 6666
#define MSG_SIZE 2048

struct send_msg
{
	int msg_length; //发送的消息主体的长度
	char msg_content[1024]; //消息主体
	char msg_ID[20]; //发送者ID
	char msg_REC[20]; //接收者ID
};

static int sock;
static char rbuf[MSG_SIZE];
static char wbuf[1024];

static struct sockaddr_in servaddr,clientaddr;

int private_checker = 0;//是否处于私聊模式
//0：非 
//1：未获得指定用户ID 
//2：ID确认

int main(int argc, char** argv)
{
	char ID[20];
	int rsize, wsize;//readSize writeSize

	if((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		printf("create socket error: %s(errno: %d)\n", strerror(errno), errno);
		return 0;
	}

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(PORT);
	servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");//客户端ip

	if(connect(sock, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
	{
		printf("connect error: %s(errno: %d)\n",strerror(errno),errno);
		return 0;
	}

	printf("Enter your ID:\n");
	
	bzero(rbuf, sizeof(rbuf));
	bzero(ID, sizeof(ID));

	rsize = read(STDIN_FILENO, rbuf, sizeof(rbuf));//阻塞读取ID
	system("clear");
	printf("YOUR ARE ONLINE\n");
	rbuf[strlen(rbuf) - 1] = '\0';//结尾加终结符
	sprintf(ID, "IDtag#%s", rbuf);//前面加上"IDtag#"一起打包在ID里
	send(sock, ID, sizeof(ID), 0);//给服务器发送ID

	fd_set fd_marker;
	FD_ZERO(&fd_marker);
	FD_SET(STDIN_FILENO, &fd_marker);//监测键盘输入
	FD_SET(sock, &fd_marker);//监测sock变化
	while(1)//同客户端 检查socket更新并轮循传送
	{
		FD_ZERO(&fd_marker);
		FD_SET(STDIN_FILENO, &fd_marker);
		FD_SET(sock, &fd_marker);
		switch(select(sock+1,&fd_marker,NULL,NULL,NULL))//监测有本地输入或服务器输入
		//int select(int nfds,  fd_set* readset,  fd_set* writeset,  fe_set* exceptset,  struct timeval* timeout);
		{
			case 0://timeout
				continue;
			case -1://error
				continue;
			default:
				if(FD_ISSET(STDIN_FILENO, &fd_marker))//输出到服务器
				{
					bzero(rbuf, sizeof(rbuf));
					if((rsize = read(STDIN_FILENO, rbuf, sizeof(rbuf))) > 0)
					{
						if(private_checker == 1)
						{
							bzero(ID, sizeof(ID));
							sprintf(ID, "PIDtag#%s", rbuf);
							send(sock, ID, sizeof(ID), 0);
							private_checker = 2;
						}
						else if(private_checker == 2)
						{
							send(sock, rbuf, rsize, 0);
							printf("\n");
						}
						else
						{
							send(sock, rbuf, rsize, 0);
							printf("\n");
						}
					}

				}

				if(FD_ISSET(sock, &fd_marker))//从服务器读取
				{	
					bzero(rbuf, sizeof(rbuf));
					if((rsize = recv(sock, rbuf, sizeof(rbuf), 0)) > 0)
					{
						if(!strcmp(rbuf, "end"))//如果返回自己退出指令
						{
							if(private_checker)//如果在private模式则退出private
							{
								private_checker = 0;
								printf("====Exit private mode====\n");
							}
							else
							{
								printf("\033[41mYOU ARE OFFLINE\033[0m\n");
								close(sock); 
								return 1;
							}
						}
						else if(!strcmp(rbuf, "sendTo") && private_checker == 0)
						{
							private_checker = 1;
							printf("====Enter private mode====\ntarget ID:\n");
						}
						else
						{
							struct send_msg msg_pack;
							memset(&msg_pack, 0x0, sizeof(msg_pack));
							memcpy(&msg_pack, &rbuf, sizeof(rbuf));

							if(!strcmp(msg_pack.msg_content,"end\0"))//获取其他用户退出信息
							{
								printf("\033[41m%s has left\033[0m\n\n", msg_pack.msg_ID);
							}
							else
							{
								printf("%s:\n\033[46m%s\033[0m\n\n", msg_pack.msg_ID, msg_pack.msg_content);
							}
						/*
						printf("===DEBUGGER===\n");
						printf("ID:%s\n", msg_pack.msg_ID);
						printf("Length:%d\n", msg_pack.msg_length);
						printf("Content:%s\n", msg_pack.msg_content);
						*/
						}

						
						
					}
				}
		}
	
	}

	return 0;
}