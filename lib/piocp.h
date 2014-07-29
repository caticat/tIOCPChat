#pragma once

/*
	完成端口通信工具类
	注：发送接收均异步非阻塞，发送部分WSASend的异常处理没有做，数据量过多可能会有问题。
*/
/*
	// 完成端口测试
	PIOCP iocp;
	if (!iocp.Start(12345,callBackFunc))
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


	void callBackFunc(PER_SOCKET_CONTEXT* pPSock,char* data,uint16 dataLen)
	{
	PMsg pmsg(data,dataLen);
	int32 id = 0;
	string msg;
	pmsg>>msg>>id>>id;
	cout << "proto:" << pmsg.GetProto() << ",dataLen:" << dataLen << ",len:" << pmsg.GetDataLen() << ",data:" << msg << ",id:" << id << endl;
	pPSock->Send(data,dataLen);
	}
*/

/* 客户端例子 接收和发送数据
#include "pmsg.h"
#include <WinSock2.h>
#include <iostream>
#include <string>
//#include "tmp.h"

#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"PIOCP.lib")

using std::cout;
using std::endl;
using std::string;

void main()
{
WSAData wsaData;
WSAStartup(MAKEWORD(2,2),&wsaData);

SOCKET sock = socket(AF_INET,SOCK_STREAM,0);
if (sock == INVALID_SOCKET)
{
cout << "1" << endl;
return;
}

SOCKADDR_IN addr;
memset(&addr,0,sizeof(SOCKADDR_IN));
addr.sin_family = AF_INET;
addr.sin_addr.s_addr = inet_addr("127.0.0.1");
addr.sin_port = htons(12345);

if (connect(sock,(sockaddr*)&addr,sizeof(sockaddr)) == SOCKET_ERROR)
{
cout << "2" << endl;
return;
}

int32 id = 1001;
PMsg msg;
msg.SetProto(11);
msg<<"abcdefg"<<id;
const std::string& data = msg.GetData();
cout << "send:" << (int)send(sock,data.c_str(), data.length(),0) << endl;

// 主线程停止循环
//string cmd = "";
//while (true)
string cmd = "";
while (true)
{
{
char buf[256];
int ret = recv(sock,buf,256,0);
if (ret == SOCKET_ERROR)
{
cout << "socket error" << endl;
return;
}
PMsg msg2(buf,ret);
string aa;
int32 id2;
msg2>>aa>>id2;
cout << "buf:" << msg2.GetProto() << ",msg:" << aa << ",id:" << id2 << endl;
}

std::cin>>cmd;
if ((cmd == "quit") || (cmd == "exit")) // 退出程序
{
printf_s("关闭\n");
break;
}

cout << "send:" << (int)send(sock,data.c_str(), data.length(),0) << endl;
}

closesocket(sock);
WSACleanup();
cout << "end" << endl;
}
*/

#include <vector>
#include <string>

#include <WinSock2.h>
#include <MSWSock.h>
#pragma comment(lib,"ws2_32.lib")

#include "piocpdef.h"

#define PIOCP_MAX_BUFFER_LEN 8192 // 默认缓冲区长度 1024*8
#define PIOCP_DEFAULT_IP "127.0.0.1" // 默认IP
#define PIOCP_DEFAULT_PORT 12345 // 默认端口号

#define PIOCP_CHECK_POINTER(x,msg) {if(x==NULL){printf_s(msg);return;}} // 校验空指针
#define PIOCP_CHECK_POINTER_BOOL(x,msg) {if(x==NULL){printf_s(msg);return false;}} // 校验空指针 返回假
#define PIOCP_CHECK_BOOL_BOOL(x,msg) {if(!x){printf_s(msg);return false;}} // 校验bool是否为真，否则返回假

// 在完成端口上投递的I/O操作类型
enum PIOCP_OPERATION_TYPE
{
	NULL_POSTED, // 初始化，无意义
	ACCEPT_POSTED, // accept
	SEND_POSTED, // send
	RECV_POSTED, // receive
};

// 单IO数据结构体定义
struct PER_IO_CONTEXT
{
	OVERLAPPED overlapped; // 每一个重叠网络操作的重叠结构（针对每一个socket的每一个操作，都要有一个） 另：这个成员变量必须要定义在结构体的第一位，因为宏CONTAINING_RECORD要根据他来找到PER_IO_CONTEXT的指针地址
	SOCKET sock; // 这个网络操作所使用的socket
	WSABUF wsaBuf; // WSA类型的缓冲区，用于给重叠操作传参数的
	char cbuf[PIOCP_MAX_BUFFER_LEN]; // 这个是WSABUF里的具体字符的缓冲区
	uint16 nbufLen; // 接收的数据的长度
	PIOCP_OPERATION_TYPE opType; // 标识网络操作的类型（对应上面的枚举）

	inline PER_IO_CONTEXT(); // 初始化
	inline ~PER_IO_CONTEXT(); // 释放

	inline void ResetBuffer(); // 重置缓冲区内容
};

class PIOCP;
// 单句柄数据结构体定义（用于每一个完成端口，也就是每一个socket的参数）
struct DLL_API PER_SOCKET_CONTEXT
{
	typedef std::vector<PER_IO_CONTEXT*> ioContextArr_t;
	typedef ioContextArr_t::iterator itIoContextArr_t;
	template class DLL_API std::allocator<PER_IO_CONTEXT*>;
	template class DLL_API std::vector<PER_IO_CONTEXT*,std::allocator<PER_IO_CONTEXT*>>;

