#pragma once

/*
	网络数据处理类
	支持类型：int8,int16,int32,int64,uint8,uint16,uint32,uint64,char*,string
	注：当读取数据长度超过实际数据长度后，会直接返回，不会处理数据
*/

/*
	// pmsg测试
	PMsg pmsg;
	cout << "proto:" << pmsg.GetProto() << ";len:" << pmsg.GetDataLen() << ";pos:" << pmsg.GetPos() << endl;
	pmsg.SetProto(1);
	cout << "proto:" << pmsg.GetProto() << ";len:" << pmsg.GetDataLen() << ";pos:" << pmsg.GetPos() << endl;
	int8 a = 1;
	pmsg.Write(a);
	int16 b = 2;
	int32 c = 3;
	pmsg<<b<<c<<"haha";
	cout << "proto:" << pmsg.GetProto() << ";len:" << pmsg.GetDataLen() << ";pos:" << pmsg.GetPos() << endl;
	
	int8 d = 0;
	int16 e = 0;
	int32 f = 0;
	string g = "";
	pmsg>>d>>e>>f>>g;
	cout << "proto:" << pmsg.GetProto() << ";len:" << pmsg.GetDataLen() << ";pos:" << pmsg.GetPos() << endl;
	cout << "d:" << (int)d << ",e:"<<e<<",f:"<<f<<",g:"<<g<<endl;
*/

#include <string>

#include "piocpdef.h"

class DLL_API PMsg
{
public:
	inline PMsg(); // 构造函数
	inline PMsg(const char* msg,int16 msgLen); // 设置数据

public:
	inline void SetProto(uint16 proto); // 设置协议号
	inline uint16 GetProto() const; // 获取协议号
	inline uint16 GetPos() const; // 获取当前读位置
	inline const std::string& GetData() const; // 获取所有数据
	inline uint16 GetDataLen() const; // 获取数据长度
	inline void ResetPos(); // 重置当前读取位置
	inline void Reset(); // 重置当前数据信息
	
	template <typename T>
	void Read(T& t); // 读取某个类型的数据
	template <typename T>
	PMsg& operator>>(T& t); // 读取某个类型的数据

	template <typename T>
	void Write(const T t); // 写入某个类型的数据
	inline void Write(const std::string& t); // 字符串特例化
	template <typename T>
	PMsg& operator<<(const T& t); // 写入某个类型的数据

	template <typename T>
	void Write(uint16 pos,const T t); // 在某个位置写入某个数据

private:
	void _WriteData(const void* pData,uint16 dataLen); // 写数据

private:
	std::string m_msg; // 数据
	uint16 m_pos; // 当前读取位置
	uint16 m_msgLen; // 当前数据长度

private:
	const static uint8 m_headLen = 4; // 协议头长度 协议头包括协议头外数据长度和协议号
	const static uint8 m_ProtoPos = 2; // 协议号起始位置
};

template <typename T>
void PMsg::Read(T& t)
{
	uint16 sizeT = sizeof(T);
	if (m_pos+sizeT>(uint16)m_msg.length())
		return;
	char* p = (char*)(m_msg.c_str()+m_pos);
	memcpy(&t,p,sizeT);
	m_pos += sizeT;
}

template <>
void PMsg::Read <std::string> (std::string& t) // 字符串读取特例化
{
	uint16 sLen = 0; // 字符串长度
	Read(sLen);
	if (sLen <= 0)
		return;
	if (m_pos + sLen > (uint16)m_msg.length())
		return;
	char* p = (char*)(m_msg.c_str()+m_pos);
	t.clear();
	t.insert(0,p,sLen);
	m_pos += sLen;
}

template <typename T>
PMsg& PMsg::operator>>(T& t)
{
	Read(t);
	return *this;
}

template <typename T>
void PMsg::Write(const T t)
{
	_WriteData(&t,sizeof(T));
}

template<>
void PMsg::Write <const char*> (const char* t) // 字符串写入特例化
{
	uint16 sLen = (uint16)strlen(t);
	Write(sLen);
	if (sLen > 0)
		_WriteData(t,sLen);
}

void PMsg::Write(const std::string& t) // 字符串写入特例化
{
	uint16 sLen = (uint16)t.length();
	Write(sLen);
	if (sLen > 0)
		_WriteData(t.c_str(),sLen);
}

template <typename T>
PMsg& PMsg::operator<<(const T& t)
{
	Write(t);
	return *this;
}

template <typename T>
void PMsg::Write(uint16 pos,const T t)
{
	uint16 sizeT = sizeof(T);
	if (pos+sizeT > m_msg.length())
		return;
	char* p = (char*)(m_msg.c_str()+m_pos);
	memcpy(p,&t,sizeT);
}

template <>
void PMsg::Write <const char*> (uint16 pos,const char* t)
{
	uint16 lenT = strlen(t);
	if (pos+lenT > (uint16)m_msg.length())
		return;
	char* p = (char*)(m_msg.c_str()+m_pos);
	memcpy(p,&t,lenT);
}
