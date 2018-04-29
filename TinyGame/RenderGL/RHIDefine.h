#ifndef RHIDefine_h__
#define RHIDefine_h__

#include "Core/IntegerType.h"

namespace RenderGL
{


	enum ColorWriteMask
	{
		CWM_NONE = 0,
		CWM_R = 1 << 0,
		CWM_G = 1 << 1,
		CWM_B = 1 << 2,
		CWM_A = 1 << 3,
		CWM_RGB = CWM_R | CWM_G | CWM_B ,
		CWM_RGBA = CWM_RGB | CWM_A ,
	};

	enum class ECompareFun
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
		};

		enum Operation
		{
			eAdd,
			eSub,
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

	enum AccessOperator
	{
		AO_READ_ONLY,
		AO_WRITE_ONLY,
		AO_READ_AND_WRITE,
	};

	enum class PrimitiveType
	{
		eTriangleList,
		eTriangleStrip,
		eTriangleFan,
		eLineList,
		eLineStrip,
		eLineLoop,
		eQuad,
		ePoints,
	};

	enum class ELockAccess
	{
		ReadOnly,
		ReadWrite,
		WriteOnly,
	};

	class Shader
	{
	public:
		enum Type
		{
			eVertex = 0,
			ePixel = 1,
			eGeometry = 2,
			eCompute = 3,
			eHull = 4,
			eDomain = 5,

			NUM_SHADER_TYPE,
			eEmpty = -1,
		};
	};

}//namespace RenderGL

#endif // RHIDefine_h__