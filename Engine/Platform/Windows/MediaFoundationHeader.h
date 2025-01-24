#pragma once
#ifndef MediaFoundationHeader_H_43F7AC50_CE6A_43E6_96D4_35375489EF6D
#define MediaFoundationHeader_H_43F7AC50_CE6A_43E6_96D4_35375489EF6D

#if (_WIN32_WINNT >= 0x0602 /*_WIN32_WINNT_WIN8*/)
#include <initguid.h>
#include <mfidl.h>
#include <mfapi.h>
#include <mfreadwrite.h>
#include <propvarutil.h>
#include <cguid.h>
#pragma comment(lib,"mfplat.lib")
#pragma comment(lib,"mfreadwrite.lib")

#elif (_WIN32_WINNT < _WIN32_WINNT_WIN7 )
#error This code needs _WIN32_WINNT set to 0x0601 or higher. It is compatible with Windows Vista with KB 2117917 installed
#else
#define MF_SDK_VERSION 0x0001  
#define MF_API_VERSION 0x0070 
#define MF_VERSION (MF_SDK_VERSION << 16 | MF_API_VERSION)  
#include <initguid.h>
#include <mfidl.h>
#include <mfapi.h>
#include <mfreadwrite.h>
//#pragma comment(lib,"mfplat_vista.lib")
//#pragma comment(lib,"mfreadwrite.lib")

#endif


struct FMediaFoundation
{
	static bool Initialize()
	{
		HRESULT hr;
		hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
		hr = MFStartup(MF_VERSION);
		return SUCCEEDED(hr);
	}

	static void Finalize()
	{
		MFShutdown();
		CoUninitialize();
	}
};


struct MediaFoundationScope
{
	MediaFoundationScope()
	{
		bInit = FMediaFoundation::Initialize();
	}
	~MediaFoundationScope()
	{
		FMediaFoundation::Finalize();
	}
	bool bInit;
};




#endif // MediaFoundationHeader_H_43F7AC50_CE6A_43E6_96D4_35375489EF6D
