#include "ShapeMeshBuilder.h"

#include "Base.h"
#include "ShapeCommon.h"
#include "RenderData.h"
#include "ShapeFunction.h"

#include "ProfileSystem.h"
#include "SystemPlatform.h"
#include "Renderer/MeshUtility.h"
#include "RHI/RHICommand.h"
#include "RHI/ShaderManager.h"
#include "FileSystem.h"
#include "Misc/Format.h"
#include "RHI/GpuProfiler.h"
#include "Expression.h"
#include "Misc/DuffDevice.h"

namespace CB
{

	static RealType GEmptyTime = 0.0;
	ShapeMeshBuilder::ShapeMeshBuilder()
		:mColorMap(1000)
		,mParser()
	{
		mColorMap.addPoint(0, Color3ub(0, 7, 100));
		mColorMap.addPoint(640 / 4, Color3ub( 32, 107, 203) );
		mColorMap.addPoint(1680 / 4, Color3ub(237, 255, 255));
		mColorMap.addPoint(2570 / 4, Color3ub(255, 170, 0));
		mColorMap.addPoint(3430 / 4, Color3ub(0, 2, 0));

		mColorMap.calcColorMap();
		mColorMap.setRotion(150);

		bindTime(GEmptyTime);
	}

	void ShapeMeshBuilder::setColor(float p, float* color)
	{
		float r, g, b;
		if( p < 0.5f )
		{
			r = 0;
			g = 2 * p;
			b = 1 - 2 * p;
		}
		else
		{
			r = 2 * p - 1;
			g = 2 * (1 - p);
			b = 0;
		}
		color[0] = r;
		color[1] = g;
		color[2] = b;
	}

	void ShapeMeshBuilder::updateCurveData(ShapeUpdateContext const& context, SampleParam const& paramS)
	{
		assert(context.func->getFuncType() == TYPE_CURVE_3D);

		RenderData* data = context.data;

		unsigned flag = context.flags;

		if( flag & RUF_DATA_SAMPLE )
		{
			if( data->getVertexNum() < paramS.numData )
			{
				data->release();
				data->create(paramS.numData, 1, 0, false);
			}
			flag |= (RUF_GEOM | RUF_COLOR);
		}

		if( flag & RUF_GEOM )
		{
			float ds = paramS.getIncrement();
			float s = paramS.getRangeMin();

			Curve3DFunc* func = static_cast<Curve3DFunc*>(context.func);

			uint8* posData = data->getVertexData() + data->getPositionOffset();
			for( int i = 0; i < paramS.numData; ++i )
			{
				Vector3* pPos = (Vector3*)( posData );
				func->evalExpr(*pPos, s);
				s += ds;
				posData += data->getVertexSize();
			}
		}
		if( flag & RUF_COLOR )
		{
			uint8* colorData = data->getVertexData() + data->getColorOffset();
			for( int i = 0; i < paramS.numData; ++i )
			{
				Color4f* pColor = (Color4f*)(colorData);
				*pColor = context.color;
				colorData += data->getVertexSize();
			}
		}
	}

	void ShapeMeshBuilder::bindTime(RealType& time)
	{
		mTimePtr = &time;
		getSymbolDefine().defineVar("t", mTimePtr);
	}

#define REORDER_CACHE 0

#if SIMD_USE_AVX
#define SIMD_ELEMENT_LIST(OP)\
	OP(0)OP(1)OP(2)OP(3)OP(4)OP(5)OP(6)OP(7)
#else
#define SIMD_ELEMENT_LIST(OP)\
	OP(0)OP(1)OP(2)OP(3)
#endif

#define SET_POS_OP(INDEX)\
	*reinterpret_cast<Vector3*>(pPos) = Vector3(x[INDEX], y[INDEX], z[INDEX]);\
	pPos += vertexSize;

#define UV_X_OP(INDEX) pUV[INDEX].x,
#define UV_Y_OP(INDEX) pUV[INDEX].y,


