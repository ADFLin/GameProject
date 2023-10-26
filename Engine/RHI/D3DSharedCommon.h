#pragma once
#ifndef D3DSharedCommon_H_846539EC_B59A_4E0F_9EBB_8D2735D24F68
#define D3DSharedCommon_H_846539EC_B59A_4E0F_9EBB_8D2735D24F68

#include "RHICommon.h"
#include "LogSystem.h"

#include <D3DCommon.h>
#include <dxgiformat.h>

namespace Render
{

	struct D3DTranslate
	{
		static D3D_PRIMITIVE_TOPOLOGY To(EPrimitive type);
		static DXGI_FORMAT To(EVertex::Format format, bool bNormalized);
		static DXGI_FORMAT To(ETexture::Format format);
		static DXGI_FORMAT IndexType(RHIBuffer* buffer);
	};


	struct FD3DUtils
	{
		static DeviceVendorName GetDevicVenderName(UINT vendorId);

	};

	class ID3DErrorHandler
	{
	public:
		static ID3DErrorHandler* StaticInstance;
		ID3DErrorHandler()
		{
			StaticInstance = this;
		}
		virtual ~ID3DErrorHandler() = default;
		virtual void handleErrorResult(HRESULT errorResult) = 0;
	};


}//namespace Render



#define ERROR_MSG_GENERATE( HR , CODE , FILE , LINE )\
	LogWarning(1, "ErrorCode = 0x%x File = %s Line = %s %s ", HR , FILE, #LINE, #CODE)

#define VERIFY_D3D_RESULT_INNER( FILE , LINE , CODE ,ERRORCODE )\
	{\
		HRESULT hResult = CODE;\
		if( FAILED(hResult) )\
		{\
			if (ID3DErrorHandler::StaticInstance)\
			{\
				ID3DErrorHandler::StaticInstance->handleErrorResult(hResult);\
			}\
			ERROR_MSG_GENERATE( hResult , CODE, FILE, LINE );\
			ERRORCODE\
		}\
	}

#define VERIFY_D3D_RESULT( CODE , ERRORCODE ) VERIFY_D3D_RESULT_INNER( __FILE__ , __LINE__ , CODE , ERRORCODE )
#define VERIFY_D3D_RESULT_RETURN_FALSE( CODE ) VERIFY_D3D_RESULT_INNER( __FILE__ , __LINE__ , CODE , return false; )

#define SAFE_RELEASE( PTR ) if ( PTR ){ PTR->Release(); PTR = nullptr; }


#endif // D3DSharedCommon_H_846539EC_B59A_4E0F_9EBB_8D2735D24F68
