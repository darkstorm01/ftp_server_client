//18CS01031
//Sir, I have uploaded the assignment earlier, later the other functionalities are added and the submittes.

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/socket.h>
#include<sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include<pthread.h>


#define port "3010" //port at which the transfer happens
#define queue_size 20  //max queue for listening

#define maxusers 30

typedef struct node
{
	char data[100];
	char pwd[100];
	int count;
	struct node *next;
}node;

node *user=NULL;


node *insert(node * head,char data[],char pwd[])
{
	node *t=(node*) malloc(sizeof(node));
	strcpy(t->data,data);
	strcpy(t->pwd,pwd);
	t->count=1;
	t->next=head;
	
	return t;
}

node* check(node *head,char data[],char pwd[],int *isuser)
{
	node *t=head;
	while(t!=NULL)
	{
		if(strcmp(t->data,data)==0 && strcmp(t->pwd,pwd)==0)
		{
			if(t->count==1)
			{
				t->count=0;
				*isuser=1;
			}
			else
				*isuser=-1;
		}
		
		t=t->next;
	}
	return head;
}

node *update(node *head,char user_id[])
{
	node *t=head;
	while(t!=NULL)
	{
		if(strcmp(t->data,user_id)==0)
		{
			t->count=1;
		}
		t=t->next;
	}
	return head;
}

void print(node *head)
{
	node *t=head;
	while(t!=NULL)
	{
		printf("%s\t%s\t%d\n",t->data,t->pwd,t->count);
		t=t->next;
	}
}

int comp(char msg[])
{
	int n=strlen(msg);
	char need[4]={'e','x','i','t'};
	if(n<4)
		return 1;
	for(int i=0;i<4;i++)
	{
		if(need[i]!=msg[i])
		return 1;
	}
	return 0;
}

void tokenize(char *msg[],char *r_msg)
{
	char *delimit=" \n";
	msg[0]=strtok(r_msg,delimit);
	msg[1]=strtok(NULL,delimit);
}

int exists(char * filename){
    FILE *file;
    if (file = fopen(filename, "r")){
        fclose(file);
        return 1;
    }
    return 0;
}