	template< typename TSurfaceUVFunc >
	void ShapeMeshBuilder::updatePositionData_SurfaceUV(TSurfaceUVFunc* func, SampleParam const &paramU, SampleParam const &paramV, RenderData& data)
	{
		uint8* posData = data.getVertexData() + data.getPositionOffset();

		float const du = paramU.getIncrement();
		float const dv = paramV.getIncrement();
		int const vertexSize = data.getVertexSize();

		switch( func->getUsedInputMask() )
		{
		case BIT(0):
			{
				for( int i = 0; i < paramU.numData; ++i )
				{
					float u = paramU.getRangeMin() + i * du;
					Vector3 value;
					func->evalExpr(value, u, 0);
					for( int j = 0; j < paramV.numData; ++j )
					{
						float v = paramV.getRangeMin() + j * dv;
						int idx = paramU.numData * j + i;
						Vector3* pPos = (Vector3*)(posData + idx * vertexSize);
						value.y = v;
						*pPos = value;
					}
				}
			}
			break;
		case BIT(1):
			{
				for( int j = 0; j < paramV.numData; ++j )
				{
					float v = paramV.getRangeMin() + j * dv;
					Vector3 value;
					func->evalExpr(value, 0, v);
					for( int i = 0; i < paramU.numData; ++i )
					{
						float u = paramU.getRangeMin() + i * du;
						int idx = paramU.numData * j + i;
						Vector3* pPos = (Vector3*)(posData + idx * vertexSize);
						value.x = u;
						*pPos = value;
					}
				}
			}
			break;
		case BIT(0) | BIT(1):
			{
				uint8* pPos = posData;
				int numData = paramV.numData * paramU.numData;
				if (func->bSupportSIMD)
				{
#if REORDER_CACHE
					float const* pUV = (float*)Math::AlignUp<intptr_t>((intptr_t)data.getCachedData(), CacheAlign);
#else
					Vector2 const* pUV = (Vector2 const*)data.getCachedData();
#endif
					int numBlock = numData / FloatVector::Size;
					for (int i = 0; i < numBlock; ++i)
					{
#if REORDER_CACHE
						FloatVector u{ pUV , EAligned::Value };
						FloatVector v{ pUV + FloatVector::Size , EAligned::Value };
#else
						FloatVector u{ SIMD_ELEMENT_LIST(UV_X_OP) };
						FloatVector v{ SIMD_ELEMENT_LIST(UV_Y_OP) };
#endif
						FloatVector x,y,z;
						func->evalExpr(u, v, x, y, z);
						SIMD_ELEMENT_LIST(SET_POS_OP);

#if REORDER_CACHE
						pUV += 2 * FloatVector::Size;
#else
						pUV += FloatVector::Size;
#endif
					}
				}
				else
				{
					Vector2 const* pUV = (Vector2 const*)data.getCachedData();
					for (int i = 0; i < numData; ++i)
					{
						func->evalExpr(*reinterpret_cast<Vector3*>(pPos), pUV->x, pUV->y);
						pPos += vertexSize;
						++pUV;
					}
				}
			}
			break;
		default:
			{
				Vector3 value;
				func->evalExpr(value, 0, 0);

				Vector2 const* pUV = (Vector2*)data.getCachedData();
				uint8* pPos = posData;
				int numData = paramV.numData * paramU.numData;
				for (int i = 0; i < numData; ++i)
				{
					value.x = pUV->x;
					value.y = pUV->y;
					*reinterpret_cast<Vector3*>(pPos) = value;
					pPos += vertexSize;
					++pUV;
				}
			}
			break;
		}
	}
	int constexpr CacheAlign = 32;


