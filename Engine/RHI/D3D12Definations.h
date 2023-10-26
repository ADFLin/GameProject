#pragma once
#ifndef D3D12Definations_H_38C48893_1792_4DA4_8C98_1D743AE3EE94
#define D3D12Definations_H_38C48893_1792_4DA4_8C98_1D743AE3EE94

#include "MarcoCommon.h"
#include "Core/IntegerType.h"
#include "Platform/Windows/ComUtility.h"

#include <D3D12.h>
#include <dxgi1_3.h>
#include <dxgi1_4.h>
#include <dxgi1_6.h>
#undef max
#undef min

#define ERROR_MSG_GENERATE( HR , CODE , FILE , LINE )\
	LogWarning(1, "ErrorCode = 0x%x File = %s Line = %s %s ", HR , FILE, #LINE, #CODE)

#define VERIFY_D3D12RESULT_INNER( FILE , LINE , CODE ,ERRORCODE )\
	{ HRESULT hr = CODE; if( hr != S_OK ){ ERROR_MSG_GENERATE( hr , CODE, FILE, LINE ); ERRORCODE } }

#define VERIFY_D3D12RESULT( CODE , ERRORCODE ) VERIFY_D3D11RESULT_INNER( __FILE__ , __LINE__ , CODE , ERRORCODE )
#define VERIFY_D3D12RESULT_RETURN_FALSE( CODE ) VERIFY_D3D11RESULT_INNER( __FILE__ , __LINE__ , CODE , return false; )


#define SAFE_RELEASE( PTR ) if ( PTR ){ PTR->Release(); PTR = nullptr; }

namespace Render
{
	using IDXGISwapChainRHI = IDXGISwapChain3;
	using ID3D12DeviceRHI = ID3D12Device8;
	using ID3D12GraphicsCommandListRHI = ID3D12GraphicsCommandList6;

}//namespace Render


#endif // D3D12Definations_H_38C48893_1792_4DA4_8C98_1D743AE3EE94