#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sqlite3.h>

#define PORT 33333

int admin=0;// not exist
int sign=1;//say

struct message
{
	int action;
	char from_name[20];
	char toname[20];
	char passwd[20];
	char msg [1024];
	int say;
};
void Hlp()
{
	printf("-------------------------------------------------\n");
	printf("欢迎来到聊天室\n");
	printf("输入log登录\n");
	printf("输入reg注册\n");
	printf("输入sto对某人说话\n");
	printf("输入sta对大家说话\n");
	printf("输入che查看当前在线用户\n");
	printf("输入hlp查看帮助\n");
	printf("输入ext退出聊天室\n");
	printf("--------------------------------------------------\n");
}



void *recv_message(void* arg)
{
    int ret;
	int cfd = *((int *)arg);
	
	struct message *msg = (struct message *)malloc(sizeof(struct message));
	
    while(1)
    {
        memset(msg, 0, sizeof(struct message));
        if((ret = recv(cfd, msg, sizeof(struct message),0)) < 0)
        {
        	perror("recv error !\n");
        	exit(1);
		}
		
		switch(msg->action)
		{

			case 1:
				{
					if(msg->msg[0] == 0)
					{
						printf("the user isn't existed!\n");//此人不存在
					}
					else if(msg->msg[0] == -1)
					{
						printf("This user is logged in\n");//此人已登录
					}
					else 
					{
							printf("The user logs success!\n");
					}
					break;
				}
			

			case 2:
				{
					
					if(msg->msg[0] == 0)
					{
						printf("The user alreadly exists!\n");
					}
					else 
					{
							printf("The user regs success!\n");
					}
					
					break;
				}
			

			case 3:
				{
					printf("Recv message from %s: %s\n", msg->from_name, msg->msg);
					break;
				}
			

			case 4:
				{
					printf("The broadcast message received from %s: %s\n", msg->from_name, msg->msg);
					break;
				}
			
			case 5:
				{
					if(msg->toname == NULL)
						printf("NULL\n");
					else 
						printf("---%s\n", msg->toname);
					break;
				}
			
			case 6:
				{
					printf("Help  success!\n");
					break;
				}
			

			case 7:
				{
					printf("%s is log out!\n",msg->from_name);
					exit(1);
					break;
				}
			
			
		}
    }
}

int main()
{
	pthread_t id;
	int sockfd;
	struct sockaddr_in s_addr;
	
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket error !\n");
		exit(1);
	}
	printf("socket success !\n");
	
	bzero(&s_addr, sizeof(struct sockaddr_in));
	s_addr.sin_family = AF_INET;
	s_addr.sin_port = htons(PORT);
	s_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	
	if(connect(sockfd, (struct sockaddr *)(&s_addr),sizeof(struct sockaddr_in)) < 0)
	{
		perror("connect error !\n");
		exit(1);
	}
	printf("connect success !\n");
	
	if(pthread_create(&id, NULL, recv_message, (void *)(&sockfd)) != 0)
	{
		perror("pthread_create error !\n");
		exit(1);
	}
	
	Hlp();
	struct message *msg = (struct message *)malloc(sizeof(struct message));
	char order[6];
	char name[20];
	char *passwd;
	char toname[20];
	char m_message[1024];

	char *command[11] = {"log\0","reg\0","sto\0","sta\0","che\0","hlp\0","tra\0","ext\0","nos\0","say\0","out\0"};
	
	while(1)
	{
		int i;
		printf("Please enter a command\n");
		scanf("%s", order);

		if(strcmp(order, command[0]) == 0)
		{
			msg->action = 1;	
			printf("Please enter your name:\n");
			scanf("%s", name);
			//scanf("%s", passwd);
			passwd=getpass("Please enter your passwd:\n");
			//printf("passwd:%s\n",passwd);

			strcpy(msg->from_name, name);
			strcpy(msg->passwd, passwd);
			if(send(sockfd, msg, sizeof(struct message), 0) < 0)
			{
				perror("reg: send error!\n");
				exit(1);
			}
			sleep(1);	
		}

		if(strcmp(order, command[1]) == 0)
		{
			msg->action = 2;
			printf("Please enter your name:\n");
			scanf("%s", name);
			//printf("Please enter your passwd:\n");
			//scanf("%s", passwd);
			passwd=getpass("Please enter your passwd:\n");
			//printf("passwd:%s\n",passwd);

			strcpy(msg->from_name, name);
			strcpy(msg->passwd, passwd);
			if(send(sockfd, msg, sizeof(struct message), 0) < 0)
			{
				perror("reg: send error!\n");
				exit(1);
			}
			
			sleep(1);
		}
			
		// sto
		if(strcmp(order, command[2]) == 0)
		{
				msg->action = 3;
				printf("Please input the name you want to send:\n");
				scanf("%s", name);
				printf("Please input the message send to %s:\n", name);
				scanf("%s", m_message);
				strcpy(msg->toname, name);
				strcpy(msg->msg, m_message);
				if(send(sockfd, msg, sizeof(struct message), 0) < 0)
				{
					perror("reg: send error!\n");
					exit(1);
				}			
			sleep(1);	
			
		}
			
		// sta
		if(strcmp(order, command[3]) == 0)
		{
				msg->action = 4;
				printf("Please enter the messge send to all:\n");
				scanf("%s",m_message);
						
				strcpy(msg->msg, m_message);
				send(sockfd, msg, sizeof(struct message), 0);
			sleep(1);
		}
			
		// che
		if(strcmp(order, command[4]) == 0)
		{
			msg->action = 5;
			printf("Online users : ");
			send(sockfd, msg, sizeof(struct message), 0);
			sleep(1);
		}
			
		//hlp
		if(strcmp(order, command[5]) == 0)
		{
			msg->action = 6;
			Hlp();
			sleep(1);
		}
				

		if(strcmp(order, command[6]) == 0)
		{
			msg->action = 7;
			send(sockfd, msg, sizeof(struct message), 0);
			sleep(1);
		}
		usleep(10);
	}

	shutdown(sockfd, SHUT_RDWR);
	return 0;
}

