#include <xanadu-airdrop/airdrop.h>
#include <xanadu-airdrop/broadcast.h>
#include <xanadu-airdrop/package.h>



/// Airdrop device data
typedef struct airdrop_device_data
{
	airdrop_device_info			info;
}airdrop_device_data;

/// Airdrop Context
struct airdrop_context
{
	x_ushort_t				service_port;				// Service port
	bool					service_running;			// Service thread running status
	bool					service_allow;				// Service thread allow status

	x_mutex_t 				service_thread_mutex;			// Airdrop thread mutex
	x_ullong_t 				service_thread_number;			// Current thread number

	airdrop_device_event_cb_t		notify_address;				// callback interface
	void*					notify_user_data;			// callback parameter
	x_mutex_t 				notify_mutex;				// callback lock

	airdrop_device_handler_cb_t		handler_address;			// handler interface
	void*					handler_user_data;			// handler parameter
	x_mutex_t 				handler_mutex;				// handler lock

	airdrop_broadcast_context_t		broadcast_context;			// broadcast context

	x_mutex_t 				device_mutex;				// device mutex
	xtl_vector_t 				device_vector;				// device list
	airdrop_device_id			device_next_id;				// next device ID identifier
};

/// Service child thread data structure
typedef struct _airdrop_accept_thread_data
{
	airdrop_context_t			_Context;
	x_socket_t 				_Socket;
	airdrop_device_data			_Device;
}airdrop_accept_thread_data_t;



// Service child thread
static void __xcall__ airdrop_thread_service_child_thread(void* _Param)
{
	airdrop_accept_thread_data_t*	_InfoThread = (airdrop_accept_thread_data_t*)_Param;
	airdrop_context_t		_Context = _InfoThread->_Context;
	x_socket_t			_Socket = _InfoThread->_Socket;

	// Increase the number of currently running threads
	x_mutex_lock(&(_Context->service_thread_mutex));
	++(_Context->service_thread_number);
	x_mutex_unlock(&(_Context->service_thread_mutex));

	// Execute thread logic
	/*
	while(_Context->service_allow)
	{
	}
	*/
	if(_Context->handler_address)
	{
		_Context->handler_address(&(_InfoThread->_Device.info), _Socket, _Context->handler_user_data);
	}

	// The number of currently running threads is reduced
	x_mutex_lock(&(_Context->service_thread_mutex));
	--(_Context->service_thread_number);
	x_mutex_unlock(&(_Context->service_thread_mutex));

	x_posix_free(_InfoThread);
}

