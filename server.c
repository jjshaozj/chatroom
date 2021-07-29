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

int b, k;
char mname[20];

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

//在注册表查找此人
int Check_reg(sqlite3 *db, char name[], char passwd[])
{
    char *error;
    int row, col;
	char **result;
    int ret;

	//获取表的信息
	ret = sqlite3_get_table(db, "select * from reg_user;", &result, &row, &col, &error);
    if(ret != SQLITE_OK)
    {
        perror("check error !\n");
        sqlite3_close(db);
        exit(1);
    }
    
	if(row == 0 || col == 0)
	{
		return 0;
	}

    int i, j;
    for(i = 0; i <= row; i++)
    {
    	int a = strcmp(result[i * col + 1], name);
		if(!a)//不为0
    	{
    		sqlite3_free_table(result);
    		return 1;//此人存在于注册表内
		}
    }
    sqlite3_free_table(result);
    return 0;//不存在
}

//在登录表查找此人的信息
int Check_log(sqlite3 *db, char name[], char passwd[])
{
    char *error;
    int row, col;
	char **result;
    int ret;
	
	ret = sqlite3_get_table(db, "select * from log_user;", &result, &row, &col, &error);
    if(ret != SQLITE_OK)
    {
        perror("check error !\n");
        sqlite3_close(db);
        exit(1);
    }

	if(row == 0 || col == 0)
	{
		return 0;
	}
    
    int i, j;
    for(i = 0; i <= row; i++)
    {
    	int a = strcmp(result[i * col + 1], name);
    	int b = strcmp(result[i * col + 2], passwd);
    	if(!a && !b)
    	{
    		sqlite3_free_table(result);
    		return 1;
		}
    }
    sqlite3_free_table(result);
    return 0;
}

//将用户信息(注册、登录)插入数据库   F return 0     S return 1
int insert_chat_user(char name[], char passwd[], int cid, int flag)
{
	//打开数据库
	sqlite3 *db = NULL;
	int ret = sqlite3_open("/home/szj/Desktop/work/chatroom.db", &db);
	if(ret != SQLITE_OK)
	{
		perror("open sqlite error\n");
		exit(1);
	}	
	if(flag == 0 && !Check_reg(db, name, passwd))
	{
		return 0;	//	log, not existed in reg_user
	}
	else if(flag == 0 && Check_log(db, name, passwd))
	{
		return -1;	//	log, has existed in log_user
	}
	else if(flag == 1 && Check_reg(db, name, passwd))
	{
		return 0;	//	reg, has existed in reg_user
	}

	char *error = NULL;
	char sql[100];
	memset(sql,0,100);

	if(flag == 0)
	{
		sprintf(sql, "insert into log_user values(NULL, '%s', '%s', %d,datetime('now'))", name, passwd, cid);
	}else if(flag == 1)
	{
		sprintf(sql, "insert into reg_user values(NULL, '%s', '%s', %d,datetime('now'))", name, passwd, cid);
	}
	ret = sqlite3_exec(db,sql, NULL, NULL, &error);
	if(ret != SQLITE_OK)
	{
		perror("exec error !\n");
		sqlite3_close(db);
		exit(1);
	}
	sqlite3_close(db);
	return 1;
}


void insert_sto_history(char *from_name, char* toname, char *msg)
{

	//打开数据库
	sqlite3 *db = NULL;
	int ret = sqlite3_open("/home/szj/Desktop/work/chatroom.db", &db);
	if(ret != SQLITE_OK)
	{
		perror("open sqlite error\n");
		exit(1);
	}
	
	char *error = NULL;
	char sql[100];
	memset(sql,0,100);

	sprintf(sql, "insert into data values('%s', '%s', '%s', datetime('now'))", from_name,toname, msg);
	
	ret = sqlite3_exec(db,sql, NULL, NULL, &error);
	if(ret != SQLITE_OK)
	{
		perror("exec error !\n");
		sqlite3_close(db);
		exit(1);
	}
	sqlite3_close(db);
	
}

//根据客户端sockfd发送的用户名查找
int GetId(char *name)
{
	sqlite3 *db = NULL;
	int ret = sqlite3_open("/home/szj/Desktop/work/chatroom.db", &db);
	if(ret != SQLITE_OK)
	{
		perror("open sqlite error\n");
		exit(1);
	}
	
    char *error;
    int row, col;
	char **result;
	
	ret = sqlite3_get_table(db, "select * from log_user", &result, &row, &col, &error);//查找登陆表的信息
    if(ret != SQLITE_OK)
    {
        perror("get cid error !\n");
        sqlite3_close(db);
        exit(1);
    }

    int i, j, a;
    for(i = 1; i <= row; i++)
    {
    	a = strcmp(result[i * col + 1], name);
    	if(a == 0)//a=0  找到该用户
    	{
			
			//把字符串转为整型
			b = 0;
			k = 0;
			while(result[i*col+3][k]!='\0' && result[i*col+3][k]!=' ')
			{
				b = (int)(b * 10 + (result[i*col+3][k] - 48));
				k++;
			}
			sqlite3_free_table(result);
    		sqlite3_close(db);
			return b;
		}
    }
    sqlite3_free_table(result);
    sqlite3_close(db);
    return 0;
}


