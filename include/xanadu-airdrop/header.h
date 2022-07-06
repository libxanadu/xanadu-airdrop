#ifndef			_XANADU_AIRDROP_HEADER_H_
#define			_XANADU_AIRDROP_HEADER_H_

#include <xanadu-posix/xanadu.h>
#include <xanadu-cctl/xanadu.h>



// xanadu airdrop 导出宏
#if defined(XANADU_AIRDROP_BUILD_STATIC)
#define			_XAIRDROPAPI_
#else
#if defined(XANADU_AIRDROP_BUILD_LOCAL)
#define			_XAIRDROPAPI_					XANADU_COMPILER_API_EXP
#else
#define			_XAIRDROPAPI_					XANADU_COMPILER_API_IMP
#endif
#endif



// 宏定义
#define 		AIRDROP_LOG_TAG					("X-Airdrop")

/// 广播休息时间
#define AIRDROP_BROADCAST_SOCKET_MSLEEP		(100)
#define AIRDROP_BROADCAST_DISCONNECT_TIME	(500)

// 服务默认端口
#define AIRDROP_SERVICE_DEFAULT_PORT		(8059)

// 广播默认端口
#define AIRDROP_BROADCAST_DEFAULT_PORT		(8060)


/// airdrop 错误列表
typedef enum airdrop_error
{
	AIRDROP_E_SUCCESS			= 0,
	AIRDROP_E_INVALID_ARG			= 1,
	AIRDROP_E_NOT_ENOUGH_MEMORY		= 2,
	AIRDROP_E_DEVICE_NOT_EXIST		= 3,
	AIRDROP_E_SEND_TIMEOUT			= 4,
	AIRDROP_E_RECEIVE_TIMEOUT		= 5,
	AIRDROP_E_UNKNOWN_ERROR			= 255
}airdrop_error_t;


#endif