// Service main thread
static void __xcall__ airdrop_thread_service_main_thread(void* _Param)
{
	x_socket_t		vSocket = X_INVALID_SOCKET;
	struct sockaddr_in	vServerAddress;
	char 			vClientBuffer[XANADU_PATH_MAX] = {0};
	airdrop_context_t	_Context = (airdrop_context_t)_Param;
	int			vSockOpt = 1;

	// create socket
	vSocket = x_socket_open(AF_INET, SOCK_DGRAM, IPPROTO_TCP);
	if(vSocket == X_INVALID_SOCKET)
	{
		XLOG_ERROR(AIRDROP_LOG_TAG, u8"[%s : %d] Could not create service socket : errno(%d)", __XFUNCTION__, __XLINE__, x_posix_errno());
		return;
	}
	x_socket_set_opt(vSocket, SOL_SOCKET, SO_REUSEADDR, &vSockOpt, sizeof(int));

	vServerAddress.sin_family = AF_INET;
	vServerAddress.sin_port = htons(_Context->service_port);
	vServerAddress.sin_addr.s_addr = INADDR_ANY;
	if(x_socket_bind(vSocket, (struct sockaddr*)&vServerAddress, sizeof(struct sockaddr_in)))
	{
		XLOG_ERROR(AIRDROP_LOG_TAG, u8"[%s : %d] Unable to bind service socket : errno(%d)", __XFUNCTION__, __XLINE__, x_posix_errno());
		return;
	}
	x_socket_listen(vSocket, SOMAXCONN);

	// start processing service
	_Context->service_running = true;
	XLOG_INFO(AIRDROP_LOG_TAG, u8"[%s : %d] Service listener thread has been started", __XFUNCTION__, __XLINE__);
	while(_Context->service_allow)
	{
		int 		vSocketSelect = x_socket_is_readable(vSocket, AIRDROP_BROADCAST_SOCKET_MSLEEP);
		if(vSocketSelect < 0)
		{
			XLOG_ERROR(AIRDROP_LOG_TAG, u8"[%s : %d] Exception in service socket : errno(%d)", __XFUNCTION__, __XLINE__, x_posix_errno());
			break;
		}
		else if(vSocketSelect == 0)
		{
			continue;
		}
		else
		{
			struct sockaddr_in	vClientAddress;
			socklen_t		vClientLength = sizeof(struct sockaddr_in);
			x_posix_memset(vClientBuffer, 0, sizeof(char) * XANADU_PATH_MAX);
			x_socket_t 		vClientSocket = x_socket_accept(vSocket, (struct sockaddr*)(&vClientAddress), &vClientLength);
			if(vClientSocket == X_INVALID_SOCKET)
			{
				break;
			}

			airdrop_accept_thread_data_t*	vInfoThread = (airdrop_accept_thread_data_t*) x_posix_malloc(sizeof(airdrop_accept_thread_data_t));
			if(vInfoThread)
			{
				x_posix_memset(vInfoThread, 0, sizeof(airdrop_accept_thread_data_t));
				vInfoThread->_Context = _Context;
				vInfoThread->_Socket = vClientSocket;
				x_thread_create(airdrop_thread_service_child_thread, vInfoThread);
			}
		}
	}

	// Waiting for all child threads to exit
	while(_Context->service_thread_number)
	{
		x_posix_msleep(AIRDROP_BROADCAST_SOCKET_MSLEEP);
	}

	XLOG_WARNING(AIRDROP_LOG_TAG, u8"[%s : %d] Service listener thread has exited", __XFUNCTION__, __XLINE__);
	_Context->service_running = false;
}

/// device broadcast callback
static void __xcall__ airdrop_broadcast_event_callback(const airdrop_broadcast_device* _Device, airdrop_broadcast_event_t _Event, void* _UserData)
{
	airdrop_context_t	_Context = (airdrop_context_t)_UserData;
	airdrop_device_data*	vDevice = NULL;
	xtl_vector_iter_t	vIterator = XTL_ITERATOR_NULL;

	x_mutex_lock(&(_Context->device_mutex));

	switch (_Event)
	{
		case AIRDROP_BROADCAST_CONNECT:
		{
			vDevice = (airdrop_device_data*) x_posix_malloc(sizeof(airdrop_device_data));
			if(vDevice)
			{
				x_posix_memset(vDevice, 0, sizeof(airdrop_device_data));

				x_posix_strcpy(vDevice->info.ipv4, _Device->ipv4);
				x_posix_strcpy(vDevice->info.name, _Device->name);
				vDevice->info.port = _Device->port;
				vDevice->info.id = _Context->device_next_id;

				xtl_vector_push_back(_Context->device_vector, vDevice);

				++_Context->device_next_id;

				x_mutex_lock(&(_Context->notify_mutex));
				if(_Context->notify_address)
				{
					_Context->notify_address(&(vDevice->info), AIRDROP_DEVICE_CONNECT, _Context->notify_user_data);
				}
				x_mutex_unlock(&(_Context->notify_mutex));
			}
		}
			break;
		case AIRDROP_BROADCAST_DISCONNECT:
		{
			for(vIterator = xtl_vector_begin(_Context->device_vector); vIterator != xtl_vector_end(_Context->device_vector);)
			{
				vDevice = (airdrop_device_data*) xtl_vector_iter_data(_Context->device_vector, vIterator);
				if(0 == x_posix_strcmp(vDevice->info.ipv4, _Device->ipv4))
				{
					x_mutex_lock(&(_Context->notify_mutex));
					if(_Context->notify_address)
					{
						_Context->notify_address(&(vDevice->info), AIRDROP_DEVICE_DISCONNECT, _Context->notify_user_data);
					}
					x_mutex_unlock(&(_Context->notify_mutex));

					xtl_vector_erase(_Context->device_vector, vIterator);
					break;
				}
				else
				{
					vIterator = xtl_vector_iter_next(_Context->device_vector, vIterator);
				}
			}
		}
			break;
		default:
			break;
	}

	x_mutex_unlock(&(_Context->device_mutex));
}





