#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>
#include<fcntl.h>
#include<arpa/inet.h>

#define MAX_CLIENT 10
#define PORT 6666
#define MSG_SIZE 2048

struct clientData{
	int sock_tag;//对应client的sock
	char ID[20];
	char PID[20]; //Private
	int Pchecker;
	int checker;//是否发送登陆信息checker
};

struct send_msg
{
	int msg_length; //发送的消息主体的长度
	char msg_content[1024]; //消息主体
	char msg_ID[20]; //发送者ID
	char msg_REC[20]; //接收者ID
};

static struct clientData client_data[MAX_CLIENT];
static char rbuf[1024]; //read buffer
static char rbuf_[1024]; //temp read buffer
static char wbuf[MSG_SIZE]; //write buffer
static struct sockaddr_in  servaddr, clientaddr;



int main(int argc, char** argv)
{
	int sock;

	if((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		printf("create socket error: %s(errno: %d)\n", strerror(errno), errno);
		return 0;
	}

	int max_sock = sock;//select需要检查动态阻塞的数量

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(PORT);

	if(bind(sock, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1)
	{
		printf("bind socket error: %s(errno: %d)\n", strerror(errno), errno);
		return 0;
	}

	if(listen(sock, MAX_CLIENT) == -1)
	{
		printf("listen socket error: %s(errno: %d)\n", strerror(errno), errno);
		return 0;
	}

	printf("======server settled, waiting...======\n");

	int i = 0, j = 0;
	fd_set fd_marker;//更新检查描述符结构体 描述符集合

	while(1)
	{
		int rsize;//readSize
		FD_ZERO(&fd_marker);//套接字集合清零
		FD_SET(STDIN_FILENO, &fd_marker);//监测键盘输入
		FD_SET(sock, &fd_marker);//添加sock到套接字集合观测
		for(i = 0 ; i < MAX_CLIENT; i++)//遍历cli用户数据，更新观测套接字fd_marker集合
		{
			if(client_data[i].sock_tag != 0)//如果client_data[i]有记录，则将此套接字添加到fd_marker集合中
			{
				FD_SET(client_data[i].sock_tag, &fd_marker);
			}
		}
		switch(select(max_sock + 1, &fd_marker, NULL, NULL, NULL))//检查sock发生变化 是否有新用户、消息接入
		//int select(int nfds,  fd_set* readset,  fd_set* writeset,  fe_set* exceptset,  struct timeval* timeout);
		{
			case 0://timeout
			continue;
			case -1://error
			continue;
			default:
				if(FD_ISSET(STDIN_FILENO, &fd_marker))//监测本地指令
				{
					bzero(rbuf, sizeof(rbuf));
					if((rsize = read(STDIN_FILENO, rbuf, sizeof(rbuf))) > 0)
					{
						if(!strcmp(rbuf, "shutdown\n"))
						{
							close(sock);
							printf("\033[41m======server closed======\033[0m\n");
							return 1;
						}
					}
				}

				if(FD_ISSET(sock, &fd_marker))//检查对应sock是否在fd_marker集合的监测中
				{
					for(i = 0; i < MAX_CLIENT; i++)
					{
						if(client_data[i].sock_tag == 0)//选择空位
						{
							int sin_size = sizeof(struct sockaddr);
							client_data[i].sock_tag = accept(sock, (struct sockaddr *) &clientaddr,&sin_size);//获取连线客户端addr
							if(client_data[i].sock_tag == -1)
							{
								perror("Acceptance Error\n");
								client_data[i].sock_tag = 0;
								break;
							}
							client_data[i].checker = 0;//第一次登陆checker 后续更新初次登陆信息
							if(client_data[i].sock_tag > max_sock)//更新sock描述字
								max_sock = client_data[i].sock_tag;
							printf("Acceptance Success\n");
							break;
						}
					}
				}
				for(i = 0; i < MAX_CLIENT; i++)
				{
					if(client_data[i].sock_tag != 0)//非空数据集
					{
						if(FD_ISSET(client_data[i].sock_tag, &fd_marker))//描述字在监听集内
						{
							bzero(rbuf, sizeof(rbuf));
							bzero(rbuf_, sizeof(rbuf_));
							if((rsize = recv(client_data[i].sock_tag, rbuf,sizeof(rbuf), 0))>0)//如果读取到read
							{
								if(client_data[i].checker)//如果不是登陆初始态
								{
									printf("received message\n%s\n", wbuf);//本地直接显示写入数据

									printf("rsize:%d\n", rsize);
									printf("rcontent:%s\n", rbuf);
									printf("rID:%s\n", client_data[i].ID);
									//TEMP CHECKER
									struct send_msg msg_pack;//设置传输包结构体
									memcpy(msg_pack.msg_ID,&client_data[i].ID,sizeof(client_data[i].ID));//设定ID
									memset(msg_pack.msg_content,0x0,sizeof(msg_pack.msg_content));

									if(client_data[i].Pchecker)
									{
										sprintf(rbuf_, "Private msg:\n%s", rbuf);
										rsize += 13;
									}
									else
									{
										memcpy(rbuf_, &rbuf, sizeof(rbuf));
									}
									memcpy(msg_pack.msg_content, &rbuf_, sizeof(rbuf_));
									msg_pack.msg_length = rsize;//长度
									msg_pack.msg_content[msg_pack.msg_length - 1] = '\0';//设定终结符
									
									/*
									printf("===DEBUGGER===\n");
									printf("ID:%s\n", msg_pack.msg_ID);
									printf("Length:%d\n", msg_pack.msg_length);
									printf("Content:%s\n", msg_pack.msg_content);
									printf("---DEBUGGER---\n");
									*/
									if(!strcmp(rbuf, "sendTo\n"))//启动sendTo
									{
										send(client_data[i].sock_tag, "sendTo", 6, 0);//发送给准备进入private的用户
									}
									else if(!strncmp(rbuf, "PIDtag#", 7))//如果传入privateIDtag
									{
										strcpy(client_data[i].PID, rbuf + 7);//设定PID;
										printf("PID set:%s\n", client_data[i].PID);
										client_data[i].Pchecker = 1;
									}
									else if(!strcmp(rbuf, "end\n"))//如果当前用户写入end
									{
										send(client_data[i].sock_tag, "end", 3, 0);//发送给退出的用户
										if(client_data[i].Pchecker)//如果在private态则退出 PID置空
										{
											memset(client_data[i].PID,0x0,sizeof(client_data[i].PID));
											client_data[i].Pchecker = 0;
										}
										else//不在private态
										{
											for(j = 0; j < MAX_CLIENT; j++)
											{
												if(client_data[j].sock_tag == 0 || j == i || client_data[j].checker == 0)//跳过空数据、自身、未完成登陆发言用户
													continue;
												if(!strncmp(client_data[i].PID, client_data[j].ID, strlen(client_data[j].ID)) || !client_data[i].Pchecker)//筛选
												{
													send(client_data[j].sock_tag, (char *)&msg_pack, sizeof(msg_pack), 0);
												}
												printf("marker1\n");
											}
											close(client_data[i].sock_tag);//对应socket关闭
											printf("%s has quit\n", client_data[i].ID);
											client_data[i].sock_tag = 0;
											client_data[i].checker = 0;
											memset(client_data[i].ID, 0, sizeof(client_data[i].ID));
										}
									}
									else
									{

										for(j = 0; j < MAX_CLIENT; j++)
										{
											if(client_data[j].sock_tag == 0 || j == i || client_data[j].checker == 0)//跳过空数据、自身、未完成登陆发言用户
												continue;
											if(!strncmp(client_data[i].PID, client_data[j].ID, strlen(client_data[j].ID)) || !client_data[i].Pchecker)//筛选
											{
												send(client_data[j].sock_tag, (char *)&msg_pack, sizeof(msg_pack), 0);
											}
											printf("marker1\n");
										}
									}

									
									
								}
								else//如果是第一次发言，非登陆
								{
									if(!strncmp(rbuf,"IDtag#",6))//确认是否传入ID信息
									{
										strcpy(client_data[i].ID, rbuf + 6);//设定ID
										printf("%s Has Entered\n\n", rbuf);
										client_data[i].checker = 1;//设置完成登陆发言
										client_data[i].Pchecker = 0;
										for(j = 0; j < MAX_CLIENT; j++)
										{
											if(client_data[j].sock_tag == 0 || j == i || client_data[j].checker == 0)//跳过空数据、自身、未完成登陆发言用户
												continue;

											char temp_msg[] = "Just Entered";
											struct send_msg msg_pack;//设置传输包结构体
											memcpy(msg_pack.msg_ID,&client_data[i].ID,sizeof(client_data[i].ID));//设定ID
											memset(msg_pack.msg_content,0x0,sizeof(msg_pack.msg_content));
											memcpy(msg_pack.msg_content, &temp_msg, sizeof(temp_msg));
											send(client_data[j].sock_tag, (char *)&msg_pack, sizeof(msg_pack), 0);
										}
									}
								}
							}
						}
					}
				}
			}
		}
	return 0;
}