void sendfile(int new_sockfd, char userid[])
{
	int mode=0;
	char mode_r[20],mode_w[20];
	while(1)
	{
		char r_msg[100];
		if(mode==1)
		{
			strcpy(mode_r,"rb");
			strcpy(mode_w,"wb");
		}
		else if(mode==0)
		{
			strcpy(mode_r,"r");
			strcpy(mode_w,"w");
		}
		printf("%s\t%s\n",mode_r,mode_w);
		bzero(r_msg,sizeof(r_msg));
		if(recv(new_sockfd,r_msg,100,0)==-1)
		{
			fprintf(stderr,"server: receiving error\n");
			//break;
		}
		printf("%s\n",r_msg);
		if(comp(r_msg)==0)
		{
			user=update(user,userid);
			FILE *ptr;
			ptr=fopen("log.txt","a");
			char data[100];
			strcpy(data,userid);
			strcat(data," has looged out.\n");
			fputs(data,ptr);
			fclose(ptr);
			break;
		}
		char *msg[2];
		tokenize(msg,r_msg);
		if(strcmp(msg[0],"mode")==0)
		{
			if(strcmp(msg[1],"bin")==0)
			{
				mode=1;
				FILE *ptr1;
				ptr1=fopen("log.txt","a");
				char data[100];
				strcpy(data,userid);
				strcat(data," has changed the mode to ");
				strcat(data,msg[1]);
				strcat(data,".\n");
				fputs(data,ptr1);
				fclose(ptr1);
			}
			else if(strcmp(msg[1],"char")==0)
			{
				mode=0;
				FILE *ptr1;
				ptr1=fopen("log.txt","a");
				char data[100];
				strcpy(data,userid);
				strcat(data," has changed the mode to ");
				strcat(data,msg[1]);
				strcat(data,".\n");
				fputs(data,ptr1);
				fclose(ptr1);
			}
		}
		else if(strcmp(msg[0],"get")==0)
		{
			FILE *ptr;
			char filepath[100];
			bzero(filepath,sizeof(filepath));
			strcpy(filepath,userid);
			strcat(filepath,"/");
			strcat(filepath,msg[1]);
			ptr=fopen(filepath,mode_r);
			if(ptr==NULL)
			{
				if(send(new_sockfd,"-20",3,0)==-1)
				{
					fprintf(stderr,"server:sending error\n");
					fclose(ptr);
					//continue;
				}
			}
			char s_msg[100];
			while(fgets(s_msg,100,ptr)!=NULL)
			{
				if(send(new_sockfd,s_msg,100,0)==-1)
				{
					fprintf(stderr,"server:sending error\n");
					fclose(ptr);
					break;
				}
			}
			//to end the file
			if(send(new_sockfd,"endi",100,0)==-1)
			{
				fprintf(stderr,"server:sending error\n");
				fclose(ptr);
				break;
			}
			fclose(ptr);
			FILE *ptr1;
			ptr1=fopen("log.txt","a");
			char data[100];
			strcpy(data,userid);
			strcat(data," has downloaded the file ");
			strcat(data,msg[1]);
			strcat(data,".\n");
			fputs(data,ptr1);
			fclose(ptr1);
		}
		else if(strcmp(msg[0],"post")==0)
		{
			char filepath[100];
			strcpy(filepath,userid);
			strcat(filepath,"/");
			strcat(filepath,msg[1]);
			FILE *ptr;
			if(exists(filepath))
			{
				char confirm[1];
				if(send(new_sockfd,"0",1,0)==-1)
				{
					fprintf(stderr,"Sending error\n");
					return ;
				}
				if(recv(new_sockfd,confirm,1,0)==-1)
				{
					fprintf(stderr,"Sending error\n");
					return ;					
				}
				if(confirm[0]=='n' || confirm[0]=='N')
					continue;
			}
			else
			{
				if(send(new_sockfd,"1",1,0)==-1)
				{
					fprintf(stderr,"Sending error\n");
					return ;
				}
			}
			ptr=fopen(filepath,mode_w);
			if(ptr==NULL)
			{
				printf("Cannot open the file\n");
				return ;
			}
			char r_msgf[100];
			while(1)
			{
				if(recv(new_sockfd,r_msgf,100,0)==-1)
				{
					fprintf(stderr,"server:receiving error\n");
					fclose(ptr);
					break;
				}
				if(strcmp(r_msgf,"endi")==0)
				{
					break;
				}
				fputs(r_msgf,ptr);
			}
			fclose(ptr);
			FILE *ptr1;
			ptr1=fopen("log.txt","a");
			char data[100];
			strcpy(data,userid);
			strcat(data," has uploaded the file ");
			strcat(data,msg[1]);
			strcat(data,".\n");
			fputs(data,ptr1);
			fclose(ptr1);
		}
	}
}


void connection(int new_sockfd)
{
	char userid[100];
	while(1)
	{
		char r_mg1[100];
		char r_mg2[100];
		bzero(r_mg1,sizeof(r_mg1));
		bzero(r_mg2,sizeof(r_mg2));
		//User authentication
		if(recv(new_sockfd,r_mg1,100,0)==-1)
		{
			fprintf(stderr,"server: receiving error\n");
			exit(1);
		}
		//printf("%s\t%ld\n",r_mg1,strlen(r_mg1));
		if(recv(new_sockfd,r_mg2,100,0)==-1)
		{
			fprintf(stderr,"server: receiving error\n");
			exit(1);
		}
		//printf("%s\t%ld\n",r_mg2,strlen(r_mg2));
		int isuser=0;
		int *ptr1=&isuser;
		user=check(user,r_mg1,r_mg2,ptr1);
		if(isuser==1)
		{
			if(send(new_sockfd,"1",1,0)==-1)
			{
				fprintf(stderr,"server: sending error\n");
				exit(1);
			}
			strcpy(userid,r_mg1);
			FILE *ptr;
			ptr=fopen("log.txt","a");
			char data[100];
			strcpy(data,userid);
			strcat(data," has looged in.\n");
			fputs(data,ptr);
			fclose(ptr);
			break;
		}
		else if(isuser==-1)
		{
			if(send(new_sockfd,"-1",1,0)==-1)
			{
				fprintf(stderr,"server: sending error\n");
				exit(1);
			}
		}
		else
		{
			if(send(new_sockfd,"0",1,0)==-1)
			{
				fprintf(stderr,"server: sending error\n");
				exit(1);
			}		
		}
	}
	sendfile(new_sockfd,userid);
}