/// Create airdrop context
_XAIRDROPAPI_ airdrop_error_t __xcall__ airdrop_context_new(airdrop_context_t* _Context)
{
	if(_Context == NULL)
	{
		return AIRDROP_E_INVALID_ARG;
	}

	*_Context = (struct airdrop_context*) x_posix_malloc(sizeof(struct airdrop_context));

	(*_Context)->service_port = AIRDROP_BROADCAST_DEFAULT_PORT;
	(*_Context)->service_running = false;
	(*_Context)->service_allow = false;

	x_mutex_init(&((*_Context)->service_thread_mutex));
	(*_Context)->service_thread_number = 0;

	(*_Context)->notify_address = NULL;
	(*_Context)->notify_user_data = NULL;
	x_mutex_init(&((*_Context)->notify_mutex));

	(*_Context)->handler_address = NULL;
	(*_Context)->handler_user_data = NULL;
	x_mutex_init(&((*_Context)->handler_mutex));

	(*_Context)->broadcast_context = NULL;
	airdrop_broadcast_context_create(&((*_Context)->broadcast_context));

	x_mutex_init(&((*_Context)->device_mutex));
	(*_Context)->device_next_id = AIRDROP_INVALID_DEVICE + 1;
	(*_Context)->device_vector = xtl_vector_new(sizeof(airdrop_device_data));

	return AIRDROP_E_SUCCESS;
}

/// Release the airdrop context
_XAIRDROPAPI_ airdrop_error_t __xcall__ airdrop_context_free(airdrop_context_t _Context)
{
	if(_Context == NULL)
	{
		return AIRDROP_E_INVALID_ARG;
	}

	airdrop_service_stop(_Context);

	x_mutex_destroy(&(_Context->service_thread_mutex));

	x_mutex_destroy(&(_Context->notify_mutex));

	x_mutex_destroy(&(_Context->handler_mutex));

	airdrop_broadcast_context_release(_Context->broadcast_context);

	x_mutex_destroy(&(_Context->device_mutex));
	xtl_vector_free(_Context->device_vector);

	x_posix_free(_Context);

	return AIRDROP_E_SUCCESS;
}



/// Set service port
_XAIRDROPAPI_ airdrop_error_t __xcall__ airdrop_set_service_port(airdrop_context_t _Context, x_ushort_t _Port)
{
	if(_Context == NULL || _Port == 0)
	{
		return AIRDROP_E_INVALID_ARG;
	}

	_Context->service_port = _Port;
	airdrop_broadcast_set_service_port(_Context->broadcast_context, _Port);

	return AIRDROP_E_SUCCESS;
}

/// Set broadcast port
_XAIRDROPAPI_ airdrop_error_t __xcall__ airdrop_set_broadcast_port(airdrop_context_t _Context, x_ushort_t _Port)
{
	if(_Context == NULL || _Port == 0)
	{
		return AIRDROP_E_INVALID_ARG;
	}

	airdrop_broadcast_set_broadcast_port(_Context->broadcast_context, _Port);

	return AIRDROP_E_SUCCESS;
}