	template< typename TSurfaceXYFunc >
	void ShapeMeshBuilder::updatePositionData_SurfaceXY(TSurfaceXYFunc* func, SampleParam const &paramU, SampleParam const &paramV, RenderData& data)
	{
		uint8* posData = data.getVertexData() + data.getPositionOffset();

		float const du = paramU.getIncrement();
		float const dv = paramV.getIncrement();
		int const vertexSize = data.getVertexSize();

		switch (func->getUsedInputMask())
		{
		case BIT(0):
			{
				if (func->bSupportSIMD)
				{
					float const* pU = (float*)Math::AlignUp<intptr_t>((intptr_t)data.getCachedData(), CacheAlign);
					int numBlock = Math::AlignCount(paramU.numData , FloatVector::Size);
					for (int i = 0; i < numBlock; ++i)
					{
						FloatVector x{ pU , EAligned::Value };
						FloatVector z;
						func->evalExpr(x, FloatVector::Zero(), z);
						for (int j = 0; j < paramV.numData; ++j)
						{
							float v = paramV.getRangeMin() + j * dv;
							int idx = paramU.numData * j + i * FloatVector::Size;
							uint8* pPos = posData + idx * vertexSize;

#define SET_POS_NY_OP(INDEX)\
	*reinterpret_cast<Vector3*>(pPos) = Vector3(x[INDEX], v, z[INDEX]);\
	pPos += vertexSize;
							SIMD_ELEMENT_LIST(SET_POS_NY_OP);
#undef  SET_POS_NY_OP
						}
						pU += FloatVector::Size;
					}
				}
				else
				{
					for (int i = 0; i < paramU.numData; ++i)
					{
						float u = paramU.getRangeMin() + i * du;
						Vector3 value;
						func->evalExpr(value, u, 0);

						for (int j = 0; j < paramV.numData; ++j)
						{
							float v = paramV.getRangeMin() + j * dv;
							int idx = paramU.numData * j + i;
							Vector3* pPos = (Vector3*)(posData + idx * vertexSize);
							value.y = v;
							*pPos = value;
						}
					}
				}
			}
			break;
		case BIT(1):
			{
				if (func->bSupportSIMD)
				{
					float const* pV = (float*)Math::AlignUp<intptr_t>((intptr_t)data.getCachedData(), CacheAlign);
					int numBlock = Math::AlignCount(paramV.numData, FloatVector::Size);
					for (int j = 0; j < numBlock; ++j)
					{
						FloatVector y{ pV , EAligned::Value };
						FloatVector z;
						func->evalExpr(FloatVector::Zero(), y,  z);
						for (int i = 0; i < paramU.numData; ++i)
						{
							float u = paramU.getRangeMin() + i * du;
							int idx = paramU.numData * FloatVector::Size * j + i;
							uint8* pPos = posData + idx * vertexSize;

	#define SET_POS_NX_OP(INDEX)\
		*reinterpret_cast<Vector3*>(pPos) = Vector3(u, y[INDEX], z[INDEX]);\
		pPos += paramU.numData * vertexSize;
							SIMD_ELEMENT_LIST(SET_POS_NX_OP);
	#undef  SET_POS_NX_OP
						}
						pV += FloatVector::Size;
					}
				}
				else
				{
					for (int j = 0; j < paramV.numData; ++j)
					{
						float v = paramV.getRangeMin() + j * dv;

						Vector3 value;
						func->evalExpr(value, 0, v);

						for (int i = 0; i < paramU.numData; ++i)
						{
							float u = paramU.getRangeMin() + i * du;

							int idx = paramU.numData * j + i;
							Vector3* pPos = (Vector3*)(posData + idx * vertexSize);
							value.x = u;
							*pPos = value;
						}
					}
				}
			}
			break;
		case BIT(0) | BIT(1):
			{
				uint8* pPos = posData;
				int numData = paramV.numData * paramU.numData;
				if (func->bSupportSIMD)
				{
#if REORDER_CACHE
					float const* pUV = (float*)Math::AlignUp<intptr_t>((intptr_t)data.getCachedData(), CacheAlign);
#else
					Vector2 const* pUV = (Vector2 const*)data.getCachedData();
#endif


#if EBC_USE_VALUE_BUFFER
					FloatVector valueBuffer[64];
					if constexpr (std::is_same_v< TSurfaceXYFunc, SurfaceXYFunc>)
					{
						func->mExpr.GetEvalResource<ExecutableCode>().initValueBuffer(valueBuffer, 2);
					}
#endif

					int numBlock = Math::AlignCount(numData, FloatVector::Size);
					for (int i = 0; i < numBlock; ++i)
					{
#if REORDER_CACHE
						FloatVector x{ pUV , EAligned::Value };
						FloatVector y{ pUV + FloatVector::Size , EAligned::Value };
#else
						FloatVector x{ SIMD_ELEMENT_LIST(UV_X_OP) };
						FloatVector y{ SIMD_ELEMENT_LIST(UV_Y_OP) };
#endif

						FloatVector z;
#if EBC_USE_VALUE_BUFFER
						if constexpr (std::is_same_v< TSurfaceXYFunc, SurfaceXYFunc>)
						{
							valueBuffer[0] = x;
							valueBuffer[1] = y;
							func->evalExpr(TArrayView<FloatVector const>(valueBuffer), z);
						}
						else
#endif
						{
							func->evalExpr(x, y, z);
						}

						SIMD_ELEMENT_LIST(SET_POS_OP);

#if REORDER_CACHE
						pUV += 2 * FloatVector::Size;
#else
						pUV += FloatVector::Size;
#endif
					}
				}
				else
				{
					Vector2 const* pUV = (Vector2 const*)data.getCachedData();

#if EBC_USE_VALUE_BUFFER
					RealType valueBuffer[64];
					if constexpr (std::is_same_v< TSurfaceXYFunc, SurfaceXYFunc>)
					{
						func->mExpr.GetEvalResource<ExecutableCode>().initValueBuffer(valueBuffer, 2);

#if 0
						RealType z;

#define EVAL_OP 		valueBuffer[0] = pUV->x;\
						valueBuffer[1] = pUV->y;\
						func->evalExpr(TArrayView<RealType const>(valueBuffer), z);\
						*reinterpret_cast<Vector3*>(pPos) = Vector3(pUV->x, pUV->y, z);\
						pPos += vertexSize;\
						++pUV;\

						DUFF_DEVICE_4(numData, EVAL_OP);
#undef EVAL_OP
#else
						for (int i = 0; i < numData; ++i)
						{
							valueBuffer[0] = pUV->x;
							valueBuffer[1] = pUV->y;
							RealType z;
							func->evalExpr(TArrayView<RealType const>(valueBuffer), z);
							*reinterpret_cast<Vector3*>(pPos) = Vector3(pUV->x, pUV->y, z);
							pPos += vertexSize;
							++pUV;
						}
#endif
					}
					else
#endif
					{
#if 1
#define EVAL_OP 	func->evalExpr(*(Vector3*)pPos, pUV->x, pUV->y);pPos += vertexSize; ++pUV;
						DUFF_DEVICE_4(numData, EVAL_OP);
#undef EVAL_OP
#else
						for (int i = 0; i < numData; ++i)
						{
							func->evalExpr(*reinterpret_cast<Vector3*>(pPos), pUV->x, pUV->y);
							pPos += vertexSize;
							++pUV;
						}
#endif
					}
				}
			}
			break;
		default:
			{
				Vector3 value;
				func->evalExpr(value, 0, 0);
				Vector2 const* pUV = (Vector2*)data.getCachedData();
				uint8* pPos = posData;
				int numData = paramV.numData * paramU.numData;
				for (int i = 0; i < numData; ++i)
				{
					value.x = pUV->x;
					value.y = pUV->y;
					*reinterpret_cast<Vector3*>(pPos) = value;
					pPos += vertexSize;
					++pUV;
				}
			}
			break;
		}
	}



