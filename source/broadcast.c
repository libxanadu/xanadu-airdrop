#include <xanadu-airdrop/broadcast.h>



// The key used by the broadcast event
#define AIRDROP_BROADCAST_KEY_NAME		("Name")
#define AIRDROP_BROADCAST_KEY_PORT		("Port")

/// airdrop broadcast context
struct airdrop_broadcast_context
{
	x_ushort_t 				broadcast_port;				// broadcast listening port
	x_ushort_t 				service_port;				// service listening port

	bool					thread_notify_running;			// Notify whether the thread is running
	bool					thread_notify_allow;			// Notify whether the thread is allowed to run

	bool					thread_broadcast_running;		// Broadcast whether the thread is running
	bool					thread_broadcast_allow;			// Broadcast whether the thread is allowed to run

	airdrop_broadcast_event_cb_t		notify_address;				// event callback interface
	void*					notify_user_data;			// event callback param

	xtl_vector_t 				device_vector;				// List of currently connected devices
};



// Notify device events
static void airdrop_broadcast_notify_device_events(airdrop_broadcast_context_t _Context, const airdrop_broadcast_device* _Device, airdrop_broadcast_event_t _Event)
{
	if(_Context->notify_address)
	{
		_Context->notify_address(_Device, _Event, _Context->notify_user_data);
	}
}

// Update device information
static void airdrop_broadcast_update_device_info(airdrop_broadcast_context_t _Context, struct sockaddr_in* _Address, const char* _JsonString)
{
	xtl_json_t 	vJson = xtl_json_parse(_JsonString);
	if(vJson == NULL)
	{
		return;
	}

	x_llong_t 			vCurrentMillisecond = x_time_system_millisecond();
	airdrop_broadcast_device*	vDevice = NULL;
	char*				vIPAddress = x_socket_address_to_string((struct sockaddr*)_Address);
	const char*			vDeviceName = xtl_json_as_string(xtl_json_object_item(vJson, AIRDROP_BROADCAST_KEY_NAME));
	x_ushort_t			vDevicePort = (x_ushort_t)xtl_json_as_ullong(xtl_json_object_item(vJson, AIRDROP_BROADCAST_KEY_PORT));
	if(vIPAddress && vDeviceName && vDevicePort)
	{
		// First check if the device is in the list
		for(xtl_vector_iter_t vIterator = xtl_vector_begin(_Context->device_vector); vIterator != xtl_vector_end(_Context->device_vector); vIterator = xtl_vector_iter_next(_Context->device_vector, vIterator))
		{
			vDevice = (airdrop_broadcast_device*) xtl_vector_iter_data(_Context->device_vector, vIterator);
			if(0 == x_posix_strcmp(vDevice->ipv4, vIPAddress))
			{
				if(0 == x_posix_strcmp(vDevice->name, vDeviceName) && vDevice->port == vDevicePort)
				{
					// When the device data is consistent, update the device time
					vDevice->lu_time = vCurrentMillisecond;
				}
				else
				{
					// Otherwise we need to disconnect and reconnect
					airdrop_broadcast_notify_device_events(_Context, vDevice, AIRDROP_BROADCAST_DISCONNECT);
					xtl_vector_erase(_Context->device_vector, vIterator);
					vDevice = NULL;
				}
				// If it is found successfully, we jump out of the loop, and subsequent addition operations can be ignored
				break;
			}
			// When the device does not match, set the device to be empty so that the device can be added later
			vDevice = NULL;
		}

		// Add a new device when the device is not found in the device list
		if(vDevice == NULL)
		{
			vDevice = (airdrop_broadcast_device*) x_posix_malloc(sizeof(airdrop_broadcast_device));
			if(vDevice)
			{
				x_posix_strcpy(vDevice->ipv4, vIPAddress);
				x_posix_strcpy(vDevice->name, vDeviceName);
				vDevice->port = vDevicePort;
				vDevice->lu_time = vCurrentMillisecond;

				xtl_vector_push_back(_Context->device_vector, vDevice);
				airdrop_broadcast_notify_device_events(_Context, vDevice, AIRDROP_BROADCAST_CONNECT);
				x_posix_free(vDevice);
			}
		}

		// Check if any device times out
		for(xtl_vector_iter_t vIterator = xtl_vector_begin(_Context->device_vector); vIterator != xtl_vector_end(_Context->device_vector);)
		{
			vDevice = (airdrop_broadcast_device*) xtl_vector_iter_data(_Context->device_vector, vIterator);
			if((vCurrentMillisecond - vDevice->lu_time) > AIRDROP_BROADCAST_DISCONNECT_TIME)
			{
				airdrop_broadcast_notify_device_events(_Context, vDevice, AIRDROP_BROADCAST_DISCONNECT);
				vIterator = xtl_vector_erase(_Context->device_vector, vIterator);
			}
			else
			{
				vDevice->lu_time = vCurrentMillisecond;
				vIterator = xtl_vector_iter_next(_Context->device_vector, vIterator);
			}
		}
	}
	x_posix_free(vIPAddress);
	xtl_json_free(vJson);
}

