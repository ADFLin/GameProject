#include "ShapeMeshBuilder.h"

#include "Base.h"
#include "ShapeCommon.h"
#include "RenderData.h"
#include "ShapeFunction.h"

#include "ProfileSystem.h"
#include "SystemPlatform.h"

namespace CB
{

	static void CalcNormal(Vector3& outValue, Vector3 const& v1,Vector3 const& v2, Vector3 const& v3)
	{
		outValue = (v2 - v1).cross(v3 - v1);
		outValue.normalize();
	}

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



	void ShapeMeshBuilder::updateCurveData(ShapeUpdateInfo const& info, SampleParam const& paramS)
	{
		assert(info.func->getFunType() == TYPE_CURVE_3D);

		RenderData* data = info.data;

		unsigned flag = info.flag;

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

			Curve3DFunc* func = static_cast<Curve3DFunc*>(info.func);

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
				*pColor = info.color;
				colorData += data->getVertexSize();
			}
		}
	}

	void ShapeMeshBuilder::updateSurfaceData(ShapeUpdateInfo const& info, SampleParam const& paramU, SampleParam const& paramV)
	{
		PROFILE_ENTRY("UpdateSurfaceData");

		assert(isSurface(info.func->getFunType()));

		RenderData* data = info.data;
		unsigned flag = info.flag;
		int vertexNum = paramU.numData * paramV.numData;
		int indexNum = 6 * (paramU.numData - 1) * (paramV.numData - 1);

		if( flag & RUF_DATA_SAMPLE )
		{
			if( data->getVertexNum() != vertexNum || data->getIndexNum() != indexNum )
			{
				data->release();
				data->create(vertexNum, indexNum, true);
			}
			flag |= (RUF_GEOM | RUF_COLOR);
		}

		if( flag & RUF_GEOM )
		{

			uint8* posData = data->getVertexData() + data->getPositionOffset();

			float du = paramU.getIncrement();
			float dv = paramV.getIncrement();

			if( info.func->getFunType() == TYPE_SURFACE_UV )
			{
				SurfaceUVFunc* func = static_cast<SurfaceUVFunc*>(info.func);

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
								Vector3* pPos = (Vector3*)(posData + idx * data->getVertexSize());
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
								Vector3* pPos = (Vector3*)(posData + idx * data->getVertexSize());
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
								Vector3* pPos = (Vector3*)(posData + idx * data->getVertexSize());
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
								Vector3* pPos = (Vector3*)(posData + idx * data->getVertexSize());
								*pPos = value;
							}
						}

					}
					break;
				}
			}
			else if( info.func->getFunType() == TYPE_SURFACE_XY )
			{
				SurfaceXYFunc* func = static_cast<SurfaceXYFunc*>(info.func);

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
								Vector3* pPos = (Vector3*)(posData + idx * data->getVertexSize());
								pPos->setValue( u , v , value.z);
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
								Vector3* pPos = (Vector3*)(posData + idx * data->getVertexSize());
								pPos->setValue( u , v , value.z);
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
								Vector3* pPos = (Vector3*)(posData + idx * data->getVertexSize());
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
								Vector3* pPos = (Vector3*)(posData + idx * data->getVertexSize());
								pPos->setValue(u, v, value.z);
							}
						}

					}
					break;
				}
			}

			int*     pIndexData = data->getIndexData();
			int nu = paramU.numData;
			int nv = paramV.numData;

			int count = 0;
			for( int i = 0; i < nu - 1; ++i )
			{
				for( int j = 0; j < nv - 1; ++j )
				{
					int index = nv * i + j;

					pIndexData[count++] = index;
					pIndexData[count++] = index + 1;
					pIndexData[count++] = index + nv + 1;

					pIndexData[count++] = index;
					pIndexData[count++] = index + nv + 1;
					pIndexData[count++] = index + nv;
				}
			}

			if( data->getNormalOffset() != INDEX_NONE )
			{
#if USE_PARALLEL_UPDATE
				std::vector< Vector3 > mCacheNormal;
				std::vector< int >     mCacheCount;
#endif
				mCacheCount.resize(vertexNum, 0);
				mCacheNormal.resize(vertexNum, Vector3(0, 0, 0));

				uint8* normalData = data->getVertexData() + data->getNormalOffset();

				for( int i = 0; i < indexNum; i += 3 )
				{
					int idx0 = pIndexData[i];
					int idx1 = pIndexData[i + 1];
					int idx2 = pIndexData[i + 2];

					Vector3* pPos0 = (Vector3*)(posData + idx0 * data->getVertexSize());
					Vector3* pPos1 = (Vector3*)(posData + idx1 * data->getVertexSize());
					Vector3* pPos2 = (Vector3*)(posData + idx2 * data->getVertexSize());
					Vector3 normal;
					CalcNormal(normal, *pPos0, *pPos1, *pPos2);
					//normal = -normal;

					mCacheNormal[idx0] += normal;
					mCacheNormal[idx1] += normal;
					mCacheNormal[idx2] += normal;

					mCacheCount[idx0] += 1;
					mCacheCount[idx1] += 1;
					mCacheCount[idx2] += 1;
				}

				for( int i = 0; i < vertexNum; ++i )
				{
					Vector3* pNoraml = (Vector3*)(normalData + i * data->getVertexSize());
					*pNoraml = (1.0 / mCacheCount[i]) *mCacheNormal[i];
					(*pNoraml).normalize();
				}

			}

		}

		if( flag & RUF_COLOR )
		{
			uint8* colorData = data->getVertexData() + data->getColorOffset();
			for( int i = 0; i < vertexNum; ++i )
			{
				Color4f* pColor = (Color4f*)(colorData);
				//BYTE temp[3];
				//float z = pVertex[3*i+2].z;
				//m_ColorMap.getColor((z-datamin)/(datamax-datamin),temp);
				//setColor((z-datamin)/(datamax-datamin),&m_pColorData[4*i]);

				*pColor = info.color;
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