	void GenerateGirdMeshPos(Vector2* pUV , SampleParam const& paramU, SampleParam const& paramV)
	{
		float du = paramU.getIncrement();
		float dv = paramV.getIncrement();
		for (int j = 0; j < paramV.numData; ++j)
		{
			float v = paramV.getRangeMin() + j * dv;
			for (int i = 0; i < paramU.numData; ++i)
			{
				float u = paramU.getRangeMin() + i * du;
				int idx = paramU.numData * j + i;
				pUV[idx] = Vector2(u, v);
			}
		}
	}


	void FillGirdMeshIndices(uint32* pIndexData, int nx, int ny)
	{
		uint32*  pIndex = pIndexData;

		for (int j = 0; j < ny - 1; ++j)
		{
			for (int i = 0; i < nx - 1; ++i)
			{
				int index = nx * j + i;
				int indexN = index + nx;

				pIndex[0] = index;
				pIndex[1] = index  + 1;
				pIndex[2] = indexN + 1;

				pIndex[3] = indexN + 1;
				pIndex[4] = indexN;
				pIndex[5] = index;

				pIndex += 6;
			}
		}
	}


	void ShapeMeshBuilder::updateSurfaceData(ShapeUpdateContext const& context, SampleParam const& paramU, SampleParam const& paramV)
	{
		PROFILE_ENTRY("UpdateSurfaceData", "CB");

		CHECK(isSurface(context.func->getFuncType()));

		if (context.func->getEvalType() == EEvalType::GPU)
		{
			updateSurfaceDataGPU(context, paramU , paramV);
			return;
		}

		RenderData* data = context.data;

		unsigned flags = context.flags;
		int vertexNum = paramU.numData * paramV.numData;


		if( flags & (RUF_DATA_SAMPLE | RUF_DATA_TYPE) )
		{
			int indexNum = 6 * (paramU.numData - 1) * (paramV.numData - 1);
			bool bSupportSIMD = context.func->bSupportSIMD;
			if ( (flags & RUF_DATA_TYPE) || ( data->getVertexNum() != vertexNum || data->getIndexNum() != indexNum ) )
			{
				data->release();
				data->create(vertexNum, bSupportSIMD ? FloatVector::Size : 1, indexNum, true, true);
			}
			flags |= (RUF_GEOM | RUF_COLOR | RUF_INDEX_DATA | RUF_CACHE_DATA);
		}

		if (flags & RUF_INDEX_DATA)
		{
			uint32*  pIndexData;
			if (data->resource)
			{
				pIndexData = (uint32*)RHILockBuffer(data->resource->indexBuffer, Render::ELockAccess::WriteDiscard);
			}
			else
			{
				pIndexData = data->getIndexData();
			}

			FillGirdMeshIndices(pIndexData, paramU.numData, paramV.numData);

			if (data->resource)
			{
				RHIUnlockBuffer(data->resource->indexBuffer);
			}
		}

		if (flags & RUF_CACHE_DATA)
		{
			bool bSupportSIMD = context.func->bSupportSIMD;
			int num = vertexNum;

			bool bNeedReorderCache = false;
			if (bSupportSIMD)
			{
				num = Math::AlignUp(num, FloatVector::Size);
#if REORDER_CACHE
				bNeedReorderCache = (context.func->getUsedInputMask() == BIT(0)|BIT(1) ) || (context.func->getUsedInputMask() == BIT(0));
#endif
			}


			if (REORDER_CACHE && bNeedReorderCache)
			{
				// x y x y x y x y -> x x x x y y y y
				TArray<Vector2> tempData;
				tempData.resize(num);

				Vector2* pUV = tempData.data();
				GenerateGirdMeshPos(tempData.data(), paramU , paramV);

				data->setCachedDataSize(num * sizeof(Vector2) + CacheAlign - 1);
				float* pValue = (float*)Math::AlignUp<intptr_t>((intptr_t)data->getCachedData(), CacheAlign);
				int numBlock = num / FloatVector::Size;

				for (int i = 0; i < numBlock; ++i)
				{
					for( int n = 0; n < FloatVector::Size; ++n )	
					{
						*pValue = pUV[n].x;
						++pValue;
					}
					for (int n = 0; n < FloatVector::Size; ++n)
					{
						*pValue = pUV[n].y;
						++pValue;
					}
					pUV += FloatVector::Size;
				}
			}
			else
			{
				switch (context.func->getUsedInputMask())
				{
				case BIT(0):
					{
						data->setCachedDataSize( Math::AlignUp(paramU.getNumData(), FloatVector::Size) * sizeof(float) + CacheAlign - 1);
						float* pU = (float*)Math::AlignUp<intptr_t>((intptr_t)data->getCachedData(), CacheAlign);
						float du = paramU.getIncrement();
						for (int i = 0; i < paramU.numData; ++i)
						{
							float u = paramU.getRangeMin() + i * du;
							pU[i] = u;
						}
					}
					break;
				case BIT(1):
					{
						data->setCachedDataSize( Math::AlignUp(paramV.getNumData(), FloatVector::Size) * sizeof(float) + CacheAlign - 1);
						float* pV = (float*)Math::AlignUp<intptr_t>((intptr_t)data->getCachedData(), CacheAlign);
						float dv = paramV.getIncrement();
						for (int i = 0; i < paramV.numData; ++i)
						{
							float v = paramV.getRangeMin() + i * dv;
							pV[i] = v;
						}
					}
					break;
				default:
					{
						data->setCachedDataSize(num * sizeof(Vector2));

						Vector2* pUV = (Vector2*)data->getCachedData();
						GenerateGirdMeshPos(pUV, paramU, paramV);
					}
					break;
				}
			}
		}

		if (flags & (RUF_COLOR | RUF_GEOM) )
		{
			data->lockVertexResource();
		}

		if( flags & RUF_GEOM )
		{
			if( context.func->getFuncType() == TYPE_SURFACE_UV )
			{
				SurfaceUVFunc* func = static_cast<SurfaceUVFunc*>(context.func);

				switch(context.func->getEvalType())
				{
				case EEvalType::Native:
					{
						NativeSurfaceUVFunc* func = static_cast<NativeSurfaceUVFunc*>(context.func);
						updatePositionData_SurfaceUV(func, paramU, paramV, *data);
					}
					break;
				case EEvalType::CPU:
					{
						SurfaceUVFunc* func = static_cast<SurfaceUVFunc*>(context.func);
						updatePositionData_SurfaceUV(func, paramU, paramV, *data);
					}
					break;
				}
			}
			else if( context.func->getFuncType() == TYPE_SURFACE_XY )
			{
				PROFILE_ENTRY("Update Position", "CB");

				switch(context.func->getEvalType())
				{
				case EEvalType::Native:
					{
						NativeSurfaceXYFunc* func = static_cast<NativeSurfaceXYFunc*>(context.func);
						updatePositionData_SurfaceXY(func, paramU, paramV, *data);
					}
					break;
				case EEvalType::CPU:
					{
						SurfaceXYFunc* func = static_cast<SurfaceXYFunc*>(context.func);
						updatePositionData_SurfaceXY(func, paramU, paramV, *data);
					}
					break;
				}
			}

			if( data->getNormalOffset() != INDEX_NONE )
			{
				uint32* pIndexData;
				if (data->resource)
				{
					pIndexData = (uint32*)RHILockBuffer(data->resource->indexBuffer, Render::ELockAccess::ReadOnly);
				}
				else
				{
					pIndexData = data->getIndexData();
				}
				using namespace Render;
				VertexElementReader posReader{ data->getVertexData() + data->getPositionOffset() , data->getVertexSize() };
				VertexElementWriter normalWriter{ data->getVertexData() + data->getNormalOffset() , data->getVertexSize() };

				PROFILE_ENTRY("Update Normal", "CB");
				if (context.func->getFuncType() == TYPE_SURFACE_XY && false)
				{
					uint32 numTriangles = data->getIndexNum() / 3;
					float du = paramU.getIncrement();
					float dv = paramV.getIncrement();
					for (int i = 0; i < vertexNum; ++i)
					{
						normalWriter[i] = Vector3::Zero();
					}

					for (int i = 0; i < numTriangles; ++i)
					{
						float sign = (i & 0x1) ? -1.0f : 1.0f;
						uint32 i0 = pIndexData[0];
						uint32 i1 = pIndexData[1];
						uint32 i2 = pIndexData[2];
						pIndexData += 3;

						Vector3 const& p0 = posReader[i0];
						Vector3 const& p1 = posReader[i1];
						Vector3 const& p2 = posReader[i2];

						CHECK(p0.y == p1.y);
						float Nx = sign * (p1.z - p0.z) / du;
						CHECK(p1.x == p2.x);
						float Ny = sign * (p2.z - p1.z) / dv;

						SIMD::SVector3 vNormal = SIMD::SVector3(Nx, Ny, 1.0f);
						vNormal.normalize();
						Vector3 normal = { vNormal.v[0], vNormal.v[1], vNormal.v[2] };
						normalWriter[i0] += normal;
						normalWriter[i1] += normal;
						normalWriter[i2] += normal;
					}
				}
				else
				{

					uint8* posData = data->getVertexData() + data->getPositionOffset();
					uint8* normalData = data->getVertexData() + data->getNormalOffset();

					MeshUtility::FillNormal_TriangleList(posReader , normalWriter,
						data->getVertexNum(), pIndexData, data->getIndexNum(), true, false
					);				
				}

				if (data->resource)
				{
					RHIUnlockBuffer(data->resource->indexBuffer);
				}

			}

			flags |= RUF_COLOR;
		}

		if( flags & RUF_COLOR )
		{
			PROFILE_ENTRY("Update Color", "CB");

			int vertexSize = data->getVertexSize();
			uint8* colorData = data->getVertexData() + data->getColorOffset();
			uint8* posData = data->getVertexData() + data->getPositionOffset();
			for( int i = 0; i < vertexNum; ++i )
			{
				Color4f* pColor = (Color4f*)(colorData);
				BYTE temp[3];
#if 0
				float z = reinterpret_cast<Vector3*>(posData)->z;
				float zMax = 20;
				float zMin = 10;
				float posColor = (z - zMin) / (zMax - zMin);
				Color3f color;
				mColorMap.getColor(posColor, color);
				*pColor = Color4f(color, context.color.a);
				posData += vertexSize;
#else

				*pColor = context.color;
#endif
				colorData += vertexSize;

			}
		}

		if (flags & (RUF_COLOR | RUF_GEOM))
		{
			data->unlockVertexResource();
		}
	}