// Broadcast thread: Notify other devices of online status
static void airdrop_broadcast_thread_online(void* _Param)
{
	airdrop_broadcast_context_t	_Context = (airdrop_broadcast_context_t)_Param;


	x_socket_t		vSocket = x_socket_open(AF_INET, SOCK_DGRAM, 0);
	int			opt = 1;
	x_socket_set_opt(vSocket, SOL_SOCKET, SO_BROADCAST, (char*)&opt, sizeof(opt));

	struct sockaddr_in	vAddress;
	vAddress.sin_family = AF_INET;
	vAddress.sin_port = htons(_Context->broadcast_port);
	vAddress.sin_addr.s_addr = INADDR_BROADCAST;

	// Create fixed send data
	char*			vBuffer = NULL;
	uint32_t 		vLength = 0;
	char 			vHostName[XANADU_PATH_MAX] = {0};
	xtl_json_t 		vJson = xtl_json_new_object();
	if(vJson == NULL)
	{
		_Context->thread_notify_running = false;
		XLOG_ERROR(AIRDROP_LOG_TAG, u8"[%s : %d] Broadcast notification thread failed to start : errno(%d)", __XFUNCTION__, __XLINE__, x_posix_errno());
		return;
	}
	x_socket_get_host_name(vHostName, XANADU_PATH_MAX);
	xtl_json_object_insert(vJson, AIRDROP_BROADCAST_KEY_NAME, xtl_json_new_string(vHostName));
	xtl_json_object_insert(vJson, AIRDROP_BROADCAST_KEY_PORT, xtl_json_new_number(_Context->service_port));
	vBuffer = xtl_json_print(vJson);
	vLength = x_posix_strlen(vBuffer);
	xtl_json_free(vJson);
	if(vBuffer == NULL)
	{
		_Context->thread_notify_running = false;
		XLOG_ERROR(AIRDROP_LOG_TAG, u8"[%s : %d] Broadcast notification thread failed to start : errno(%d)", __XFUNCTION__, __XLINE__, x_posix_errno());
		return;
	}

	_Context->thread_notify_running = true;
	XLOG_INFO(AIRDROP_LOG_TAG, u8"[%s : %d] Broadcast notification thread has started", __XFUNCTION__, __XLINE__);
	while(_Context->thread_notify_allow)
	{
		int 		vSync = x_socket_sendto(vSocket, vBuffer, (int)vLength, 0, (struct sockaddr*)&vAddress, sizeof(struct sockaddr_in));
		if(vSync != (int)vLength)
		{
			XLOG_ERROR(AIRDROP_LOG_TAG, u8"[%s : %d] Broadcast notification thread cannot send data : errno(%d)", __XFUNCTION__, __XLINE__, x_posix_errno());
			break;
		}
		x_posix_usleep(AIRDROP_BROADCAST_SOCKET_MSLEEP);
	}
	XLOG_WARNING(AIRDROP_LOG_TAG, u8"[%s : %d] Broadcast notification thread has exited", __XFUNCTION__, __XLINE__);
	_Context->thread_notify_running = false;
	x_posix_free(vBuffer);
}

