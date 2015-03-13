#include <iostream.h>
#include <stdlib.h>
#include <errno.h>
#include <winsock2.h> 
#include <io.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#pragma comment(lib, "ws2_32.lib")

//宏定义

typedef	struct{
	SOCKET	s1;
	SOCKET	s2;
		}Stu_sock;

//函数声明
void version();
int param(int argc,char **argv);		//对命令行参数
void Funlisten(int port1, int port2);	//监听功能函数
bool bindAndFunlisten(SOCKET s,int port);//绑定socket的地址结构
void datatrans(LPVOID data);			//数据转发函数
void slave(char* hostIp,char * slaveIp,int destionPort,int slavePort);//slave函数
bool checkIP(char * str);
int client_connect(int sockfd,char* server,int port);//连接服务端

void main(int argc,char **argv)
{
	
	version();
	//参数判断
	WSADATA wsadata;
    WSAStartup(MAKEWORD(1, 1), &wsadata);
	char hostIp[20]={0};
	char slaveIp[20]={0};
	int ret=1;
	ret = param(argc,argv);
	if (ret==1)//Funlisten
	{
	Funlisten(atoi(argv[2]),atoi(argv[3]));
	}else if (ret == 2)
	{
		strcpy(hostIp,argv[2]);
		strcpy(slaveIp,argv[4]);
	slave(argv[2],argv[4],atoi(argv[3]),atoi(argv[5]));
	}
	
}

int param(int argc,char **argv)
{
	//stricmp==strcmp ignore case忽略大小写
	if (argc==4&&stricmp(argv[1],"-listen")==0)
	{
//		cout<<"Funlisten"<<endl;
		return 1;
	}
	if (argc==6&&stricmp(argv[1],"-slave")==0&&checkIP(argv[2])&&checkIP(argv[4]))
	{
		return 2;
	}else
	{
			version();
			return -1;
	}
}
/************************************************************************/
/*                                                                      */
/*				listen 功能模块											*/
/************************************************************************/

void Funlisten(int port1,int port2)
{
	Stu_sock	stu_sock;
	
	//创建套接字
	SOCKET sock1=socket(AF_INET,SOCK_STREAM,0);
	SOCKET sock2=socket(AF_INET,SOCK_STREAM,0);
	if (sock1<0||sock1<0)
	{
		cout<<"[-] Create socket error"<<endl;
		return;
	}

	//绑定端口到socket并监听
	if (!bindAndFunlisten(sock1,port1)||!bindAndFunlisten(sock2,port2))
	{
		return;
	}
	//都监听好了接下来……


	int SizeOfAddr=sizeof(sockaddr);
	while (true)
	{
		cout<<"[+] Waiting for Client ......"<<endl;
		sockaddr_in	remoteSockAddr;
		//sock1等待连接
		SOCKET	recvSock1=accept(sock1,(sockaddr *)&remoteSockAddr,&SizeOfAddr);
		cout<<recvSock1<<endl;
		if (recvSock1<0)
		{
			cout<<"[-] Accept error."<<endl;
			continue;
		}
		cout<<"[+] Accept a Client on port "<<port1<<"  from "<<inet_ntoa(remoteSockAddr.sin_addr)<<endl;
		cout<<"[+] Waiting another Client on port:"<<port2<<"...."<<endl;
			
		SOCKET	recvSock2=accept(sock2,(sockaddr *)&remoteSockAddr,&SizeOfAddr);
		cout<<recvSock2<<endl;
		if (recvSock2<0)
		{
			cout<<"[-] Accept error."<<endl;
			continue;
		}
		cout<<"[+] Accept a Client on port"<<port1<<"  from "<<inet_ntoa(remoteSockAddr.sin_addr)<<endl;

		//两个都连上来了
		cout<<"[+] Accept Connect OK!"<<endl;
		
		
		stu_sock.s1=recvSock1;		stu_sock.s2=recvSock2;
		DWORD	dwThreadID;
		
		//创建一个转发数据的线程
		HANDLE	hThread=CreateThread(0,0,(LPTHREAD_START_ROUTINE)datatrans,(LPVOID)&stu_sock,0,&dwThreadID);
		if (hThread==NULL)
		{
			TerminateThread(hThread,0);
			return;//线程错了直接退出
		}
		cout<<"[+] CreateThread OK!"<<endl;
		Sleep(800);//挂起当前线程
		
		//
	}



}

bool bindAndFunlisten(SOCKET s,int port)
{
	//地址结构
	sockaddr_in	addr;
	memset(&addr,0,sizeof(addr));
	addr.sin_port=htons(port);
	addr.sin_family=AF_INET;
	addr.sin_addr.s_addr=htonl(INADDR_ANY);

	char on =1;
	setsockopt(s,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));

	//绑定地址结构
	if (bind(s,(const sockaddr *)&addr,sizeof(sockaddr))<0)
	{
		cout<<"[-] Socket bind error."<<endl;
		return false;
	}
	
	//监听端口
	if (listen(s,5)<0)
	{
		cout<<"[-] Listen error."<<endl;
		return false;
	}
	return true;
}