	using namespace Render;

	class GenNormalCS : public GlobalShader
	{
	public:
		DECLARE_SHADER(GenNormalCS, Global);

		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			mParamParamsData.bind(parameterMap, NormalGenParamsData::GetStructInfo().getParameterName(EShaderResourceType::Uniform));
			mParamVertexOutput.bind(parameterMap, MAKE_STRUCTUREED_BUFFER_INFO(VertexOutput).getParameterName(EShaderResourceType::Storage));
			mParamVertexInput.bind(parameterMap, MAKE_STRUCTUREED_BUFFER_INFO(VertexInput).getParameterName(EShaderResourceType::Storage));
			mParamTriangleIndices.bind(parameterMap, MAKE_STRUCTUREED_BUFFER_INFO(TriangleIndices).getParameterName(EShaderResourceType::Storage));
#if USE_SHARE_TRIANGLE_INFO
			mParamTriangleIdList.bind(parameterMap, MAKE_STRUCTUREED_BUFFER_INFO(TriangleIdList).getParameterName(EShaderResourceType::Storage));
			mParamShareTriangleInfos.bind(parameterMap, MAKE_STRUCTUREED_BUFFER_INFO(ShareTriangleInfos).getParameterName(EShaderResourceType::Storage));
#endif
		}

		void setParameters(RHICommandList& commandList, RHIBuffer& ParamsBuffer, RHIBuffer& vertexBuffer, RHIBuffer& indexBuffer)
		{
			setUniformBuffer(commandList, mParamParamsData, ParamsBuffer);
			setStorageBuffer(commandList, mParamVertexInput, vertexBuffer, EAccessOp::ReadOnly);
			setStorageBuffer(commandList, mParamVertexOutput, vertexBuffer, EAccessOp::ReadAndWrite);
			setStorageBuffer(commandList, mParamTriangleIndices, indexBuffer, EAccessOp::ReadOnly);
		}

