#include "ShapeMeshBuilder.h"

#include "Base.h"
#include "ShapeCommon.h"
#include "RenderData.h"
#include "ShapeFunction.h"

#include "ProfileSystem.h"
#include "SystemPlatform.h"
#include "Renderer/MeshUtility.h"

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
				data->create(paramS.numData, 0, false);
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
				for (int j = 0; j < paramV.numData; ++j)
				{
					float v = paramV.getRangeMin() + j * dv;
					for (int i = 0; i < paramU.numData; ++i)
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
				for (int j = 0; j < paramV.numData; ++j)
				{
					float v = paramV.getRangeMin() + j * dv;
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
		}
	}


	void ShapeMeshBuilder::updateSurfaceData(ShapeUpdateContext const& context, SampleParam const& paramU, SampleParam const& paramV)
	{
		PROFILE_ENTRY("UpdateSurfaceData");

		CHECK(isSurface(context.func->getFuncType()));

		RenderData* data = context.data;
		unsigned flags = context.flags;
		int vertexNum = paramU.numData * paramV.numData;
		int indexNum = 6 * (paramU.numData - 1) * (paramV.numData - 1);


		bool bUpdateIndices = false;

		if( flags & RUF_DATA_SAMPLE )
		{
			if( data->getVertexNum() != vertexNum || data->getIndexNum() != indexNum )
			{
				data->release();
				data->create(vertexNum, indexNum, true);
			}
			flags |= (RUF_GEOM | RUF_COLOR);
			bUpdateIndices = true;
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

		if( flags & RUF_GEOM )
		{
			if( context.func->getFuncType() == TYPE_SURFACE_UV )
			{
				SurfaceUVFunc* func = static_cast<SurfaceUVFunc*>(context.func);

				if (context.func->isNative())
				{
					NativeSurfaceUVFunc* func = static_cast<NativeSurfaceUVFunc*>(context.func);
					updatePositionData_SurfaceUV(func, paramU, paramV, *data);
				}
				else
				{
					SurfaceUVFunc* func = static_cast<SurfaceUVFunc*>(context.func);
					updatePositionData_SurfaceUV(func, paramU, paramV, *data);
				}
			}
			else if( context.func->getFuncType() == TYPE_SURFACE_XY )
			{
				PROFILE_ENTRY("Update Position", "CB");
				if (context.func->isNative())
				{
					NativeSurfaceXYFunc* func = static_cast<NativeSurfaceXYFunc*>(context.func);
					updatePositionData_SurfaceXY(func, paramU, paramV, *data);
				}
				else
				{
					SurfaceXYFunc* func = static_cast<SurfaceXYFunc*>(context.func);
					updatePositionData_SurfaceXY(func, paramU, paramV, *data);
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

	bool ShapeMeshBuilder::parseFunction(ShapeFuncBase& func)
	{
#if USE_PARALLEL_UPDATE
		Mutex::Locker locker(mParserLock);
#endif
		if( !func.parseExpression(mParser) )
			return false;

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