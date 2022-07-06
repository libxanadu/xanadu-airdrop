#ifndef			_XANADU_AIRDROP_BROADCAST_H_
#define			_XANADU_AIRDROP_BROADCAST_H_

#include "header.h"

XANADU_CXX_EXTERN_BEGIN



/// device broadcast event
typedef enum airdrop_broadcast_event
{
	AIRDROP_BROADCAST_CONNECT	= 0,
	AIRDROP_BROADCAST_DISCONNECT	= 1
}airdrop_broadcast_event_t;

/// device broadcast device
typedef struct airdrop_broadcast_device
{
	char 				name[XANADU_PATH_MAX];
	x_ushort_t 			port;
	char 				ipv4[XANADU_PATH_MAX];
	char 				ipv6[XANADU_PATH_MAX];
	x_llong_t 			lu_time;			// Last update time
}airdrop_broadcast_device;

/// device broadcast callback
typedef void (*airdrop_broadcast_event_cb_t) (const airdrop_broadcast_device* _Device, airdrop_broadcast_event_t _Event, void* _UserData);

///  airdrop broadcast context
struct airdrop_broadcast_context;
typedef	struct airdrop_broadcast_context*		airdrop_broadcast_context_t;



/// Create broadcast context
_XAIRDROPAPI_ airdrop_error_t __xcall__ airdrop_broadcast_context_create(airdrop_broadcast_context_t* _Context);

/// Release broadcast context
_XAIRDROPAPI_ airdrop_error_t __xcall__ airdrop_broadcast_context_release(airdrop_broadcast_context_t _Context);



/// Set service port
_XAIRDROPAPI_ airdrop_error_t __xcall__ airdrop_broadcast_set_service_port(airdrop_broadcast_context_t _Context, x_ushort_t _Port);

/// Set the communication port
_XAIRDROPAPI_ airdrop_error_t __xcall__ airdrop_broadcast_set_broadcast_port(airdrop_broadcast_context_t _Context, x_ushort_t _Port);



/// start broadcast event
_XAIRDROPAPI_ airdrop_error_t __xcall__ airdrop_broadcast_event_start(airdrop_broadcast_context_t _Context, airdrop_broadcast_event_cb_t _Function, void* _UserData);

/// stop broadcast event
_XAIRDROPAPI_ airdrop_error_t __xcall__ airdrop_broadcast_event_stop(airdrop_broadcast_context_t _Context);



XANADU_CXX_EXTERN_END

#endif
