#include <xanadu-airdrop/header.h>


// dll main
#if defined(XANADU_SYSTEM_WINDOWS)
extern "C" BOOL WINAPI DllMain(HANDLE _HDllHandle, DWORD _Reason, LPVOID _Reserved)
{
	XANADU_UNUSED(_HDllHandle);
	XANADU_UNUSED(_Reserved);

	switch(_Reason)
	{
		case DLL_PROCESS_ATTACH:
			break;
		case DLL_PROCESS_DETACH:
			break;
		default:
			break;
	}
	return TRUE;
}
#else
__attribute((constructor)) void xanadu_airdrop_dynamic_library_init(void)
{
};

__attribute((destructor)) void xanadu_airdrop_dynamic_library_fini(void)
{
};
#endif
