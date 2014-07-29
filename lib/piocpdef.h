#pragma once

#ifdef DLL_EXPORT
#define DLL_API __declspec(dllexport)
#else
#define DLL_API __declspec(dllimport)
#endif // DLL_EXPORT

#define RELEASE(x) {if(x!=NULL){delete x;x=NULL;}} // 释放指针
#define RELEASE_ARR(x) {if(x!=NULL){delete[] x;x=NULL;}} // 释放指针数组
#define RELEASE_HANDLE(x) {if(x!=NULL && x!=INVALID_HANDLE_VALUE){CloseHandle(x);x=NULL;}} // 释放句柄
#define RELEASE_SOCKET(x) {if(x!=INVALID_SOCKET){closesocket(x);x=INVALID_SOCKET;}} // 释放SOCKET

#define WORKER_THREAD_PER_PROCESSOR 2 // 每个处理器上产生多少个线程
#define MAX_POST_ACCEPT 10 // 同时投递的Accept请求数量（这个要根据实际情况灵活设置）
#define EXIT_CODE NULL // 传递给Worker线程的退出信号

typedef char int8;
typedef short int16;
typedef int int32;
typedef long long int64;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;

enum PIOCP_CLOSETYPE
{
	CLOSETYPE_NORMAL, // 标准退出
	CLOSETYPE_ERROR, // 错误退出
};
