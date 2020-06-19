#ifndef RHIDefine_h__
#define RHIDefine_h__

#include "Core/IntegerType.h"
#include "CoreShare.h"

#define SHADER_ENTRY( NAME ) #NAME
#define SHADER_PARAM( NAME ) #NAME

#define GPU_BUFFER_ALIGN alignas(16)

#define RHI_API CORE_API

namespace Render
{
	extern RHI_API float gRHIClipZMin;
	extern RHI_API float gRHIProjectYSign;

	enum class DeviceVendorName
	{
		Unknown,
		NVIDIA,
		ATI,
		Intel,
	};

	extern DeviceVendorName gRHIDeviceVendorName;

	enum ColorWriteMask
	{
		CWM_None = 0,
		CWM_R = 1 << 0,
		CWM_G = 1 << 1,
		CWM_B = 1 << 2,
		CWM_A = 1 << 3,
		CWM_RGB = CWM_R | CWM_G | CWM_B ,
		CWM_RGBA = CWM_RGB | CWM_A ,
		CWM_Alpha = CWM_A ,
	};


	enum class EFillMode
	{
		Point ,
		Wireframe,
		Solid,
		System,
	};

	enum class ECullMode
	{
		None,
		Front ,
		Back ,
	};

	enum class ECompareFunc
	{
		Never,
		Less,
		Equal,
		NotEqual,
		LessEqual,
		Greater,
		GeraterEqual,
		Always,
	};


	struct Blend
	{
		enum Factor
		{
			eOne,
			eZero,
			eSrcAlpha,
			eOneMinusSrcAlpha,
			eDestAlpha,
			eOneMinusDestAlpha,
			eSrcColor,
			eOneMinusSrcColor,
			eDestColor,
			eOneMinusDestColor,
		};

		enum Operation
		{
			eAdd,
			eSub,
			eReverseSub ,
			eMax ,
			eMin ,
		};
	};

	struct Stencil
	{
		enum Operation
		{
			eKeep,
			eZero,
			eReplace,
			eIncr,
			eIncrWarp,
			eDecr,
			eDecrWarp,
			eInvert,
		};
		enum Function
		{
			eNever,
			eLess,
			eLessEqual,
			eGreater,
			eGreaterEqual,
			eEqual,
			eNotEqual,
			eAlways,
		};
	};

	enum EAccessOperator
	{
		AO_READ_ONLY,
		AO_WRITE_ONLY,
		AO_READ_AND_WRITE,
	};

	enum class EPrimitive
	{
		Points,
		TriangleList,
		TriangleStrip,
		LineList,
		LineStrip,
		TriangleAdjacency ,
		//Discard?
		TriangleFan,
		LineLoop,
		Quad,
		Polygon ,
		//
		Patchs ,
	};

	enum class ELockAccess
	{
		ReadOnly,
		ReadWrite,
		WriteOnly,
		WriteDiscard,
	};

	namespace EShader
	{
		enum Type
		{
			Vertex = 0,
			Pixel = 1,
			Geometry = 2,
			Compute = 3,
			Hull = 4,
			Domain = 5,

			Count,
			MaxStorageSize = 4,
			Empty = -1,
		};
	};


	struct Sampler
	{
		enum AddressMode
		{
			eWarp,
			eClamp,
			eMirror,
			eBorder,
		};

		enum Filter
		{
			ePoint,
			eBilinear,
			eTrilinear,
			eAnisotroicPoint,
			eAnisotroicLinear,
		};

	};


}//namespace Render

#endif // RHIDefine_h__