	SOCKET sock; // 每一个客户端连接的socket
	SOCKADDR_IN addr; // 客户端地址
	PIOCP* pPIOCP; // 主功能类指针
	ioContextArr_t ioContextArr; // 客户端网络操作上下文数据（对于每一个客户端socket，是可以在上面同时投递多个IO请求的）

	inline PER_SOCKET_CONTEXT();
	inline ~PER_SOCKET_CONTEXT();

	inline PER_IO_CONTEXT* GetNewIoContext(); // 获取一个新的IoContext
	inline void RemoveIoContext(PER_IO_CONTEXT* pIoContext); // 从数组中移除一个指定的IoContext
	inline void Send(const char* data,uint16 dataLen); // 发送数据
};

typedef PER_SOCKET_CONTEXT PSocket; // 方便调用

// 工作线程参数
struct THREADPARAMS_WORKER
{
	PIOCP* pPIOCP; // 主功能类指针
	uint32 nThreadNo; // 线程编号
};

class DLL_API PIOCP
{
public:
	PIOCP();
	~PIOCP();

private:
	typedef void(*callbackFunc_t)(PER_SOCKET_CONTEXT* pPSock,char* data,uint16 dataLen);
	typedef void(*onCloseFunc_t)(PER_SOCKET_CONTEXT* pPSock,uint16 closeType);

public:
	bool Start(uint16 port,callbackFunc_t callbackFunc,onCloseFunc_t onCloseFunc); // 启动网络模块
	void Stop(); // 停止网络模块
	std::string GetLocalIP(); // 获得本地IP地址
	void Send(PER_SOCKET_CONTEXT* pSocketContext,PER_IO_CONTEXT* pIoContext); // 发送数据

private:
	bool _LoadSocketLib(); // 加载socket库
	inline void _UnLoadSocketLib(); // 卸载socket库
	bool _InitializeIOCP(); // 初始化IOCP
	bool _InitializeListenSocket(); // 初始化监听socket
	void _DeInitialize(); // 释放资源
	void _SetPort(uint16 port); // 设置监听端口号
	void _SetCallbackFunction(callbackFunc_t callbackFunc); // 设置回调函数
	void _SetOnCloseFunction(onCloseFunc_t onCloseFunc); // 设置关闭回调函数
	bool _PostAccept(PER_IO_CONTEXT* pIoContext); // 投递accept请求
	bool _PostRecv(PER_IO_CONTEXT* pIoContext); // 投递recv请求
	bool _DoAccept(PER_SOCKET_CONTEXT* pSocketContext, PER_IO_CONTEXT* pIoContext); // 有客户端连入的处理
	bool _DoRecv(PER_SOCKET_CONTEXT* pSocketContext, PER_IO_CONTEXT* pIoContext); // 有数据到达时的处理
	bool _DoSend(PER_SOCKET_CONTEXT* pSocketContext, PER_IO_CONTEXT* pIoContext); // 发送数据到客户端
	bool _DoClose(PER_SOCKET_CONTEXT* pSocketContext, PIOCP_CLOSETYPE closeType); // 客户端退出
	void _AddSocketContext(PER_SOCKET_CONTEXT* pSocketContext); // 存储玩家连接信息
	void _RemoveSocketContext(PER_SOCKET_CONTEXT* pSocketContext); // 移除玩家连接信息
	void _ClearSocketContext(); // 清空客户端连接信息
	bool _AssociateWithIOCP(PER_SOCKET_CONTEXT* pSocketContext); // 将客户端连接绑定到完成端口中
	bool _HandleError(PER_SOCKET_CONTEXT* pSocketContext,DWORD dwError); // 错误处理
	static DWORD WINAPI _WorkerThread(LPVOID lpParam); // 线程函数，为IOCP请求服务的工作者线程
	uint16 _GetNoOfProcessor(); // 获得本机的处理器数量
	bool _IsSocketAlive(SOCKET s); // 判断客户端socket是否有效(是否断开)

private:
	typedef std::vector<PER_SOCKET_CONTEXT*> socketContextArr_t;
	typedef socketContextArr_t::iterator itSocketContextArr_t;
	template class DLL_API std::allocator<PER_SOCKET_CONTEXT*>;
	template class DLL_API std::vector<PER_SOCKET_CONTEXT*,std::allocator<PER_SOCKET_CONTEXT*>>;

private:
	HANDLE m_hShutdownEvent; // 用来通知线程退出的事件，为了更好的退出线程
	HANDLE m_hIOCompletionPort; // 完成端口句柄
	HANDLE* m_phWorkerThreads; // 工作者线程句柄指针
	uint8 m_nThreads; // 生成的线程数量
	std::string m_strIP; // 服务器端的IP地址
	uint16 m_nPort; // 监听端口
	CRITICAL_SECTION m_csSocketContextArr; // socketContextArr的互斥量
	socketContextArr_t m_socketContextArr; // 客户端连接列表
	PER_SOCKET_CONTEXT* m_pListenContext; // 监听socket的context信息
	LPFN_ACCEPTEX m_lpfnAcceptEx; // AcceptEx的函数指针
	LPFN_GETACCEPTEXSOCKADDRS m_lpfnGetAcceptExSockAddrs; // GetAcceptExSockAddrs的函数指针
	callbackFunc_t m_pfnCallBackFunc; // 回调函数指针
	onCloseFunc_t m_pfnOnCloseFunc; // 关闭回调函数指针
};
