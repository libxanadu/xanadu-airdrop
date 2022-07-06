#include <xanadu-airdrop/package.h>


#define AIRDROP_PACKAGE_SEND_TIMEOUT			(10 * 1000)
#define AIRDROP_PACKAGE_RECV_TIMEOUT			(10 * 1000)



/// Send a bytes to the interface
_XAIRDROPAPI_ airdrop_error_t __xcall__ airdrop_package_send_bytes(x_socket_t _Socket, const void* _Bytes, uint32_t _Length)
{
	return airdrop_package_send_bytes_with_timeout(_Socket, _Bytes, _Length, AIRDROP_PACKAGE_SEND_TIMEOUT);

}

/// Receive a bytes from the interface
_XAIRDROPAPI_ airdrop_error_t __xcall__ airdrop_package_recv_bytes(x_socket_t _Socket, void* _Bytes, uint32_t _Length)
{
	return airdrop_package_recv_bytes_with_timeout(_Socket, _Bytes, _Length, AIRDROP_PACKAGE_RECV_TIMEOUT);
}

/// Send a bytes to the interface
_XAIRDROPAPI_ airdrop_error_t __xcall__ airdrop_package_send_bytes_with_timeout(x_socket_t _Socket, const void* _Bytes, uint32_t _Length, x_uint_t _Timeout)
{
	if(_Socket == X_INVALID_SOCKET || _Bytes == NULL || _Length == 0)
	{
		return AIRDROP_E_INVALID_ARG;
	}

	const char*		vBuffer = (const char*)_Bytes;
	uint32_t 		vAlready = 0;
	int 			vSync = 0;
	while(vAlready < _Length)
	{
		if(_Timeout)
		{
			int 		vSelect = x_socket_is_writable(_Socket, _Timeout);
			if(vSelect <= 0)
			{
				break;
			}
		}

		vSync = x_socket_send(_Socket, vBuffer + vAlready, (int)(_Length - vAlready), 0);
		if(vSync <= 0)
		{
			break;
		}
		vAlready += vSync;
	}

	if(vAlready !=  _Length)
	{
		return AIRDROP_E_SEND_TIMEOUT;
	}
	return AIRDROP_E_SUCCESS;
}

/// Receive a bytes from the interface
_XAIRDROPAPI_ airdrop_error_t __xcall__ airdrop_package_recv_bytes_with_timeout(x_socket_t _Socket, void* _Bytes, uint32_t _Length, x_uint_t _Timeout)
{
	if(_Socket == X_INVALID_SOCKET || _Bytes == NULL || _Length == 0)
	{
		return AIRDROP_E_INVALID_ARG;
	}

	char*			vBuffer = (char*)_Bytes;
	uint32_t 		vAlready = 0;
	int 			vSync = 0;
	while(vAlready < _Length)
	{
		if(_Timeout)
		{
			int 		vSelect = x_socket_is_readable(_Socket, _Timeout);
			if(vSelect <= 0)
			{
				break;
			}
		}

		vSync = x_socket_recv(_Socket, vBuffer + vAlready, (int)(_Length - vAlready), 0);
		if(vSync <= 0)
		{
			break;
		}
		vAlready += vSync;
	}

	if(vAlready !=  _Length)
	{
		return AIRDROP_E_SEND_TIMEOUT;
	}
	return AIRDROP_E_SUCCESS;
}





/// Send a Json object package to the interface
_XAIRDROPAPI_ airdrop_error_t __xcall__ airdrop_package_send_json(x_socket_t _Socket, xtl_json_t _Package)
{
	return airdrop_package_send_json_with_timeout(_Socket, _Package, AIRDROP_PACKAGE_SEND_TIMEOUT);
}

/// Receive a Json object package from the interface
_XAIRDROPAPI_ airdrop_error_t __xcall__ airdrop_package_recv_json(x_socket_t _Socket, xtl_json_t* _Package)
{
	return airdrop_package_recv_json_with_timeout(_Socket, _Package, AIRDROP_PACKAGE_RECV_TIMEOUT);
}

/// Send a Json object package to the interface
_XAIRDROPAPI_ airdrop_error_t __xcall__ airdrop_package_send_json_with_timeout(x_socket_t _Socket, xtl_json_t _Package, x_uint_t _Timeout)
{
	if(_Socket == X_INVALID_SOCKET || _Package == NULL)
	{
		return AIRDROP_E_INVALID_ARG;
	}

	airdrop_error_t		vError = AIRDROP_E_UNKNOWN_ERROR;

	// Convert Json to byte array
	char*			vJsonBytes = xtl_json_print(_Package);
	uint32_t 		vJsonSize = (uint32_t)x_posix_strlen(vJsonBytes);
	if(vJsonBytes == NULL)
	{
		return AIRDROP_E_NOT_ENOUGH_MEMORY;
	}
	if(vJsonSize == 0)
	{
		x_posix_free(vJsonBytes);
		return AIRDROP_E_NOT_ENOUGH_MEMORY;
	}

	// Send the length first
	uint32_t 	vNetworkSize = x_endian_host_to_network_32(vJsonSize);
	vError = airdrop_package_send_bytes_with_timeout(_Socket, &vNetworkSize, sizeof(uint32_t), _Timeout);
	if(vError != AIRDROP_E_SUCCESS)
	{
		x_posix_free(vJsonBytes);
		return vError;
	}

	// send packet data
	vError = airdrop_package_send_bytes_with_timeout(_Socket, vJsonBytes, vJsonSize, _Timeout);
	x_posix_free(vJsonBytes);
	return vError;
}

/// Receive a Json object package from the interface
_XAIRDROPAPI_ airdrop_error_t __xcall__ airdrop_package_recv_json_with_timeout(x_socket_t _Socket, xtl_json_t* _Package, x_uint_t _Timeout)
{
	if(_Socket == X_INVALID_SOCKET || _Package == NULL)
	{
		return AIRDROP_E_INVALID_ARG;
	}

	airdrop_error_t		vError = AIRDROP_E_UNKNOWN_ERROR;

	// Receive length first
	uint32_t 	vNetworkSize = 0;
	vError = airdrop_package_recv_bytes_with_timeout(_Socket, &vNetworkSize, sizeof(uint32_t), _Timeout);
	if(vError != AIRDROP_E_SUCCESS)
	{
		return vError;
	}

	// request enough memory
	uint32_t 	vJsonSize = x_endian_network_to_host_32(vNetworkSize);
	char*		vJsonBytes = (char*) x_posix_malloc(vJsonSize + 1);
	if(vJsonBytes == NULL)
	{
		return AIRDROP_E_INVALID_ARG;
	}
	vJsonBytes[vJsonSize] = 0;

	// send packet data
	vError = airdrop_package_send_bytes_with_timeout(_Socket, vJsonBytes, vJsonSize, _Timeout);
	if(vError != AIRDROP_E_SUCCESS)
	{
		x_posix_free(vJsonBytes);
		return AIRDROP_E_INVALID_ARG;
	}

	// Convert byte array to Json
	xtl_json_t 	vJson = xtl_json_parse(vJsonBytes);
	x_posix_free(vJsonBytes);
	if(vJson == NULL)
	{
		return AIRDROP_E_INVALID_ARG;
	}

	*_Package = vJson;
	return AIRDROP_E_SUCCESS;
}
