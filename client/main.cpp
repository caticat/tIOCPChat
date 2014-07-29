#include <WinSock2.h>
#include <iostream>
#include <map>
#include <vector>
#include <string>
#include "pmsg.h"
#include "proto.h"

#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"piocp.lib")

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 12345
#define RECV_TIMEOUT 8000

#define RELEASE_SOCKET(x) {if(x!=INVALID_SOCKET){closesocket(x);x=INVALID_SOCKET;}} // 释放SOCKET
#define RELEASE_HANDLE(x) {if(x!=NULL && x!=INVALID_HANDLE_VALUE){CloseHandle(x);x=NULL;}} // 释放句柄

using std::cout;
using std::endl;
using std::map;
using std::vector;
using std::string;
using std::make_pair;

typedef void (*protoFunc_t)(SOCKET sock,PMsg& pmsg); // 逻辑处理函数指针
typedef map<int,protoFunc_t> protoMap_t; // 协议关联
typedef protoMap_t::iterator protoMapIt_t; // 协议关联迭代器

void InitializeProtoMap(); // 初始化协议表
static DWORD WINAPI _WorkerThread(LPVOID lpParma); // 工作者线程
bool ResolveCommand(string& command,string& cmd, string& data); // 解析命令
int TransformProto(string& cmd); // 命令转协议号
void ProtoLogin(SOCKET sock,PMsg& pmsg); // 登陆返回
void ProtoSay(SOCKET sock,PMsg& pmsg); // 对话返回
void ProtoOthLogin(SOCKET sock,PMsg& pmsg); // 其他角色登陆
void ProtoOthLogout(SOCKET sock,PMsg& pmsg); // 其他角色退出

protoMap_t protoMap; // 协议表
HANDLE hWorkerThread;
HANDLE hShutdownEvent;

int main()
{
	InitializeProtoMap();

	WSAData wsaData;
	if (WSAStartup(MAKEWORD(2,2),&wsaData) != NO_ERROR)
	{
		printf_s("wsastartup failed.\n");
		return 1;
	}

	SOCKET sock = socket(AF_INET,SOCK_STREAM,0);
	if (sock == INVALID_SOCKET)
	{
		printf_s("sock failed.\n");
		return 2;
	}

	SOCKADDR_IN addr;
	memset(&addr,0,sizeof(SOCKADDR_IN));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(SERVER_IP);
	addr.sin_port = htons(SERVER_PORT);

	if (connect(sock,(sockaddr*)&addr,sizeof(sockaddr)) == SOCKET_ERROR)
	{
		printf_s("connect failed.\n");
		return 3;
	}

	int timeout = RECV_TIMEOUT; // 超时设置
	int ret = setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout,sizeof(timeout));
	if (ret == SOCKET_ERROR)
	{
		printf_s("设置超时失败，错误码：%d\n",WSAGetLastError());
	}

	hShutdownEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
	DWORD dwThreadID;
	hWorkerThread = CreateThread(0,0,_WorkerThread,(void*)&sock,0,&dwThreadID);

	printf_s("客户端已经正常启动\n");

	char cscommand[255];
	string command;
	string cmd = "";
	string data = "";
	PMsg pmsg;
	while (true)
	{
		std::cin.getline(cscommand,sizeof(cscommand));
		command = cscommand;
		if ((command == "quit") || (command == "exit")) // 退出程序
		{
			printf_s("退出程序\n");
			SetEvent(hShutdownEvent);
			break;
		}
		
		if (!ResolveCommand(command,cmd,data)) continue;
		
		pmsg.Reset();
		pmsg.SetProto(TransformProto(cmd));
		pmsg<<data;
		if (send(sock,(char*)pmsg.GetData().c_str(),pmsg.GetData().length(),0) == SOCKET_ERROR)
		{
			printf_s("发送消息错误，错误码：%d\n",GetLastError());
			break;
		}
	}

	WaitForSingleObject(hWorkerThread,INFINITE);

	closesocket(sock);
	WSACleanup();

	RELEASE_HANDLE(hShutdownEvent);
	RELEASE_HANDLE(hWorkerThread);

	printf_s("客户端正常停止\n");
	return 0;
}

void InitializeProtoMap()
{
	protoMap.insert(make_pair(E_LOGIN_SC,ProtoLogin));
	protoMap.insert(make_pair(E_SAY_SC,ProtoSay));
	protoMap.insert(make_pair(E_OTHLOGIN_SC,ProtoOthLogin));
	protoMap.insert(make_pair(E_OTHLOGOUT_SC,ProtoOthLogout));
}

DWORD WINAPI _WorkerThread(LPVOID lpParma)
{
	SOCKET* pSock = (SOCKET*)lpParma;
	SOCKET sock = *pSock;
	int ret = 0;
	char buf[255];
	while(WAIT_OBJECT_0 != WaitForSingleObject(hShutdownEvent,0))
	{
		memset(buf,0,sizeof(buf));
		ret = recv(sock,buf,255,0);
		if (ret == SOCKET_ERROR)
		{
			if (WSAGetLastError() == WSAETIMEDOUT) // 超时的话
			{
				if (WAIT_OBJECT_0 != WaitForSingleObject(hShutdownEvent,0))
					continue;
				else // 主动关闭退出
					break;
			}
			else
				break;
		}
		PMsg pmsg(buf,ret);
		protoMapIt_t protoMapIt = protoMap.find(pmsg.GetProto());
		if (protoMapIt == protoMap.end())
		{
			printf_s("没有找到对应的协议解析：%d\n",pmsg.GetProto());
			continue;
		}
		protoMapIt->second(sock,pmsg);
	}
	printf_s("_WorkerThread线程结束\n");
	return 0;
}

bool ResolveCommand(string& command,string& cmd, string& data)
{
	int pos = command.find(" ");
	if (pos == command.npos)
	{
		cout << "command格式不正确" << endl;
		return false;
	}
	cmd = command.substr(0,pos);
	data = command.substr(pos+1,command.length()-pos-1);
	return true;
}

int TransformProto(string& cmd)
{
	if (cmd == "login")
		return E_LOGIN_CS;
	else if (cmd == "say")
		return E_SAY_CS;
	return E_INVALID;
}

void ProtoLogin(SOCKET sock,PMsg& pmsg)
{
	string log;
	pmsg>>log;
	printf_s("返回：%s\n",log.c_str());
}

void ProtoSay(SOCKET sock,PMsg& pmsg)
{
	string log;
	pmsg>>log;
	printf_s("%s\n",log.c_str());
}

void ProtoOthLogin(SOCKET sock,PMsg& pmsg)
{
	string log;
	pmsg>>log;
	printf_s("%s\n",log.c_str());
}

void ProtoOthLogout(SOCKET sock,PMsg& pmsg)
{
	string log;
	pmsg>>log;
	printf_s("%s\n",log.c_str());
}