char *GetName(int i)
{
	sqlite3 *db = NULL;
	int ret = sqlite3_open("/home/szj/Desktop/work/chatroom.db", &db);
	if(ret != SQLITE_OK)
	{
		perror("open sqlite error\n");
		exit(1);
	}

    char *error;
    int row, col;
	char **result;

	mname[0] = 0;
	ret = sqlite3_get_table(db, "select * from log_user", &result, &row, &col, &error);
    if(ret != SQLITE_OK)
    {
        perror("error !\n");
        sqlite3_close(db);
        exit(1);
    }

	k = b = 0;
    if(i <= row)
	{
		while(result[i * col + 1][k] != '\0' && result[i * col + 1][k] != ' ')
		{
			mname[b++] = result[i * col + 1][k++];
		}
		mname[b] = 0;
	}

	sqlite3_free_table(result);
    sqlite3_close(db);

	if(b == 0)
		return NULL;
	return mname;
}

void out(char *fromname)
{
	sqlite3 *db = NULL;
	int ret = sqlite3_open("/home/szj/Desktop/work/chatroom.db", &db);
	if(ret != SQLITE_OK)
	{
		perror("open sqlite error\n");
		exit(1);
	}

	char *error;
    char sql[100];
    memset(sql, 0, 100);
    sprintf(sql, "delete from log_user where name='%s'",fromname);
    ret = sqlite3_exec(db, sql, NULL, NULL, &error);
    if(ret != SQLITE_OK)
    {
	    perror("error !\n");
        sqlite3_close(db);
		exit(1);
    }
    sqlite3_close(db);
}


void *recv_message(void *arg)
{
	int ret;
	int to_cfd;
	int mark=0;
	int cfd = *((int *)arg);
	
	struct message *msg = (struct message *)malloc(sizeof(struct message));

	while(1)
	{
		memset(msg, 0, sizeof(struct message));
		if((ret = recv(cfd, msg, sizeof(struct message), 0)) < 0) //出错
		{
			perror("recv error !\n");
			exit(1);
		}
		
		if(ret == 0)          //连接关闭
		{
			printf("%d is log out !\n",cfd);
			pthread_exit(NULL); 
		}
		
		switch(msg->action)
		{
			case 1:
				{
				
					msg->msg[0] = insert_chat_user(msg->from_name, msg->passwd, cfd, 0);//找此人
				
					msg->action = 1;

					send(cfd, msg, sizeof(struct message), 0);
					
					break;
				}
			
			case 2:
				{

					msg->msg[0] = insert_chat_user(msg->from_name, msg->passwd, cfd, 1);
				
					msg->action = 2;			
					send(cfd, msg, sizeof(struct message), 0); 
					
					break;
				}
			
			case 3:
				{
					to_cfd = GetId(msg->toname); 
					insert_sto_history(msg->from_name,msg->toname, msg->msg);
					msg->action = 3;
					send(to_cfd, msg, sizeof(struct message), 0);
					
					break;
				}
			
			case 4:
				{
					int i = 1;
					
					while(GetName(i))   //getname根据i找人名,
					{
						to_cfd = GetId(GetName(i));//找到人名，再根据名字找到sockfd
						msg->action = 4;
						send(to_cfd, msg, sizeof(struct message), 0);
						i++;
					}
					
					break;
				}
			
			// che   查看在线用户,直接输出各个在线用户姓名
			case 5:
				{
				
					int i = 1;
					
					while(GetName(i))
					{
						memset(msg->toname, 0, sizeof(msg->toname));
						strcpy(msg->toname, GetName(i));
						msg->action = 5;
						send(cfd, msg, sizeof(struct message), 0);
						i++;
					}
				
					break;
				}

	
			case 6:
			{
				Hlp();
				msg->action = 6;
				break;
			}
			

		
			case 7:
				{
					out(msg->from_name);	
					msg->action=8;
					send(cfd, msg, sizeof(struct message), 0);
					break;
				}


			
		}
		memset(msg, 0, sizeof(struct message));
	}
	pthread_exit(NULL);
}


int main()
{
	int sockfd;
	int cfd;
	int c_len;
	char buffer [1024];
	pthread_t id;
	
	struct sockaddr_in addr;
	struct sockaddr_in c_addr;
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd == -1)
	{
		perror("socket error !\n");
		exit(1);
	}
	printf("socket success !\n");
	
	int opt = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));  //设置与某个套接字关联的选项
	
	bzero(&addr, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	
	if(bind(sockfd, (struct sockaddr *)(&addr), sizeof(struct sockaddr_in)) < 0)
	{
		perror("bind error !\n");
		exit(1);
	}
	printf("bind success !\n");
	
	if(listen(sockfd, 3) < 0)
	{
		perror("listen error !\n");
		exit(1);
	}
	printf("listen success !\n");
	
	while(1)
	{
		memset(buffer, 0, sizeof(buffer));
		bzero(&c_addr, sizeof(struct sockaddr_in));
		c_len = sizeof(struct sockaddr_in);
		cfd = accept(sockfd, (struct sockaddr *)(&c_addr), &c_len);
		if(cfd == -1)
		{
			perror("accept error !\n");
			exit(1);
		}
		printf("accepting......\n");
        printf("port=%d   ip=%s\n",ntohs(c_addr.sin_port),inet_ntoa(c_addr.sin_addr));

		if(pthread_create(&id,NULL,recv_message,(void*)(&cfd))!=0) //创建线程从recv_message开始运行
        {
            perror("pthread create error\n");
            exit(1);
        }

	}
	return 0;
}

