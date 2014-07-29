#pragma once

/*
	协议类型
*/

enum E_PROTO
{
	E_INVALID, // 无效协议
	E_LOGIN_CS, // 登陆 客户端至服务器
	E_LOGIN_SC, // 登陆 服务器至客户端
	E_SAY_CS, // 发言
	E_SAY_SC, // 发言
	E_OTHLOGIN_SC, // 其他玩家进入聊天室
	E_OTHLOGOUT_SC, // 其他玩家离开聊天室
};