		ShaderParameter mParamParamsData;
		ShaderParameter mParamVertexOutput;
		ShaderParameter mParamVertexInput;
		ShaderParameter mParamTriangleIndices;

#if USE_SHARE_TRIANGLE_INFO
		void setSharedInfoParameters(RHICommandList& commandList, RHIBuffer& triangleIdBuffer, RHIBuffer& sharedTriangleInfoBuffer)
		{
			setStorageBuffer(commandList, mParamTriangleIdList, triangleIdBuffer, EAccessOp::ReadOnly);
			setStorageBuffer(commandList, mParamShareTriangleInfos, sharedTriangleInfoBuffer, EAccessOp::ReadOnly);
		}
		ShaderParameter mParamTriangleIdList;
		ShaderParameter mParamShareTriangleInfos;
#endif



		static void SetupShaderCompileOption(ShaderCompileOption& option) 
		{
			option.addDefine(SHADER_PARAM(USE_SHARE_TRIANGLE_INFO), USE_SHARE_TRIANGLE_INFO);
#if USE_SHARE_TRIANGLE_INFO
			option.addDefine(SHADER_PARAM(USE_COMPACTED_SHARE_TRIANGLE_INFO), USE_COMPACTED_SHARE_TRIANGLE_INFO);
#endif
		}

