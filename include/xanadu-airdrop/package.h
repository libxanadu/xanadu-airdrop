#ifndef			_XANADU_AIRDROP_PACKAGE_H_
#define			_XANADU_AIRDROP_PACKAGE_H_

#include <xanadu-airdrop/header.h>

XANADU_CXX_EXTERN_BEGIN



/// Send a bytes to the interface
/// \param _Socket : Interface descriptor
/// \param _Bytes : Byte buffer to send
/// \param _Length : length of bytes to send
/// \return : Return AIRDROP_E_SUCCESS for success, others for failure
_XAIRDROPAPI_ airdrop_error_t __xcall__ airdrop_package_send_bytes(x_socket_t _Socket, const void* _Bytes, uint32_t _Length);

/// Receive a bytes from the interface
/// \param _Socket : Interface descriptor
/// \param _Bytes : Byte buffer to receive
/// \param _Length : length of bytes to receive
/// \return : Return AIRDROP_E_SUCCESS for success, others for failure
_XAIRDROPAPI_ airdrop_error_t __xcall__ airdrop_package_recv_bytes(x_socket_t _Socket, void* _Bytes, uint32_t _Length);

/// Send a bytes to the interface
/// \param _Socket : Interface descriptor
/// \param _Bytes : Byte buffer to send
/// \param _Length : length of bytes to send
/// \param _Timeout : timeout in milliseconds, 0 means never timeout
/// \return : Return AIRDROP_E_SUCCESS for success, others for failure
_XAIRDROPAPI_ airdrop_error_t __xcall__ airdrop_package_send_bytes_with_timeout(x_socket_t _Socket, const void* _Bytes, uint32_t _Length, x_uint_t _Timeout);

/// Receive a bytes from the interface
/// \param _Socket : Interface descriptor
/// \param _Bytes : Byte buffer to receive
/// \param _Length : length of bytes to receive
/// \param _Timeout : timeout in milliseconds, 0 means never timeout
/// \return : Return AIRDROP_E_SUCCESS for success, others for failure
_XAIRDROPAPI_ airdrop_error_t __xcall__ airdrop_package_recv_bytes_with_timeout(x_socket_t _Socket, void* _Bytes, uint32_t _Length, x_uint_t _Timeout);





/// Send a Json object package to the interface
/// \param _Socket : Interface descriptor
/// \param _Package : Json object to send
/// \return : Return AIRDROP_E_SUCCESS for success, others for failure
_XAIRDROPAPI_ airdrop_error_t __xcall__ airdrop_package_send_json(x_socket_t _Socket, xtl_json_t _Package);

/// Receive a Json object package from the interface
/// \param _Socket : Interface descriptor
/// \param _Package : Json object pointer to receive data
/// \return : Return AIRDROP_E_SUCCESS for success, others for failure
_XAIRDROPAPI_ airdrop_error_t __xcall__ airdrop_package_recv_json(x_socket_t _Socket, xtl_json_t* _Package);

/// Send a Json object package to the interface
/// \param _Socket : Interface descriptor
/// \param _Package : Json object to send
/// \param _Timeout : timeout in milliseconds, 0 means never timeout
/// \return : Return AIRDROP_E_SUCCESS for success, others for failure
_XAIRDROPAPI_ airdrop_error_t __xcall__ airdrop_package_send_json_with_timeout(x_socket_t _Socket, xtl_json_t _Package, x_uint_t _Timeout);

/// Receive a Json object package from the interface
/// \param _Socket : Interface descriptor
/// \param _Package : Json object pointer to receive data
/// \param _Timeout : timeout in milliseconds, 0 means never timeout
/// \return : Return AIRDROP_E_SUCCESS for success, others for failure
_XAIRDROPAPI_ airdrop_error_t __xcall__ airdrop_package_recv_json_with_timeout(x_socket_t _Socket, xtl_json_t* _Package, x_uint_t _Timeout);



XANADU_CXX_EXTERN_END

#endif