// Broadcast thread: listening for device notifications
static void airdrop_broadcast_thread_accept(void* _Param)
{
	airdrop_broadcast_context_t	_Context = (airdrop_broadcast_context_t)_Param;

	// create socket
	x_socket_t		vSocket = x_socket_open(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(X_INVALID_SOCKET == vSocket)
	{
		XLOG_ERROR(AIRDROP_LOG_TAG, u8"[%s : %d] Broadcast listener thread failed to start : errno(%d)", __XFUNCTION__, __XLINE__, x_posix_errno());
		return;
	}
	int			opt = 1;
	x_socket_set_opt(vSocket, SOL_SOCKET, SO_BROADCAST, (char*)&opt, sizeof(int));

	struct sockaddr_in	vAddress;
	vAddress.sin_family = AF_INET;
	vAddress.sin_port = htons(_Context->broadcast_port);
	vAddress.sin_addr.s_addr = INADDR_BROADCAST;

	int 			vError = x_socket_bind(vSocket, (struct sockaddr*)&vAddress, sizeof(struct sockaddr_in));
	if(SOCKET_ERROR == vError)
	{
		XLOG_ERROR(AIRDROP_LOG_TAG, u8"[%s : %d] Broadcast listener thread failed to start : errno(%d)", __XFUNCTION__, __XLINE__, x_posix_errno());
		x_socket_close(vError);
		return;
	}

	char 			vBytes[XANADU_PATH_MAX + 1] = {0};
	struct sockaddr_in	vClientAddress;
	int 			vClientSize = 0;

	_Context->thread_broadcast_running = true;
	XLOG_INFO(AIRDROP_LOG_TAG, u8"[%s : %d] Broadcast listener thread has been started", __XFUNCTION__, __XLINE__);
	while(_Context->thread_broadcast_allow)
	{
		int		vSync = x_socket_recvfrom(vSocket, vBytes, XANADU_PATH_MAX, 0, (struct sockaddr*)&vClientAddress, &vClientSize);
		if(vSync <= 0)
		{
			XLOG_ERROR(AIRDROP_LOG_TAG, u8"[%s : %d] Broadcast listener thread cannot receive data : errno(%d)", __XFUNCTION__, __XLINE__, x_posix_errno());
			break;
		}
		vBytes[vSync] = 0;

		airdrop_broadcast_update_device_info(_Context, &vClientAddress, vBytes);
	}
	XLOG_WARNING(AIRDROP_LOG_TAG, u8"[%s : %d] Broadcast listener thread has exited", __XFUNCTION__, __XLINE__);
	_Context->thread_broadcast_running = false;
}




/// Create broadcast context
_XAIRDROPAPI_ airdrop_error_t __xcall__ airdrop_broadcast_context_create(airdrop_broadcast_context_t* _Context)
{
	if(_Context == NULL)
	{
		return AIRDROP_E_INVALID_ARG;
	}

	airdrop_broadcast_context_t	vObject = (airdrop_broadcast_context_t) x_posix_malloc(sizeof(struct airdrop_broadcast_context));
	if(vObject == NULL)
	{
		return AIRDROP_E_NOT_ENOUGH_MEMORY;
	}
	x_posix_memset(vObject, 0, sizeof(struct airdrop_broadcast_context));

	vObject->broadcast_port = AIRDROP_BROADCAST_DEFAULT_PORT;
	vObject->thread_notify_running = false;
	vObject->thread_notify_allow = false;
	vObject->thread_broadcast_running = false;
	vObject->thread_broadcast_allow = false;
	vObject->notify_address = NULL;
	vObject->notify_user_data = NULL;
	vObject->device_vector = xtl_vector_new(sizeof(airdrop_broadcast_device));
	if(vObject->device_vector == NULL)
	{
		x_posix_free(vObject);
		return AIRDROP_E_NOT_ENOUGH_MEMORY;
	}

	*_Context = vObject;
	return AIRDROP_E_SUCCESS;
}

/// Release broadcast context
_XAIRDROPAPI_ airdrop_error_t __xcall__ airdrop_broadcast_context_release(airdrop_broadcast_context_t _Context)
{
	if(_Context == NULL)
	{
		return AIRDROP_E_INVALID_ARG;
	}

	airdrop_broadcast_event_stop(_Context);

	xtl_vector_free(_Context->device_vector);
	x_posix_free(_Context);
	return AIRDROP_E_SUCCESS;
}



/// Set service port
_XAIRDROPAPI_ airdrop_error_t __xcall__ airdrop_broadcast_set_service_port(airdrop_broadcast_context_t _Context, x_ushort_t _Port)
{
	if(_Context == NULL)
	{
		return AIRDROP_E_INVALID_ARG;
	}

	_Context->service_port = _Port;

	return AIRDROP_E_SUCCESS;
}

/// Set the communication port
_XAIRDROPAPI_ airdrop_error_t __xcall__ airdrop_broadcast_set_broadcast_port(airdrop_broadcast_context_t _Context, x_ushort_t _Port)
{
	if(_Context == NULL)
	{
		return AIRDROP_E_INVALID_ARG;
	}

	_Context->broadcast_port = _Port;

	return AIRDROP_E_SUCCESS;
}



/// start broadcast event
_XAIRDROPAPI_ airdrop_error_t __xcall__ airdrop_broadcast_event_start(airdrop_broadcast_context_t _Context, airdrop_broadcast_event_cb_t _Function, void* _UserData)
{
	if(_Context == NULL || _Function == NULL)
	{
		return AIRDROP_E_INVALID_ARG;
	}

	airdrop_broadcast_event_stop(_Context);
	_Context->notify_address = _Function;
	_Context->notify_user_data = _UserData;

	_Context->thread_notify_allow = true;
	_Context->thread_broadcast_allow = true;
	x_thread_create(airdrop_broadcast_thread_online, _Context);
	x_thread_create(airdrop_broadcast_thread_accept, _Context);

	for(int vIndex = 0; vIndex < 10; ++vIndex)
	{
		if(_Context->thread_broadcast_running && _Context->thread_notify_running)
		{
			break;
		}
		x_posix_msleep(100);
	}

	if(_Context->thread_broadcast_running && _Context->thread_notify_running)
	{
		return AIRDROP_E_SUCCESS;
	}
	return AIRDROP_E_UNKNOWN_ERROR;
}

/// stop broadcast event
_XAIRDROPAPI_ airdrop_error_t __xcall__ airdrop_broadcast_event_stop(airdrop_broadcast_context_t _Context)
{
	if(_Context == NULL)
	{
		return AIRDROP_E_INVALID_ARG;
	}

	_Context->thread_notify_allow = false;
	_Context->thread_broadcast_allow = false;

	while(_Context->thread_broadcast_running && _Context->thread_notify_running)
	{
		x_posix_msleep(100);
	}

	for(xtl_vector_iter_t vIterator = xtl_vector_begin(_Context->device_vector); vIterator != xtl_vector_end(_Context->device_vector); vIterator = xtl_vector_iter_next(_Context->device_vector, vIterator))
	{
		airdrop_broadcast_device*	vDevice = (airdrop_broadcast_device*) xtl_vector_iter_data(_Context->device_vector, vIterator);
		airdrop_broadcast_notify_device_events(_Context, vDevice, AIRDROP_BROADCAST_DISCONNECT);
	}
	xtl_vector_clear(_Context->device_vector);

	_Context->notify_address = NULL;
	_Context->notify_user_data = NULL;
	return AIRDROP_E_SUCCESS;
}
