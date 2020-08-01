#ifndef RHIDefine_h__
#define RHIDefine_h__

#include "Core/IntegerType.h"
#include "CoreShare.h"

#define SHADER_ENTRY( NAME ) #NAME
#define SHADER_PARAM( NAME ) #NAME
#define SHADER_SAMPLER(NAME) #NAME"Sampler"

#define GPU_BUFFER_ALIGN alignas(16)

#define RHI_API CORE_API

namespace Render
{
	extern RHI_API float GRHIClipZMin;
	extern RHI_API float GRHIProjectYSign;

	enum class DeviceVendorName
	{
		Unknown,
		NVIDIA,
		ATI,
		Intel,
	};

	extern DeviceVendorName GRHIDeviceVendorName;

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
		PatchPoint1,
		PatchPoint2,
		PatchPoint3,
		PatchPoint4,
		PatchPoint5,
		PatchPoint6,
		PatchPoint7,
		PatchPoint8,
		PatchPoint9,
		PatchPoint10,
		PatchPoint11,
		PatchPoint12,
		PatchPoint13,
		PatchPoint14,
		PatchPoint15,
		PatchPoint16,
		PatchPoint17,
		PatchPoint18,
		PatchPoint19,
		PatchPoint20,
		PatchPoint21,
		PatchPoint22,
		PatchPoint23,
		PatchPoint24,
		PatchPoint25,
		PatchPoint26,
		PatchPoint27,
		PatchPoint28,
		PatchPoint29,
		PatchPoint30,
		PatchPoint31,
		PatchPoint32,

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


	enum class EFrontFace
	{
		Default,
		Inverse,
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

	enum class EBufferStoreOp
	{
		Store,
		DontCare,
	};

	enum class EBufferLoadOp
	{
		Load,
		Clear,
		DontCare,
	};


}//namespace Render

#endif // RHIDefine_h__