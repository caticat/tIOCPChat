#include <iostream>
#include <map>
#include <vector>
#include <string>
#include "piocp.h"
#include "pmsg.h"
#include "proto.h"

#pragma comment(lib,"piocp.lib")

#define SERVER_PORT 12345

using std::cout;
using std::endl;
using std::map;
using std::vector;
using std::string;
using std::make_pair;

typedef void (*protoFunc_t)(PSocket* pPSock,PMsg& pmsg); // 逻辑处理函数指针
typedef map<int,protoFunc_t> protoMap_t; // 协议关联
typedef protoMap_t::iterator protoMapIt_t; // 协议关联迭代器
struct User;
typedef map<PSocket*,User> sockMap_t; // 用户关联
typedef sockMap_t::iterator sockMapIt_t; // 用户关联迭代器

void InitializeProtoMap(protoMap_t& protoMap);
void callBackFunc(PSocket* pPSock,char* data,uint16 dataLen);
void onCloseFunc(PSocket* pPSock,uint16 closeType);
void sendToAll(PMsg& pmsg);

void ProtoLogin(PSocket* pPSock,PMsg& pmsg);
void ProtoSay(PSocket* pPSock,PMsg& pmsg);

struct User
{
	string name;
	PSocket* pPSock;

	User(string n,PSocket* pPS) : name(n),pPSock(pPS) {}
};

protoMap_t protoMap; // 协议表
sockMap_t sockMap; // 用户表

int main()
{
	InitializeProtoMap(protoMap);

	// 完成端口测试
	PIOCP iocp;
	if (!iocp.Start(SERVER_PORT,callBackFunc,onCloseFunc))
	{
		printf_s("服务器启动失败！\n");
		return 2;
	}

	// 主线程停止循环
	string cmd = "";
	while (true)
	{
		std::cin>>cmd;
		if ((cmd == "quit") || (cmd == "exit")) // 退出程序
		{
			iocp.Stop(); // 关闭IOCP监听服务
			printf_s("服务器关闭\n");
			break;
		}
	}

	return 0;
}

void InitializeProtoMap(protoMap_t& protoMap)
{
	protoMap.insert(make_pair(E_LOGIN_CS,ProtoLogin));
	protoMap.insert(make_pair(E_SAY_CS,ProtoSay));
}

void callBackFunc(PSocket* pPSock,char* data,uint16 dataLen)
{
	PMsg pmsg(data,dataLen);

	protoMapIt_t protoMapIt = protoMap.find(pmsg.GetProto());
	if (protoMapIt == protoMap.end())
	{
		printf_s("没有找到协议号:%d\n",pmsg.GetProto());
		return;
	}
	protoMapIt->second(pPSock,pmsg);

	/*
	int32 id = 0;
	string msg;
	pmsg>>msg>>id>>id;
	cout << "proto:" << pmsg.GetProto() << ",dataLen:" << dataLen << ",len:" << pmsg.GetDataLen() << ",data:" << msg << ",id:" << id << endl;
	pPSock->Send(data,dataLen);
	*/
}

void onCloseFunc(PSocket* pPSock,uint16 closeType)
{
	sockMapIt_t sockMapIt = sockMap.find(pPSock);
	if (sockMapIt == sockMap.end())
		return;
	string data = sockMapIt->second.name;
	sockMap.erase(sockMapIt);
	printf_s("删除客户端数据完成，当前在线链接数：%d\n",sockMap.size());

	data = data+"离开聊天室";
	PMsg pmsg;
	pmsg.SetProto(E_OTHLOGOUT_SC);
	pmsg<<data;
	sendToAll(pmsg);
}

void sendToAll(PMsg& pmsg)
{
	string data = pmsg.GetData();
	int dataLen = data.length();
	for (sockMapIt_t it = sockMap.begin(); it != sockMap.end(); ++it)
	{
		it->first->Send(data.c_str(),dataLen);
	}
}

void ProtoLogin(PSocket* pPSock,PMsg& pmsg)
{
	string name;
	pmsg>>name;
	pmsg.Reset();
	sockMapIt_t sockMapIt = sockMap.find(pPSock);
	if (sockMapIt != sockMap.end()) // 已经登陆过了
	{
		pmsg.SetProto(E_LOGIN_SC);
		pmsg<<"不要重复登陆";
	}
	else
	{
		// 通知其他玩家
		string othData = name+"进入聊天室";
		PMsg othMsg;
		othMsg.SetProto(E_OTHLOGIN_SC);
		othMsg<<othData;
		sendToAll(othMsg);

		// 通知自己
		sockMap.insert(make_pair(pPSock,User(name,pPSock)));
		pmsg.SetProto(E_LOGIN_SC);
		pmsg<<"登陆成功";
	}
	pPSock->Send(pmsg.GetData().c_str(),pmsg.GetData().length());
}

void ProtoSay(PSocket* pPSock,PMsg& pmsg)
{
	sockMapIt_t sockMapIt = sockMap.find(pPSock);
	if (sockMapIt == sockMap.end())
	{
		printf_s("没有找到这个用户\n");
		return;
	}
	User& user = sockMapIt->second;
	string say;
	pmsg>>say;
	say = user.name+"说："+say;
	pmsg.Reset();
	pmsg.SetProto(E_SAY_SC);
	pmsg<<say.c_str();
	sendToAll(pmsg);
}
