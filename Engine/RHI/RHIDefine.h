#ifndef RHIDefine_h__
#define RHIDefine_h__

#include "Core/IntegerType.h"
#include "CoreShare.h"

#define SHADER_ENTRY( NAME ) #NAME
#define SHADER_PARAM( NAME ) #NAME
#define SHADER_SAMPLER(NAME) #NAME"Sampler"

#define GPU_ALIGN alignas(16)

#define RHI_API CORE_API

#define SUPPORT_ENUM_FLAGS_OPERATION(TYPE)\
	FORCEINLINE TYPE operator | (TYPE a, TYPE b) { using T = std::underlying_type_t<TYPE>; return TYPE(T(a) | T(b)); }\
	FORCEINLINE TYPE operator & (TYPE a, TYPE b) { using T = std::underlying_type_t<TYPE>; return TYPE(T(a) & T(b)); }\
	FORCEINLINE TYPE operator ^ (TYPE a, TYPE b) { using T = std::underlying_type_t<TYPE>; return TYPE(T(a) ^ T(b)); }\
	FORCEINLINE TYPE& operator |= (TYPE& a, TYPE b) { using T = std::underlying_type_t<TYPE>; a = a | b; return a; }\
	FORCEINLINE TYPE& operator &= (TYPE& a, TYPE b) { using T = std::underlying_type_t<TYPE>; a = a & b; return a; }\
	FORCEINLINE TYPE& operator ^= (TYPE& a, TYPE b) { using T = std::underlying_type_t<TYPE>; a = a ^ b; return a; }\
	FORCEINLINE bool HaveBits(TYPE a, TYPE b) { using T = std::underlying_type_t<TYPE>; return !!(T(a) & T(b)); }

namespace Render
{
	extern RHI_API float GRHIClipZMin;
	extern RHI_API float GRHIProjectionYSign;
	extern RHI_API float GRHIVericalFlip;

	extern RHI_API bool GRHISupportMeshShader;
	extern RHI_API bool GRHISupportRayTracing;
	extern RHI_API bool GRHISupportVPAndRTArrayIndexFromAnyShaderFeedingRasterizer;

	extern RHI_API bool GRHIPrefEnabled;

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

	struct FRHIZBuffer
	{
#if 1
		static int constexpr FarPlane = 0;
		static int constexpr NearPlane = 1;
#else
		static int constexpr FarPlane = 1;
		static int constexpr NearPlane = 0;
#endif
		static bool constexpr IsInverted = FarPlane < NearPlane;
	};

	enum class ECompareFunc
	{
		Never,
		Less,
		Equal,
		LessEqual,
		Greater,
		NotEqual,
		GreaterEqual,
		Always,

		DepthNear = FRHIZBuffer::IsInverted ? Greater : Less,
		DepthNearEqual = FRHIZBuffer::IsInverted ? GreaterEqual : LessEqual,

		DepthFarther = FRHIZBuffer::IsInverted ? Less : Greater,
		DepthFartherEqual = FRHIZBuffer::IsInverted ? LessEqual : GreaterEqual,
	};


	struct EBlend
	{
		enum Factor
		{
			Zero,
			One,	
			SrcAlpha,
			OneMinusSrcAlpha,
			DestAlpha,
			OneMinusDestAlpha,
			SrcColor,
			OneMinusSrcColor,
			DestColor,
			OneMinusDestColor,
		};

		enum Operation
		{
			Add,
			Sub,
			ReverseSub ,
			Max ,
			Min ,
		};

	};


	enum class EStencil
	{
		Keep,
		Zero,
		Replace,
		Incr,
		IncrWarp,
		Decr,
		DecrWarp,
		Invert,
	};

	enum class EAccessOp
	{
		ReadOnly,
		WriteOnly,
		ReadAndWrite,
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

			Task = 6,
			Mesh = 7,

#if 0
			RayGen = 8,
			RayHit = 9,
			RayMiss = 10,
#endif

			Count,
			
			Empty = -1,
		};

		constexpr int  MaxStorageSize = 5;
	};

	enum class EFrontFace
	{
		Default,
		Inverse,
	};

	struct ESampler
	{
		enum AddressMode
		{
			Wrap,
			Mirror,
			Clamp,
			Border,
			MirrorOnce,
		};

		enum Filter
		{
			Point,
			Bilinear,
			Trilinear,
			AnisotroicPoint,
			AnisotroicLinear,
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