void datatrans(LPVOID data)
{
	char host_slave[20]={0};
	char host_hacker[20]={0};
	Stu_sock *stuSock=(Stu_sock *)data;
	SOCKET	s1=stuSock->s1;	//接受的是slave的数据
	SOCKET	s2=stuSock->s2;	//发送出去的socket
	int	sentacount1=0;
//	cout<<stuSock->s1<<endl;
//	cout<<stuSock->s2<<endl;


	sockaddr_in	addr={0};
	int sizeofaddr=sizeof(sockaddr);
	
	if (getpeername(s1,(sockaddr *)&addr,&sizeofaddr))
	{

		strcpy(host_slave,inet_ntoa(addr.sin_addr));
		int port_slave=ntohs(addr.sin_port);
	}
	if (getpeername(s2,(sockaddr *)&addr,&sizeofaddr))
	{

		strcpy(host_hacker,inet_ntoa(addr.sin_addr));
		int port_hacker=ntohs(addr.sin_port);
	}
	cout<<"[+] Start Transport ("<<host_slave<<"<-> "<<host_hacker<<") ......"<<endl;
	char RecvBuffer[20480];
	char SendBuffer[20480];

	
	fd_set	readfd;fd_set	writefd;
	timeval	timeset;
	timeset.tv_sec=300;
	timeset.tv_usec=0;
	int maxfd=max(s1,s2)+1;
		bool flag=false;
		int readsize;
	while (TRUE)
	{
		
	
		readsize=0;
		FD_ZERO(&readfd);
		FD_ZERO(&writefd);
		FD_SET((UINT)s1,&readfd);
		FD_SET((UINT)s2,&readfd);
		FD_SET((UINT)s1,&writefd);
		FD_SET((UINT)s2,&writefd);


		int result=select(maxfd,&readfd,&writefd,NULL,&timeset);

		if (result<0 && (errno!=EINTR))		
		{
			cout<<"[-] Select error."<<endl;
			break;
		}
		else if (result==0)
		{
			cout<<"[-] Socket time out."<<endl;
			break;
		}

		//没出错，没超时
		if (FD_ISSET(s1,&readfd)&&flag)///////////////////////////////////1
		{
	//		if (totalread1<20408)
			{
				 readsize=recv(s1, RecvBuffer, 20480, 0);//接受host的请求。。
				 if (readsize==-1)
				 {
					 break;
				 }
				 if (readsize==SOCKET_ERROR||readsize==0)
				 {
					 cout<<"!!!"<<endl;
				 }
				 memcpy(SendBuffer,RecvBuffer,readsize);
				 memset(RecvBuffer,0,20480);
				cout<<" [+] Recv "<<readsize<<" bytes "<<"from host."<<endl;
			}
		}


		if (FD_ISSET(s1,&writefd)&&flag&&(readsize>0))///////////////////////////////////2
		{

			int sendsize=send(s2,SendBuffer,readsize,0);//发给slave
			if (sendsize==0)
			{
				break;
			}
			if (sendsize<0 && (errno!=EINTR))
			{
				cout<<"[-] Send to s2 unknow error."<<endl;
				break;
			}
			
			memset(SendBuffer,0,20480);
			cout<<" Send "<<sendsize<<" bytes "<<endl;
						
		}
		if (FD_ISSET(s2,&readfd)&&(!flag))///////////////////////////////////3
		{
			{
				readsize=recv(s2, RecvBuffer, 20480, 0);//接受slave返回数据
				
				memcpy(SendBuffer,RecvBuffer,readsize);
				cout<<" [+] Recv "<<readsize<<" bytes "<<"from host."<<endl;
				//totalread1+=readsize;
				memset(RecvBuffer,0,20480);
			}
		}
		if (FD_ISSET(s1,&writefd)&&(!flag)&&(readsize>0))///////////////////////////////////4
		{
			readsize=send(s1,SendBuffer,readsize,0);//发给host
			if (readsize==0)
			{
				break;
			}
			if (readsize<0)
			{
				cout<<"[-] Send to s2 unknow error."<<endl;
				break;
			}
			cout<<" Send "<<readsize<<" bytes "<<endl;
			memset(SendBuffer,0,20480);
					
		}

		flag=!flag;

		Sleep(5);


	}
	closesocket(s1);
	closesocket(s2);
	cout<<"[+] OK! I Closed The Two Socket."<<endl;
}