//to create a new user in server
void *adduser()
{
	while(1)
	{
		char cmnd[100];
		scanf("%s",cmnd);
		if(strcmp(cmnd,"createuser")==0)
		{
			char user_id[100],pwd[100];
			printf("Enter username :");
			scanf("%s",user_id);
			printf("Enter password :");
			scanf("%s",pwd);
			int status=mkdir(user_id,0777);
			if(!status)
				printf("New user created successfully\n");
			else
			{
				printf("Unable to create user\n");
				continue;
			}
			user=insert(user,user_id,pwd);
			FILE *ptr;
			ptr=fopen("users.txt","a");
			char users[100];
			strcpy(users,user_id);
			strcat(users," ");
			strcat(users,pwd);
			strcat(users,"\n");
			fputs(users,ptr);
			fclose(ptr);
		}
	}
}

//Initialising the existing users
int initialise()
{
	FILE *ptr;
	char *filepath="users.txt";
	ptr=fopen(filepath,"r");
	char data[100];
	if(ptr==NULL)
		return 0;
	while(fgets(data,100,ptr)!=NULL)
	{
		char *msg[2];
		tokenize(msg,data);
		user=insert(user,msg[0],msg[1]);
	}
	fclose(ptr);
	return 1;
}

void *accept_conn(void *sock)
{
	int new_sockfd;
	char address[INET_ADDRSTRLEN];
	socklen_t addrlen;
	struct sockaddr_in connect_address;
	int sockfd=*((int *)sock);
	addrlen=sizeof connect_address;		
	new_sockfd=accept(sockfd,(struct sockaddr *)&connect_address,&addrlen);
	if(new_sockfd==-1)
	{
		fprintf(stderr,"server: Accepting error\n");
		exit(1);
	}
	inet_ntop(connect_address.sin_family,&(connect_address.sin_addr),address,INET_ADDRSTRLEN);
	printf("System connected to:%s\n",address);
	connection(new_sockfd);
	close(new_sockfd);
}

int main()
{
	//initialising the existing users in the users.txt file and storing them in linked list.
	initialise();
	print(user);
	//A thread is created which will be in while loop in order to create a user.
	//A user can be created at server side only.
	//If a user is active then he/she can't login from other terminal.
	/*pthread_t use;
	pthread_create(&use,NULL,&adduser,NULL);*/
	int sockfd,yes=1;	
	struct addrinfo hints, *res,*temp;
	memset(&hints,0,sizeof hints);
	hints.ai_flags=AI_PASSIVE;
	hints.ai_family=AF_INET;  // for IPv4 socket
	hints.ai_socktype=SOCK_STREAM;
	
	int status;
	if((status=getaddrinfo(NULL,port,&hints,&res))!=0)
	{
		fprintf(stderr,"getaddrinfo error:%s\n",gai_strerror(status));
		exit(1);
	}
	
	for(temp=res;temp!=NULL;temp=temp->ai_next)
	{
		if((sockfd=socket(temp->ai_family,temp->ai_socktype,temp->ai_protocol))==-1)
		{
			perror("server:socket");
			continue;
		}
		
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,sizeof(int)) == -1)
		{
			perror("setsockopt");
			exit(1);
		}
		
		if(bind(sockfd,temp->ai_addr,temp->ai_addrlen)==-1)
		{
			close(sockfd);
			perror("server: bind");
			continue;
		}
		break;
	}
	freeaddrinfo(res);
	if(temp==NULL)
	{
		fprintf(stderr,"server: failed to bind\n");
		exit(1);
	}
	
	if(listen(sockfd,queue_size)==-1)
	{
		fprintf(stderr,"server:Listening error\n");  
		exit(1);
	}
	printf("server waiting for connections:\n");
	/*creating maxusers no.of threads which will be waiting in accepting the connection from
	client*/
	pthread_t t[maxusers];
	
	//for(int i=0;i<maxusers;i++)
		pthread_create(&t[0],NULL,&accept_conn,(void *)&sockfd);

	//for(int i=0;i<maxusers;i++)
		pthread_join(t[0],NULL);
	
	//pthread_join(use,NULL);
	close(sockfd);
	return 0;
}