/// register handler
_XAIRDROPAPI_ airdrop_error_t __xcall__ airdrop_register_handler(airdrop_context_t _Context, airdrop_device_handler_cb_t _Callback, void* _UserData)
{
	if(_Context == NULL || _Callback == NULL)
	{
		return AIRDROP_E_INVALID_ARG;
	}

	x_mutex_lock(&(_Context->handler_mutex));
	_Context->handler_address = _Callback;
	_Context->handler_user_data = _UserData;
	x_mutex_unlock(&(_Context->handler_mutex));

	return AIRDROP_E_SUCCESS;
}



/// start running the service
_XAIRDROPAPI_ airdrop_error_t __xcall__ airdrop_service_start(airdrop_context_t _Context, airdrop_device_event_cb_t _Callback, void* _UserData)
{
	if(_Context == NULL || _Callback == NULL)
	{
		return AIRDROP_E_INVALID_ARG;
	}

	airdrop_service_stop(_Context);

	x_mutex_lock(&(_Context->notify_mutex));
	_Context->notify_address = _Callback;
	_Context->notify_user_data = _UserData;
	x_mutex_unlock(&(_Context->notify_mutex));

	_Context->service_allow = true;
	x_thread_create(airdrop_thread_service_main_thread, _Context);
	airdrop_broadcast_event_start(_Context->broadcast_context, airdrop_broadcast_event_callback, _Context);

	return AIRDROP_E_SUCCESS;
}

/// stop running service
_XAIRDROPAPI_ airdrop_error_t __xcall__ airdrop_service_stop(airdrop_context_t _Context)
{
	if(_Context == NULL)
	{
		return AIRDROP_E_INVALID_ARG;
	}

	x_mutex_lock(&(_Context->handler_mutex));
	_Context->handler_address = NULL;
	_Context->handler_user_data = NULL;
	x_mutex_unlock(&(_Context->handler_mutex));

	airdrop_broadcast_event_stop(_Context->broadcast_context);

	_Context->service_allow = false;
	while(_Context->service_running)
	{
		x_posix_msleep(1);
	}

	return AIRDROP_E_SUCCESS;
}





/// Get a list of current online devices
_XAIRDROPAPI_ airdrop_error_t __xcall__ airdrop_get_device_list(airdrop_context_t _Context, airdrop_device_info** _Array, uint32_t* _Count)
{
	if(_Context == NULL || _Array == NULL || _Count == NULL)
	{
		return AIRDROP_E_INVALID_ARG;
	}

	x_mutex_lock(&(_Context->device_mutex));

	xtl_size_t		vIndex = 0;
	airdrop_device_info*	vData = NULL;
	xtl_size_t 		vSize = xtl_vector_size(_Context->device_vector);
	airdrop_device_info*	vDevices = (airdrop_device_info*) x_posix_malloc(sizeof(airdrop_device_info) * vSize);
	if(vDevices == NULL)
	{
		x_mutex_unlock(&(_Context->device_mutex));
		return AIRDROP_E_NOT_ENOUGH_MEMORY;
	}
	x_posix_memset(vDevices, 0, sizeof(airdrop_device_info) * vSize);
	for(xtl_vector_iter_t vIterator = xtl_vector_begin(_Context->device_vector); vIterator != xtl_vector_end(_Context->device_vector); vIterator = xtl_vector_iter_next(_Context->device_vector, vIterator))
	{
		vData = (airdrop_device_info*) xtl_vector_iter_data(_Context->device_vector, vIterator);
		x_posix_memcpy(vDevices + vIndex, vData, sizeof(airdrop_device_info));
		++vIndex;
	}

	x_mutex_unlock(&(_Context->device_mutex));

	*_Array = vDevices;
	*_Count = vSize;
	return AIRDROP_E_SUCCESS;
}

/// Release the current online device list
_XAIRDROPAPI_ airdrop_error_t __xcall__ airdrop_device_list_free(airdrop_device_info* _Array)
{
	if(_Array == NULL)
	{
		return AIRDROP_E_INVALID_ARG;
	}

	x_posix_free(_Array);

	return AIRDROP_E_SUCCESS;
}
