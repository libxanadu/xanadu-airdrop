#ifndef			_XANADU_AIRDROP_AIRDROP_H_
#define			_XANADU_AIRDROP_AIRDROP_H_

#include <xanadu-airdrop/header.h>

XANADU_CXX_EXTERN_BEGIN


/// airdrop device event
typedef enum airdrop_device_event
{
	AIRDROP_DEVICE_CONNECT		= 0,
	AIRDROP_DEVICE_DISCONNECT	= 1
}airdrop_device_event_t;


/// airdrop device identity
typedef uint64_t			airdrop_device_id;
#define AIRDROP_INVALID_DEVICE		(0)

/// airdrop device info
typedef struct airdrop_device_info
{
	airdrop_device_id		id;
	char 				ipv4[16];
	unsigned short			port;
	char 				name[XANADU_PATH_MAX];
}airdrop_device_info;

///  airdrop context
struct airdrop_context;
typedef	struct airdrop_context*		airdrop_context_t;

/// airdrop device event callback
typedef void (*airdrop_device_event_cb_t) (const airdrop_device_info* _Device, airdrop_device_event_t _Event, void* _UserData);

/// airdrop device handler callback
typedef void (*airdrop_device_handler_cb_t) (const airdrop_device_info* _Device, x_socket_t _Socket, void* _UserData);


/// Create airdrop context
/// \param _Context : pointer to receive context
/// \return : Return AIRDROP_E_SUCCESS for success, others for failure
_XAIRDROPAPI_ airdrop_error_t __xcall__ airdrop_context_new(airdrop_context_t* _Context);

/// Release the airdrop context
/// \param _Context : context handle
/// \return : Return AIRDROP_E_SUCCESS for success, others for failure
_XAIRDROPAPI_ airdrop_error_t __xcall__ airdrop_context_free(airdrop_context_t _Context);



/// Set service port
/// \param _Context : context handle
/// \param _Port : socket port
/// \return : Return AIRDROP_E_SUCCESS for success, others for failure
_XAIRDROPAPI_ airdrop_error_t __xcall__ airdrop_set_service_port(airdrop_context_t _Context, x_ushort_t _Port);

/// Set broadcast port
/// \param _Context : context handle
/// \param _Port : socket port
/// \return : Return AIRDROP_E_SUCCESS for success, others for failure
_XAIRDROPAPI_ airdrop_error_t __xcall__ airdrop_set_broadcast_port(airdrop_context_t _Context, x_ushort_t _Port);

/// register handler
/// \param _Context : context handle
/// \param _Callback : callback interface
/// \param _UserData : callback parameter
/// \return : Return AIRDROP_E_SUCCESS for success, others for failure
_XAIRDROPAPI_ airdrop_error_t __xcall__ airdrop_register_handler(airdrop_context_t _Context, airdrop_device_handler_cb_t _Callback, void* _UserData);



/// start running the service
/// \param _Context : context handle
/// \param _Callback : callback interface
/// \param _UserData : callback parameter
/// \return : Return AIRDROP_E_SUCCESS for success, others for failure
_XAIRDROPAPI_ airdrop_error_t __xcall__ airdrop_service_start(airdrop_context_t _Context, airdrop_device_event_cb_t _Callback, void* _UserData);

/// stop running service
/// \param _Context : context handle
/// \return : Return AIRDROP_E_SUCCESS for success, others for failure
_XAIRDROPAPI_ airdrop_error_t __xcall__ airdrop_service_stop(airdrop_context_t _Context);



/// Get a list of current online devices
/// \param _Context : context handle
/// \param _Array : pointer to receive device list
/// \param _Count : pointer to the number of receiving devices
/// \return : Return AIRDROP_E_SUCCESS for success, others for failure
_XAIRDROPAPI_ airdrop_error_t __xcall__ airdrop_get_device_list(airdrop_context_t _Context, airdrop_device_info** _Array, uint32_t* _Count);

/// Release the current online device list
/// \param _Array : Device List
/// \return : Return AIRDROP_E_SUCCESS for success, others for failure
_XAIRDROPAPI_ airdrop_error_t __xcall__ airdrop_device_list_free(airdrop_device_info* _Array);


XANADU_CXX_EXTERN_END

#endif