		static char const* GetShaderFileName()
		{
			return "Shader/Game/CurveMeshGenNormal";
		}
	};

	IMPLEMENT_SHADER(GenNormalCS, EShader::Compute, SHADER_ENTRY(MainCS));


	class GenVertexCS : public Shader
	{
	public:

		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			mParamParamsData.bind(parameterMap, VertexGenParamsData::GetStructInfo().getParameterName(EShaderResourceType::Uniform));
			mParamVertexOutput.bind(parameterMap, MAKE_STRUCTUREED_BUFFER_INFO(VertexOutput).getParameterName(EShaderResourceType::Storage));
		}

		void setParameters(RHICommandList& commandList, RHIBuffer& ParamsBuffer, RHIBuffer& vertexBuffer)
		{
			setUniformBuffer(commandList, mParamParamsData, ParamsBuffer);
			setStorageBuffer(commandList, mParamVertexOutput, vertexBuffer);
		}

		ShaderParameter mParamParamsData;
		ShaderParameter mParamVertexOutput;

		static char const* GetShaderFileName()
		{
			return "Shader/Game/CurveMeshGenVertexTemplate";
		}

		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const enties[] =
			{
				{ EShader::Compute , SHADER_PARAM(GenVertexCS) } ,
			};
			return enties;
		};
	};

	void ShapeMeshBuilder::initializeRHI()
	{
		using namespace Render;
		mVertexGenParamBuffer.initializeResource(1, EStructuredBufferType::Const);
		mNoramlGenParamBuffer.initializeResource(1, EStructuredBufferType::Const);
		mShaderGenNormal = ShaderManager::Get().getGlobalShaderT<GenNormalCS>();
	}

	void ShapeMeshBuilder::releaseRHI()
	{
		mVertexGenParamBuffer.releaseResource();
		mNoramlGenParamBuffer.releaseResource();
		mShaderGenNormal = nullptr;
	}


	void ShapeMeshBuilder::updateSurfaceDataGPU(ShapeUpdateContext const& context, SampleParam const& paramU, SampleParam const& paramV)
	{
		using namespace Render;


		PROFILE_ENTRY("UpdateSurfaceData", "CB");

		CHECK(isSurface(context.func->getFuncType()));


		RenderData* data = context.data;
		if (data->resource == nullptr)
		{
			data->resource = new RenderResource;
		}
		auto& resource = *data->resource;

		unsigned flags = context.flags;
		int vertexNum = paramU.numData * paramV.numData;

		if (flags & (RUF_DATA_SAMPLE|RUF_DATA_TYPE))
		{
			int indexNum = 6 * (paramU.numData - 1) * (paramV.numData - 1);

			if (!resource.vertexBuffer.isValid() || resource.vertexBuffer->getNumElements() != vertexNum)
			{
				data->mbNormalOwned = true;
				resource.vertexBuffer = RHICreateVertexBuffer(10 * sizeof(float), vertexNum, BCF_DefalutValue | BCF_CreateUAV);
			}

			flags |= (RUF_GEOM | RUF_COLOR | RUF_INDEX_DATA);
		}

		if (flags & RUF_INDEX_DATA)
		{
			int indexNum = 6 * (paramU.numData - 1) * (paramV.numData - 1);
			TArray< uint32 > indices;
			indices.resize(indexNum);

			uint32*  pIndexData = indices.data();
			FillGirdMeshIndices(pIndexData, paramU.numData, paramV.numData);
			resource.indexBuffer = RHICreateIndexBuffer(indexNum, true, BCF_DefalutValue | BCF_CreateUAV, indices.data());

#if USE_SHARE_TRIANGLE_INFO

			TArray<MeshUtility::SharedTriangleInfo> sharedTriangleInfos;
			TArray<uint32> triangleIds;
			MeshUtility::BuildVertexSharedTriangleInfo(indices.data(), indexNum / 3 , vertexNum, sharedTriangleInfos, triangleIds);

#if USE_COMPACTED_SHARE_TRIANGLE_INFO
			TArray<uint32> compactedSharedTriangleInfos;
			compactedSharedTriangleInfos.resize(sharedTriangleInfos.size());
			for (int i = 0; i < compactedSharedTriangleInfos.size(); ++i)
			{
				auto const& info = sharedTriangleInfos[i];
				CHECK(info.count < (1u << 4));
				compactedSharedTriangleInfos[i] = (info.offset << 4) | info.count;
			}
			resource.sharedTriangleInfoBuffer = RHICreateBuffer(sizeof(uint32), compactedSharedTriangleInfos.size(), BCF_CreateUAV, compactedSharedTriangleInfos.data());
#else
			resource.sharedTriangleInfoBuffer = RHICreateBuffer(sizeof(uint32), 2 * sharedTriangleInfos.size(), BCF_CreateUAV, sharedTriangleInfos.data());
#endif
			resource.triangleIdBuffer = RHICreateBuffer( sizeof(uint32) , triangleIds.size(), BCF_CreateUAV, triangleIds.data() );
#endif
		}

		if (flags & RUF_GEOM)
		{
			GenVertexCS* shader = nullptr;
			if (context.func->getFuncType() == TYPE_SURFACE_UV)
			{
				auto myFunc = static_cast<SurfaceUVFunc*>(context.func);
				shader = &myFunc->getEvalShader<GenVertexCS>();
			}
			else if (context.func->getFuncType() == TYPE_SURFACE_XY)
			{
				auto myFunc = static_cast<SurfaceXYFunc*>(context.func);
				shader = &myFunc->getEvalShader<GenVertexCS>();
			}

			if (shader)
			{
				GPU_PROFILE("Update Position");

				VertexGenParamsData params;
				params.delata = Vector2(paramU.getIncrement(), paramV.getIncrement());
				params.gridCountU = paramU.getNumData();
				params.vertexCount = resource.vertexBuffer->getNumElements();
				params.vertexStride = resource.vertexBuffer->getElementSize() / sizeof(float);
				params.offset = Vector2(paramU.getRangeMin(), paramV.getRangeMin());
				params.color = context.color;
				params.time = *mTimePtr;
				mVertexGenParamBuffer.updateBuffer(params);

				auto& commandList = RHICommandList::GetImmediateList();
				RHISetComputeShader(commandList, shader->getRHI());
				shader->setParameters(commandList, *mVertexGenParamBuffer.getRHI(), *resource.vertexBuffer);
				RHIDispatchCompute(commandList, Math::AlignCount(vertexNum, 16), 1, 1);
			}

			if (data->getNormalOffset() != INDEX_NONE)
			{
				GPU_PROFILE("Update Normal");

				NormalGenParamsData params;
#if USE_SHARE_TRIANGLE_INFO
				params.totalCount = vertexNum;
#else
				params.totalCount = resource.indexBuffer->getNumElements() / 3;
#endif
				params.normalOffset = 7;
				params.vertexStride = resource.vertexBuffer->getElementSize() / sizeof(float);
				mNoramlGenParamBuffer.updateBuffer(params);

				auto& commandList = RHICommandList::GetImmediateList();
				RHISetComputeShader(commandList, mShaderGenNormal->getRHI());
				mShaderGenNormal->setParameters(commandList, *mNoramlGenParamBuffer.getRHI(), *resource.vertexBuffer, *resource.indexBuffer);
#if USE_SHARE_TRIANGLE_INFO
				mShaderGenNormal->setSharedInfoParameters(commandList, *resource.triangleIdBuffer, *resource.sharedTriangleInfoBuffer);
#endif
				RHIDispatchCompute(commandList, Math::AlignCount(params.totalCount, 16u), 1, 1);
			}
		}

		if (flags & RUF_COLOR)
		{

		}
	}


	bool ShapeMeshBuilder::compileFuncShader(ShapeFuncBase& func)
	{
		CHECK(func.getEvalType() == EEvalType::GPU);

		ShaderCompileOption option;
		option.addDefine(SHADER_PARAM(FUNC_TYPE), func.getFuncType());
		ShaderEntryInfo entries[] =
		{
			{ EShader::Compute , SHADER_PARAM(GenVertexCS) } ,
		};

		if (func.getFuncType() == TYPE_SURFACE_XY)
		{
			auto& myFunc = static_cast<SurfaceXYFunc&>(func);
			if (!LoadRuntimeShader(myFunc.mExpr.GetOrCreateEvalResource<GenVertexCS>(), { StringView(myFunc.mExpr.getExprString()) }, option))
			{
				return false;
			}
		}
		else if (func.getFuncType() == TYPE_SURFACE_UV)
		{
			auto& myFunc = static_cast<SurfaceUVFunc&>(func);
			if (!LoadRuntimeShader(myFunc.mAixsExpr[0].GetOrCreateEvalResource<GenVertexCS>(), 
				{ StringView(myFunc.mAixsExpr[0].getExprString()), StringView(myFunc.mAixsExpr[1].getExprString()), StringView(myFunc.mAixsExpr[2].getExprString()) }, option))
			{
				return false;
			}
		}
		else
		{
			return false;
		}

		return true;
	}

	bool ShapeMeshBuilder::parseFunction(ShapeFuncBase& func)
	{
		{
#if USE_PARALLEL_UPDATE
			Mutex::Locker locker(mParserLock);
#endif
			if (!func.parseExpression(mParser))
				return false;
		}

		if (func.getEvalType() == EEvalType::GPU)
		{	
			return compileFuncShader(func);
		}

		return true;
	}


	static bool calcDiv(float v, float v0, float v1, float& out) // throw( RealNanException )
	{
		float dv = v1 - v0;
		if( dv == 0 )
			return false;
		out = (v - v0) / dv;
		return true;
	}

}//namespace CB