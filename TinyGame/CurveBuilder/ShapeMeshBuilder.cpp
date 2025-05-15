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

namespace CB
{

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

		SymbolTable& table = mParser.getSymbolDefine();
		table.defineVar("t", &mVarTime);
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

	int constexpr DUFF_BLOCK_SIZE = 8;
#define DUFF_DEVICE( DIM , OP )\
	{\
		int blockCount = ((DIM) + DUFF_BLOCK_SIZE - 1) / DUFF_BLOCK_SIZE;\
		switch ((DIM) % DUFF_BLOCK_SIZE)\
		{\
		case 0: do { OP;\
		case 7: OP;\
		case 6: OP;\
		case 5: OP;\
		case 4: OP;\
		case 3: OP;\
		case 2: OP;\
		case 1: OP;\
			} while (--blockCount > 0);\
		}\
	}

	template< typename TSurfaceUVFunc >
	void ShapeMeshBuilder::updatePositionData_SurfaceUV(TSurfaceUVFunc* func, SampleParam const &paramU, SampleParam const &paramV, RenderData& data)
	{
		uint8* posData = data.getVertexData() + data.getPositionOffset();

		float du = paramU.getIncrement();
		float dv = paramV.getIncrement();

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
						Vector3* pPos = (Vector3*)(posData + idx * data.getVertexSize());
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
						Vector3* pPos = (Vector3*)(posData + idx * data.getVertexSize());
						*pPos = value;
					}
				}
			}
			break;
		case BIT(0) | BIT(1):
			{
				for( int j = 0; j < paramV.numData; ++j )
				{
					float v = paramV.getRangeMin() + j * dv;
					for( int i = 0; i < paramU.numData; ++i )
					{
						float u = paramU.getRangeMin() + i * du;		
						int idx = paramU.numData * j + i;
						Vector3* pPos = (Vector3*)(posData + idx * data.getVertexSize());
						func->evalExpr(*pPos, u, v);
					}
				}
			}
			break;
		default:
			{
				Vector3 value;
				func->evalExpr(value, 0, 0);
				for( int j = 0; j < paramV.numData; ++j )
				{
					float v = paramV.getRangeMin() + j * dv;
					for( int i = 0; i < paramU.numData; ++i )
					{
						float u = paramU.getRangeMin() + i * du;
						int idx = paramU.numData * j + i;
						Vector3* pPos = (Vector3*)(posData + idx * data.getVertexSize());
						*pPos = value;
					}
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

		float du = paramU.getIncrement();
		float dv = paramV.getIncrement();

		switch (func->getUsedInputMask())
		{
		case BIT(0):
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
						Vector3* pPos = (Vector3*)(posData + idx * data.getVertexSize());
						pPos->setValue(u, v, value.z);
					}
				}
			}
			break;
		case BIT(1):
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
						Vector3* pPos = (Vector3*)(posData + idx * data.getVertexSize());
						pPos->setValue(u, v, value.z);
					}
				}
			}
			break;
		case BIT(0) | BIT(1):
			{
#define REORDER_CACHE 0
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
						FloatVector x{ pUV , EAligned::Value };
						FloatVector y{ pUV + FloatVector::Size , EAligned::Value };
#else
#if SIMD_USE_AVX
						FloatVector x{ pUV[0].x, pUV[1].x, pUV[2].x, pUV[3].x, pUV[4].x, pUV[5].x, pUV[6].x, pUV[7].x };
						FloatVector y{ pUV[0].y, pUV[1].y, pUV[2].y, pUV[3].y, pUV[4].y, pUV[5].y, pUV[6].y, pUV[7].y };
#else
						FloatVector x{ pUV[0].x , pUV[1].x, pUV[2].x, pUV[3].x };
						FloatVector y{ pUV[0].y , pUV[1].y, pUV[2].y, pUV[3].y };
#endif
#endif

						FloatVector z;
						func->evalExpr(x, y, z);
#define SET_POS(INDEX)\
	((Vector3*)pPos)->setValue(x[INDEX], y[INDEX], z[INDEX]);\
	pPos += data.getVertexSize();

						SET_POS(0);
						SET_POS(1);
						SET_POS(2);
						SET_POS(3);
#if SIMD_USE_AVX
						SET_POS(4);
						SET_POS(5);
						SET_POS(6);
						SET_POS(7);
#endif

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
#if 0
#define OP 	func->evalExpr(*(Vector3*)pPos, pUV->x, pUV->y);pPos += data.getVertexSize(); ++pUV;
					DUFF_DEVICE(numData, OP);
#undef OP
#else
					for (int i = 0; i < numData; ++i)
					{
						func->evalExpr(*(Vector3*)pPos, pUV->x, pUV->y);
						pPos += data.getVertexSize();
						++pUV;
					}
#endif
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
					((Vector3*)pPos)->setValue(pUV->x, pUV->y, value.z);
					pPos += data.getVertexSize();
					++pUV;
				}
			}
			break;
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

		bool bUpdateIndices = false;
		bool bUpdateCachedData = false;
		if( flags & RUF_DATA_SAMPLE )
		{
			int indexNum = 6 * (paramU.numData - 1) * (paramV.numData - 1);
			bool bSupportSIMD = context.func->bSupportSIMD;
			if( data->getVertexNum() != vertexNum || data->getIndexNum() != indexNum )
			{
				data->release();
				data->create(vertexNum, bSupportSIMD ? FloatVector::Size : 1, indexNum, true);
			}
			flags |= (RUF_GEOM | RUF_COLOR);
			bUpdateIndices = true;
			bUpdateCachedData = true;
		}

		uint32*  pIndexData = data->getIndexData();
		if (bUpdateIndices)
		{
			int nu = paramU.numData;
			int nv = paramV.numData;

			uint32*  pIndex = pIndexData;
			for (int i = 0; i < nu - 1; ++i)
			{
				for (int j = 0; j < nv - 1; ++j)
				{
					int index = nv * i + j;

					pIndex[0] = index;
					pIndex[1] = index + 1;
					pIndex[2] = index + nv + 1;

					pIndex[3] = index;
					pIndex[4] = index + nv + 1;
					pIndex[5] = index + nv;

					pIndex += 6;
				}
			}
		}

		if (bUpdateCachedData)
		{
			bool bSupportSIMD = context.func->bSupportSIMD;
			int num = vertexNum;
			if (bSupportSIMD)
			{
				num = Math::AlignUp(num, FloatVector::Size);
#if REORDER_CACHE
				TArray<Vector2> tempData;
				tempData.resize(num);

				Vector2* pUV = tempData.data();

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


				data->setCachedDataSize(num * sizeof(Vector2) + CacheAlign - 1);

				int const Align = 32;
				float* pValue = (float*)Math::AlignUp<intptr_t>((intptr_t)data->getCachedData(), Align);
				pUV = tempData.data();
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
#else
			}
#endif
			{
				data->setCachedDataSize(num * sizeof(Vector2));

				Vector2* pUV = (Vector2*)data->getCachedData();

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
				using namespace Render;
				PROFILE_ENTRY("Update Normal", "CB");

				uint8* posData = data->getVertexData() + data->getPositionOffset();
				uint8* normalData = data->getVertexData() + data->getNormalOffset();

				MeshUtility::FillNormal_TriangleList(
					VertexElementReader{ posData , data->getVertexSize() }, 
					VertexElementWriter{ normalData , data->getVertexSize() }, 
					data->getVertexNum(), data->getIndexData(), data->getIndexNum(), true
				);
			}
		}

		if( flags & RUF_COLOR )
		{
			uint8* colorData = data->getVertexData() + data->getColorOffset();
			for( int i = 0; i < vertexNum; ++i )
			{
				Color4f* pColor = (Color4f*)(colorData);
				//BYTE temp[3];
				//float z = pVertex[3*i+2].z;
				//m_ColorMap.getColor((z-datamin)/(datamax-datamin),temp);
				//setColor((z-datamin)/(datamax-datamin),&m_pColorData[4*i]);

				*pColor = context.color;
				colorData += data->getVertexSize();
			}
		}

	}

	void ShapeMeshBuilder::initializeRHI()
	{
		using namespace Render;
		mGenParamBuffer.initializeResource(1, EStructuredBufferType::Const);
	}

	void ShapeMeshBuilder::releaseRHI()
	{

	}


	void ShapeMeshBuilder::updateSurfaceDataGPU(ShapeUpdateContext const& context, SampleParam const& paramU, SampleParam const& paramV)
	{
		using namespace Render;

		PROFILE_ENTRY("UpdateSurfaceData", "CB");

		CHECK(isSurface(context.func->getFuncType()));

		RenderData* data = context.data;
		unsigned flags = context.flags;
		int vertexNum = paramU.numData * paramV.numData;

		bool bUpdateIndices = false;
		if (flags & RUF_DATA_SAMPLE)
		{
			int indexNum = 6 * (paramU.numData - 1) * (paramV.numData - 1);

			if (!data->vertexBuffer.isValid() || data->vertexBuffer->getNumElements() != vertexNum)
			{
				data->mbNormalOwned = false;
				data->vertexBuffer = RHICreateVertexBuffer(7 * sizeof(float), vertexNum, BCF_DefalutValue | BCF_CreateUAV);
			}

			flags |= (RUF_GEOM | RUF_COLOR);
			bUpdateIndices = true;
		}

		if (bUpdateIndices)
		{
			int indexNum = 6 * (paramU.numData - 1) * (paramV.numData - 1);
			TArray< uint32 > indices;
			indices.resize(indexNum);

			uint32*  pIndexData = indices.data();
			int nu = paramU.numData;
			int nv = paramV.numData;

			uint32*  pIndex = pIndexData;
			for (int i = 0; i < nu - 1; ++i)
			{
				for (int j = 0; j < nv - 1; ++j)
				{
					int index = nv * i + j;

					pIndex[0] = index;
					pIndex[1] = index + 1;
					pIndex[2] = index + nv + 1;

					pIndex[3] = index;
					pIndex[4] = index + nv + 1;
					pIndex[5] = index + nv;

					pIndex += 6;
				}
			}
			data->indexBuffer = RHICreateIndexBuffer(indexNum, true, BCF_DefalutValue, indices.data());


			TArray<MeshUtility::SharedTriangleInfo> sharedTriangleInfos;
			TArray<uint32> triangleIds;
			MeshUtility::BuildVertexSharedTriangleInfo(indices.data(), indexNum / 3 , vertexNum, sharedTriangleInfos, triangleIds);

			int i = 1;
		}

		if (flags & RUF_GEOM)
		{
			if (context.func->getFuncType() == TYPE_SURFACE_UV)
			{
			}
			else if (context.func->getFuncType() == TYPE_SURFACE_XY)
			{
				GPU_PROFILE("Update Position");
				PROFILE_ENTRY("Update Position", "CB");
				auto myFunc = static_cast<GPUSurfaceXYFunc*>(context.func);
				auto& commandList = RHICommandList::GetImmediateList();
				RHISetShaderProgram(commandList, myFunc->mShader.getRHI());

				GenParamsData params;

				params.delata = Vector2(paramU.getIncrement(), paramV.getIncrement());
				params.gridCountU = paramU.getNumData();
				params.vertexCount = data->vertexBuffer->getNumElements();
				params.vertexSize = data->vertexBuffer->getElementSize() / sizeof(float);
				params.offset = Vector2(paramU.getRangeMin(), paramV.getRangeMin());
				params.posOffset = 0;
				params.time = mVarTime;
				mGenParamBuffer.updateBuffer(params);
				SetStructuredUniformBuffer(commandList, myFunc->mShader, mGenParamBuffer);
				myFunc->mShader.setStorageBuffer(commandList, SHADER_PARAM(VertexOutputBuffer) , *data->vertexBuffer);
				RHIDispatchCompute(commandList, Math::AlignUp( vertexNum , 8 ) , 1, 1);
			}

			if (data->getNormalOffset() != INDEX_NONE)
			{
				using namespace Render;
				PROFILE_ENTRY("Update Normal", "CB");
			}
		}

		if (flags & RUF_COLOR)
		{

		}
	}

	bool ShapeMeshBuilder::parseFunction(ShapeFuncBase& func)
	{
		if (func.getEvalType() == EEvalType::GPU)
		{
			using namespace Render;
			{
#if USE_PARALLEL_UPDATE
				Mutex::Locker locker(mParserLock);
#endif
				if (!func.parseExpression(mParser))
					return false;
			}


			if (func.getFuncType() == TYPE_SURFACE_XY)
			{
				auto& myFunc = static_cast<GPUSurfaceXYFunc&>(func);

				std::string code;
				std::vector<uint8> codeTemplate;
				if (!FFileUtility::LoadToBuffer("Shader/Game/CurveMeshGenTemplate.sgc", codeTemplate, true))
				{
					return false;
				}

				Text::Format((char const*)codeTemplate.data(), { StringView(myFunc.mExpr) }, code);

				ShaderCompileOption option;

				option.addCode((char const*)code.data());
				ShaderEntryInfo entries[] =
				{
					{ EShader::Compute , SHADER_PARAM(GenVertexCS) } ,
				};
				if (!ShaderManager::Get().loadFile(myFunc.mShader, nullptr, entries, option))
				{
					return false;
				}

			}
			return true;
		}

		{
#if USE_PARALLEL_UPDATE
			Mutex::Locker locker(mParserLock);
#endif
			if (!func.parseExpression(mParser))
				return false;
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