/************************************************************************/
/*                                                                      */
/*				slave 功能模块											*/
/************************************************************************/
void slave(char* hostIp,char * slaveIp,int destionPort,int slavePort)
{
	
	//checkIP(hostIp);
		Stu_sock	stu_sock;
		fd_set		fd;
		char		buffer[20480];
		int			l;
	while(TRUE)
	{
		//创建套接字
		SOCKET sock1=socket(AF_INET,SOCK_STREAM,0);
		SOCKET sock2=socket(AF_INET,SOCK_STREAM,0);

		cout<<"[+] Make a Connection to "<<hostIp<<"on port:"<<slaveIp<<"...."<<endl;
		if (sock1<0||sock1<0)
		{
			cout<<"[-] Create socket error"<<endl;
			return;
		}
		//
		fflush(stdout);
		if(client_connect(sock1,hostIp,destionPort)==0) 
		{
			closesocket(sock1);
			closesocket(sock2);
			continue;/*跳过这次循环*/
           }
		
	memset(buffer,0,20480);
	while(1)
	{
		//把sock清零,加入set集
		FD_ZERO(&fd);
		FD_SET(sock1, &fd);
		
		//select事件	读 写 异常
		if (select(sock1+1, &fd, NULL, NULL, NULL) == SOCKET_ERROR) 
		{
			//不懂
			if (errno == WSAEINTR) continue;
			break;
		}
		//FD_ISSET返回值>0 表示SET里的可读写
		if (FD_ISSET(sock1, &fd)) 
		{
			l=recv(sock1, buffer, 20480, 0);
			break;
		}
		Sleep(5);
     }
		
	if(l<=0) 
	{      
		cout<<"[-] There is a error...Create a new connection."<<endl;
		continue;
    }
	while(1)
	{
	cout<<"[+] Connect OK!    \n[+] xlcTeam congratulations!"<<endl;
	cout<<"[+] Make a Connection to "<<hostIp<<"on port:"<<slaveIp<<"...."<<endl;
		fflush(stdout);
		if(client_connect(sock2,slaveIp,slavePort)==0) 
		{
			closesocket(sock1);
			closesocket(sock2);
			continue;
		}
		
		if(send(sock2,buffer,l,0)==SOCKET_ERROR)
		{      
			cout<<"[-] Send failed."<<endl;
			continue;
		}
		
		l=0;
		memset(buffer,0,20480);
		break;
    }

	cout<<"[+] All Connect OK!"<<endl;
	
	stu_sock.s1 = sock1;
	stu_sock.s2 = sock2;
	
	HANDLE	hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)datatrans, (LPVOID)&stu_sock, 0, NULL); 
	if(hThread == NULL) 
	{
		TerminateThread(hThread, 0);
		return;
	}

	}
}

//检查IP地址格式是否正确
bool checkIP(char * str)
{
	if(INADDR_NONE == inet_addr(str))
		return FALSE;
	return true;
}

int client_connect(int sockfd,char* server,int port)
{					/*sock*/		/*远程IP*/	/*远程端口*/
	
	//声明
	struct sockaddr_in cliaddr;	
	struct hostent *host;
	
	if(!(host=gethostbyname(server)))	//获得远程主机的IP
	{
     //   printf("[-] Gethostbyname(%s) error:%s\n",server,strerror(errno));
        return(0);
	}      
	//给地址结构赋值
	memset(&cliaddr, 0, sizeof(struct sockaddr));
	cliaddr.sin_family=AF_INET;
	cliaddr.sin_port=htons(port);/*远程端口*/
	cliaddr.sin_addr=*((struct in_addr *)host->h_addr);//host ip
	
	//去连接远程正在listen的主机
	if(connect(sockfd,(struct sockaddr *)&cliaddr,sizeof(struct sockaddr))<0)
	{
//        printf("[-] Connect error.\r\n");
        return(0);
	}
	return(1);
}

void version()
	
{	cout<<"xlc v1.0 -Port Transport by Chris "<<endl;
	cout<<"      _     _____                    "<<endl;
	cout<<"__  _| | __|_   _|__  __ _ _ __ ___  "<<endl;
	cout<<"\\ \\/ / |/ __|| |/ _ \\/ _` | '_ ` _ \\ "<<endl;
	cout<<" >  <| | (__ | |  __/ (_| | | | | | |"<<endl;
	cout<<"/_/\\_\\_|\\___||_|\\___|\\__,_|_| |_| |_|"<<endl;
	cout<<endl;
	cout<<"site:http://sec.xlcteam.com"<<endl<<endl;
	cout<<"usage: xlc.exe [options]"<<endl;
	cout<<endl;
	cout<<"options:"<<endl;
	cout<<"-slave  remoteIp remotePort1  localIp  localPort"<<endl;
	cout<<"-listen  remotePort1 remotePort2"<<endl;
	cout<<endl;
	cout<<"e.g.:"<<endl;
	cout<<"xlc.exe -slave  202.119.225.35 51 192.168.0.11 3389"<<endl;
	cout<<"xlc.exe -listen 51  33891"<<endl;
	cout<<endl;
}
