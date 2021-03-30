#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include<pthread.h>


#define port "3010"

int mode=0;

void * get_addr(struct sockaddr * a)
{
	return &(((struct sockaddr_in*)a)->sin_addr);
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
	msg[0]=strtok(r_msg," ");
	msg[1]=strtok(NULL," ");
}

int check(char s_msg[])
{
	int count=0,index;
	for(int i=0;i<strlen(s_msg);i++)
	{
		if(s_msg[i]==' ')
		{
			count++;
			index=i;
		}
	}
	if(count!=1)
	return 0;
	if(index==strlen(s_msg)-1)
	return 0;
	return 1;
}

int exists(char * filename){
    FILE *file;
    if (file = fopen(filename, "r")){
        fclose(file);
        return 1;
    }
    return 0;
}

int sendfile(int sockfd)
{
	while(1)
	{	
		char mode_r[20];
		char mode_w[20];
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
		char s_msg[100];
		printf("Enter the command :\n>>>>");
		scanf(" %[^\n]",s_msg);
		if(comp(s_msg)==0)
		{
			if(send(sockfd,s_msg,100,0)==-1)
			{
				fprintf(stderr,"receiver: sending error\n");
				return 0;
			}
			return 0;
		}
		char cmnd[100];
		strcpy(cmnd,s_msg);
		char *msg[2];
		if(check(s_msg)==0)
		{
			printf("Invalid command\n");
			continue;
		}
		tokenize(msg,s_msg);
		if(strcmp(msg[0],"get")==0 && strlen(msg[1])>0)
		{
			if(exists(msg[1]))
			{
				char confirm;
				printf("A file already exists with the same name\nDo you want to overwrite it (y/n) :");
				scanf(" %c",&confirm);
				if(confirm=='n' || confirm=='N')
					continue;
			}
			if(send(sockfd,cmnd,100,0)==-1)
			{
				fprintf(stderr,"receiver: sending error\n");
				return 0;
			}
			char r_msg[100];
			if(recv(sockfd,r_msg,100,0)==-1)
			{
				fprintf(stderr,"client:receiving error\n");
				break;
			}
			if(strcmp(r_msg,"-20")==0)
			{
				printf("File doesnot exist\n");
				continue;
			}

			FILE *ptr;
			ptr=fopen(msg[1],mode_w);
			if(ptr==NULL)
			{
				printf("Cannot open the file\n");
				fclose(ptr);
				continue;
			}
			fprintf(ptr,"%s",r_msg);
			if(strcmp(r_msg,"endi")==0)
				continue;
			while(1)
			{
				if(recv(sockfd,r_msg,100,0)==-1)
				{
					fprintf(stderr,"client:receiving error\n");
					break;
				}
				if(strcmp(r_msg,"-20")==0)
				{
					printf("File doesnot exist\n");
					break;
				}
				if(strcmp(r_msg,"endi")==0)
				{
					break;
				}
				fprintf(ptr,"%s",r_msg);
			}
			fclose(ptr);
		}
		else if(strcmp(msg[0],"post")==0 && strlen(msg[1])>0)
		{
			FILE *ptr;
			ptr=fopen(msg[1],mode_r);
			if(ptr==NULL)
			{
				printf("Error in opening the file\n");
				continue;
			}
			if(send(sockfd,cmnd,100,0)==-1)
			{
				fprintf(stderr,"receiver: sending error\n");
				return 0;
			}
			char data[1];
			if(recv(sockfd,data,1,0)==-1)
			{
				fprintf(stderr,"receiver: receiving error\n");
				return 0;				
			}
			if(data[0]=='0')
			{
				printf("A file already exists with the same name\nDo you want to overwrite it (y/n) :");
				char confirm[1];
				scanf("%s",confirm);
				if(send(sockfd,confirm,1,0)==-1)
				{
					fprintf(stderr,"receiver: sending error\n");
					return 0;
				}
				if(confirm[0]=='n' || confirm[0]=='N')
				{
					fclose(ptr);
					continue;
				}
			}
			char s_fmsg[100];
			while(fgets(s_fmsg,100,ptr)!=NULL)
			{
				if(send(sockfd,s_fmsg,100,0)==-1)
				{
					fprintf(stderr,"server:sending error\n");
					fclose(ptr);
					return 0;
				}
			}
			//to end the file
			if(send(sockfd,"endi",100,0)==-1)
			{
				fprintf(stderr,"server:sending error\n");
				fclose(ptr);
			}
			fclose(ptr);
		}
		else if(strcmp(msg[0],"mode")==0)
		{
			int flag=0;
			if(strcmp(msg[1],"bin")==0)
			{
				mode=1;
				flag=1;
			}
			else if(strcmp(msg[1],"char")==0)
			{
				mode=0;
				flag=1;
			}
			else
				printf("Invalid command\n");
			if(flag==1)
			{
				if(send(sockfd,cmnd,100,0)==-1)
				{
					fprintf(stderr,"receiver: sending error\n");
					return 0;
				}
			}
		}
		else
		{
			printf("Invalid command\n");
		}
	}
}

int main(int argc,char *argv[])
{
	int sockfd;
	socklen_t addrlen;
	
	char address[INET_ADDRSTRLEN];
	
	struct addrinfo hints, *res,*temp;
	memset(&hints,0,sizeof hints);
	hints.ai_family=AF_INET;  // for IPv4 socket
	hints.ai_socktype=SOCK_STREAM;
	hints.ai_flags=AI_PASSIVE;
	
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
			perror("client:socket");
			continue;
		}
		
		if(connect(sockfd,temp->ai_addr,temp->ai_addrlen)==-1)
		{
			perror("client:connect");
			close(sockfd);
			continue;
		}
		break;
	}	
	
	
	if(temp==NULL)
	{
		fprintf(stderr,"client: failed to connect\n");
		exit(1);
	}

	inet_ntop(temp->ai_family,get_addr(temp->ai_addr),address,INET_ADDRSTRLEN);
		
	printf("System connecting to:%s\n",address);
	freeaddrinfo(res);
	char flag='y';
	int acc=0;
	while(flag=='y')
	{
		char r_msg[100];
		bzero(r_msg,sizeof(r_msg));
		char user_id[100];
		char pwd[100];
		printf("Enter username :");
		scanf("%s",user_id);
		if(send(sockfd,user_id,strlen(user_id),0)==-1)
		{
			fprintf(stderr,"client: sending error\n");
			exit(1);
		}
		printf("Enter password :");
		scanf("%s",pwd);
		if(send(sockfd,pwd,strlen(pwd),0)==-1)
		{
			fprintf(stderr,"client: sending error\n");
			exit(1);
		}
		if(recv(sockfd,r_msg,100,0)==-1)
		{
			fprintf(stderr,"client: receiving error\n");
			exit(1);
		}
		if(strcmp(r_msg,"1")==0)
		{
			printf("Success\n");
			acc=1;
			printf("\t\t*****post <filename> to send the file*****\n");
			printf("\t\t*****get <filename> to request for file*****\n");
			printf("\t\t*****mode <mode> to change the mode*\n");
			printf("\t\t*****Default mode is ascii*****\n");
			break;
		}
		else if(strcmp(r_msg,"0")==0)
			printf("Invalid username/password\n");
		else
			printf("User is active\n");
			
		printf("Do you want to try again (y/n):");
		scanf(" %c",&flag);
	}
	if(acc==1)
		sendfile(sockfd);
	close(sockfd);
	return 0;
}
