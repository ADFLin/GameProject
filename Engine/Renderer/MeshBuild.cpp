#include "MeshBuild.h"

#include "Renderer/Mesh.h"
#include "Renderer/MeshUtility.h"

#include "FileSystem.h"
#include "tinyobjloader/tiny_obj_loader.h"

#include <map>
#include <unordered_map>
#include "LogSystem.h"


namespace Render
{
	using ::Math::Vector2;
	using ::Math::Vector3;

	struct TileVertex
	{
		Vector3 pos;
		Vector2 uv;
	};

	bool FMeshBuild::Tile(Mesh& mesh, int tileSize, float len, bool bHaveSkirt)
	{
		MeshBuildData buildData;
		if (!Tile(buildData, mesh.mInputLayoutDesc, tileSize, len, bHaveSkirt))
			return false;

		return buildData.initializeRHI(mesh);
	}

	bool FMeshBuild::Tile(MeshBuildData& data, InputLayoutDesc& inputLayoutDesc, int tileSize, float len, bool bHaveSkirt)
	{
		int const vLen = (tileSize + 1);
		int const nV = (bHaveSkirt) ? (vLen * vLen + 4 * vLen) : (vLen * vLen);
		int const nI = (bHaveSkirt) ? (6 * tileSize * tileSize + 4 * 6 * tileSize) : (6 * tileSize * tileSize);

		float d = len / tileSize;

		inputLayoutDesc.clear();
		inputLayoutDesc.addElement(0, EVertex::ATTRIBUTE_POSITION, EVertex::Float3);
		inputLayoutDesc.addElement(0, EVertex::ATTRIBUTE_TEXCOORD, EVertex::Float2, 0);

		data.vertexData.resize(nV * sizeof(TileVertex));
		TileVertex* pV = (TileVertex*)data.vertexData.data();
		float const dtex = 1.0 / tileSize;
		for (int j = 0; j < vLen; ++j)
		{
			for (int i = 0; i < vLen; ++i)
			{
				pV->pos = Vector3(i * d, j * d, 0);
				pV->uv = Vector2(i * dtex, j * dtex);
				++pV;
			}
		}

		data.indexData.resize(nI);
		uint32* pIdx = data.indexData.data();
		for (int j = 0; j < tileSize; ++j)
		{
			for (int i = 0; i < tileSize; ++i)
			{
				int vi = i + j * vLen;
				pIdx[0] = vi;
				pIdx[1] = vi + 1;
				pIdx[2] = vi + 1 + vLen;

				pIdx[3] = vi;
				pIdx[4] = vi + 1 + vLen;
				pIdx[5] = vi + vLen;
				pIdx += 6;
			}
		}

		//fill skirt
		if (bHaveSkirt)
		{
			TileVertex* pVStart = (TileVertex*)data.vertexData.data();
			TileVertex* pV0 = pVStart + vLen * vLen + 0 * vLen;
			TileVertex* pV1 = pVStart + vLen * vLen + 1 * vLen;
			TileVertex* pV2 = pVStart + vLen * vLen + 2 * vLen;
			TileVertex* pV3 = pVStart + vLen * vLen + 3 * vLen;
			for (int i = 0; i < vLen; ++i)
			{
				pV0->pos = Vector3(i * d, 0, -1);
				pV0->uv = Vector2(i * dtex, 0);
				++pV0;

				pV1->pos = Vector3(i * d, len, -1);
				pV1->uv = Vector2(i * dtex, 1);
				++pV1;

				pV2->pos = Vector3(0, i * d, -1);
				pV2->uv = Vector2(0, i * dtex);
				++pV2;

				pV3->pos = Vector3(len, i * d, -1);
				pV3->uv = Vector2(1, i * dtex);
				++pV3;
			}

			//[(vLen-1)*vLen] [(vLen-1)*vLen + i] 
			//      y _______________________ [vLen*vLen-1]
			//        |        |             |
			//[i*vLen]|_                    _|[(i+1)*vLen-1]
			//        |                      |
			//        |________|_____________| x
			//       [0]      [i]             [vLen-1]
			uint32* p0 = pIdx;
			uint32* p1 = pIdx + 1 * (6 * tileSize);
			uint32* p2 = pIdx + 2 * (6 * tileSize);
			uint32* p3 = pIdx + 3 * (6 * tileSize);
			for (int i = 0; i < tileSize; ++i)
			{
				int vi = vLen * vLen + i;
				int vn = i;
				p0[0] = vi; p0[1] = vi + 1; p0[2] = vn + 1;
				p0[3] = vi; p0[4] = vn + 1; p0[5] = vn;
				p0 += 6;

				vi += vLen;
				vn = (vLen - 1) * vLen + i;
				p1[0] = vn; p1[1] = vn + 1; p1[2] = vi + 1;
				p1[3] = vn; p1[4] = vi + 1; p1[5] = vi;
				p1 += 6;

				vi += vLen;
				vn = i * vLen;
				p2[0] = vi; p2[1] = vn; p2[2] = vn + vLen;
				p2[3] = vi; p2[4] = vn + vLen; p2[5] = vi + 1;
				p2 += 6;

				vi += vLen;
				vn = (1 + i) * vLen - 1;
				p3[0] = vn; p3[1] = vi; p3[2] = vi + 1;
				p3[3] = vn; p3[4] = vi + 1; p3[5] = vn + vLen;
				p3 += 6;
			}
		}

		return true;
	}

	bool FMeshBuild::UVSphere(Mesh& mesh, float radius, int rings, int sectors)
	{
		MeshBuildData buildData;
		if (!UVSphere(buildData, mesh.mInputLayoutDesc, radius, rings, sectors))
			return false;

		mesh.mType = EPrimitive::TriangleList;
		return buildData.initializeRHI(mesh);
	}

	bool FMeshBuild::UVSphere(MeshBuildData& data, InputLayoutDesc& inputLayoutDesc, float radius, int rings, int sectors)
	{
		assert(rings > 1);
		assert(sectors > 0);
		assert(radius > 0);
		inputLayoutDesc.clear();
		inputLayoutDesc.addElement(0, EVertex::ATTRIBUTE_POSITION, EVertex::Float3);
		inputLayoutDesc.addElement(0, EVertex::ATTRIBUTE_NORMAL, EVertex::Float3);
		inputLayoutDesc.addElement(0, EVertex::ATTRIBUTE_TEXCOORD, EVertex::Float2);
		// #FIXME: Bug
		//inputLayoutDesc.addElement(0, EVertex::ATTRIBUTE_TANGENT, EVertex::Float4);
		int vertexSize = inputLayoutDesc.getVertexSize() / sizeof(float);

		int nV = (rings - 1) * (sectors + 1) + 2 * sectors;
		data.vertexData.resize(nV * inputLayoutDesc.getVertexSize());
		float* vertex = (float*)data.vertexData.data();
		float const rf = 1.0 / rings;
		float const sf = 1.0 / sectors;
		int r, s;

		float* v = vertex + inputLayoutDesc.getAttributeOffset(EVertex::ATTRIBUTE_POSITION) / sizeof(float);
		float* n = vertex + inputLayoutDesc.getAttributeOffset(EVertex::ATTRIBUTE_NORMAL) / sizeof(float);
		float* t = vertex + inputLayoutDesc.getAttributeOffset(EVertex::ATTRIBUTE_TEXCOORD) / sizeof(float);

		for (r = 1; r < rings; ++r)
		{
			float const z = sin(-Math::PI / 2 + Math::PI * r * rf);
			float sr = sin(Math::PI * r * rf);

			float* vb = v;
			float* nb = n;
			float* tb = t;

			for (s = 0; s < sectors; ++s)
			{
				float x, y;
				Math::SinCos(2 * Math::PI * s * sf, x, y);
				x *= sr;
				y *= sr;

				v[0] = x * radius;
				v[1] = y * radius;
				v[2] = z * radius;

				n[0] = x;
				n[1] = y;
				n[2] = z;

				v += vertexSize;
				n += vertexSize;

				t[0] = s * sf;
				t[1] = r * rf;
				t += vertexSize;
			}

			v[0] = vb[0]; v[1] = vb[1]; v[2] = vb[2];
			n[0] = nb[0]; n[1] = nb[1]; n[2] = nb[2];

			v += vertexSize;
			n += vertexSize;

			t[0] = 1; t[1] = tb[1];
			t += vertexSize;
		}

		for (int i = 0; i < sectors; ++i)
		{
			v[0] = 0; v[1] = 0; v[2] = -radius;
			n[0] = 0; n[1] = 0; n[2] = -1;

			v += vertexSize;
			n += vertexSize;

			t[0] = i * sf + 0.5 * sf; t[1] = 0;
			t += vertexSize;
		}

		for (int i = 0; i < sectors; ++i)
		{
			v[0] = 0; v[1] = 0; v[2] = radius;
			n[0] = 0; n[1] = 0; n[2] = 1;
			v += vertexSize;
			n += vertexSize;

			t[0] = i * sf + 0.5 * sf; t[1] = 1;
			t += vertexSize;
		}

		data.indexData.resize((rings - 1) * (sectors) * 6 + sectors * 2 * 3);
		uint32* iPtr = data.indexData.data();

		int idxOffset = 0;
		int idxDown = nV - 2 * sectors;
		for (s = 0; s < sectors; ++s)
		{
			iPtr[0] = idxDown + s;
			iPtr[2] = idxOffset + s + 1;
			iPtr[1] = idxOffset + s;
			iPtr += 3;
		}
		int ringOffset = sectors + 1;
		for (s = 0; s < sectors; ++s)
		{
			for (r = 0; r < rings - 2; ++r)
			{
				iPtr[0] = idxOffset + r * ringOffset + s;
				iPtr[1] = idxOffset + (r + 1) * ringOffset + (s + 1);
				iPtr[2] = idxOffset + r * ringOffset + (s + 1);

				iPtr[3] = idxOffset + iPtr[1];
				iPtr[4] = idxOffset + iPtr[0];
				iPtr[5] = idxOffset + (r + 1) * ringOffset + s;

				iPtr += 6;
			}
		}

		int idxTop = nV - sectors;
		idxOffset = idxDown - ringOffset;
		for (s = 0; s < sectors; ++s)
		{
			iPtr[0] = idxTop + s;
			iPtr[1] = idxOffset + s + 1;
			iPtr[2] = idxOffset + s;
			iPtr += 3;
		}

		//FillTangent_TriangleList(inputLayoutDesc, (float*)data.vertexData.data(), nV, data.indexData.data(), data.indexData.size());
		return true;
	}

	bool FMeshBuild::SkyBox(Mesh& mesh)
	{
		MeshBuildData buildData;
		if (!SkyBox(buildData, mesh.mInputLayoutDesc))
			return false;

#if 0
		mesh.mType = EPrimitive::Quad;
#else
		mesh.mType = EPrimitive::TriangleList;
#endif
		return buildData.initializeRHI(mesh);
	}

	bool FMeshBuild::SkyBox(MeshBuildData& data, InputLayoutDesc& inputLayoutDesc)
	{
		inputLayoutDesc.clear();
		inputLayoutDesc.addElement(0, EVertex::ATTRIBUTE_POSITION, EVertex::Float3);
		inputLayoutDesc.addElement(0, EVertex::ATTRIBUTE_TEXCOORD, EVertex::Float3);

		Vector3 vertices[] =
		{
			Vector3(1,1,1),Vector3(1,1,1),
			Vector3(-1,1,1),Vector3(-1,1,1),
			Vector3(1,-1,1),Vector3(1,-1,1),
			Vector3(-1,-1,1),Vector3(-1,-1,1),
			Vector3(1,1,-1),Vector3(1,1,-1),
			Vector3(-1,1,-1),Vector3(-1,1,-1),
			Vector3(1,-1,-1),Vector3(1,-1,-1),
			Vector3(-1,-1,-1),Vector3(-1,-1,-1),
		};

		uint32 indices[] =
		{
			0 , 4 , 6 , 2 ,
			1 , 3 , 7 , 5 ,
			0 , 1 , 5 , 4 ,
			2 , 6 , 7 , 3 ,
			0 , 2 , 3 , 1 ,
			4 , 5 , 7 , 6 ,
		};

#if 0
		data.vertexData.assign((uint8*)vertices, (uint8*)(vertices + ARRAY_SIZE(vertices)));
		data.indexData.assign(indices, indices + ARRAY_SIZE(indices));
#else
		TArray< uint32 > tempIndices;
		int numTriangles;
		MeshUtility::ConvertToTriangleListIndices(EPrimitive::Quad, indices, 4 * 6, tempIndices, numTriangles);

		data.vertexData.assign((uint8*)vertices, (uint8*)(vertices + ARRAY_SIZE(vertices)));
		data.indexData.assign(tempIndices.begin(), tempIndices.end());
#endif
		return true;
	}
	bool FMeshBuild::CubeShare(Mesh& mesh, float halfLen)
	{
		MeshBuildData buildData;
		if (!CubeShare(buildData, mesh.mInputLayoutDesc, halfLen))
			return false;

#if 1
		mesh.mType = EPrimitive::TriangleList;
#else
		mesh.mType = EPrimitive::Quad;
#endif
		return buildData.initializeRHI(mesh);
	}

	bool FMeshBuild::CubeShare(MeshBuildData& data, InputLayoutDesc& inputLayoutDesc, float halfLen)
	{
		inputLayoutDesc.clear();
		inputLayoutDesc.addElement(0, EVertex::ATTRIBUTE_POSITION, EVertex::Float3);
		inputLayoutDesc.addElement(0, EVertex::ATTRIBUTE_NORMAL, EVertex::Float3);
		inputLayoutDesc.addElement(0, EVertex::ATTRIBUTE_TEXCOORD, EVertex::Float2);
		inputLayoutDesc.addElement(0, EVertex::ATTRIBUTE_TANGENT, EVertex::Float4);
		struct MyVertex
		{
			Vector3 pos;
			Vector3 normal;
			Vector2 uv;
			Vector4 tangent;
		};
		MyVertex v[] =
		{
			//x
			{ halfLen * Vector3(1,1,1),Vector3(1,0,0),{ 0,0 } },
			{ halfLen * Vector3(1,-1,1),Vector3(1,0,0),{ 0,1 } },
			{ halfLen * Vector3(1,-1,-1),Vector3(1,0,0),{ 1,1 } },
			{ halfLen * Vector3(1,1,-1),Vector3(1,0,0),{ 1,0 } },
			//-x
			{ halfLen * Vector3(-1,1,1),Vector3(-1,0,0),{ 0,0 } },
			{ halfLen * Vector3(-1,1,-1),Vector3(-1,0,0),{ 1,0 } },
			{ halfLen * Vector3(-1,-1,-1),Vector3(-1,0,0),{ 1,1 } },
			{ halfLen * Vector3(-1,-1,1),Vector3(-1,0,0),{ 0,1 } },
		};

#if 1
		uint32 indices[] =
		{
			0 , 1 , 2 , 0 , 2 , 3 , //x
			4 , 5 , 6 , 4 , 6 , 7 , //-x

			0 , 3 , 5, 0 , 5 , 4 , //y
			1 , 7 , 6, 1 , 6 , 2 , //-y

			0 , 4 , 7 ,0 , 7 , 1 , // z
			3 , 2 , 6 ,3 , 6 , 5 , //-z
		};

		MeshUtility::FillNormalTangent_TriangleList(inputLayoutDesc, &v[0], ARRAY_SIZE(v), &indices[0], ARRAY_SIZE(indices));
		data.vertexData.assign((uint8*)v, (uint8*)(v + ARRAY_SIZE(v)));
		data.indexData.assign(indices, indices + ARRAY_SIZE(indices));
#else
		uint32 indices[] =
		{
			0 , 1 , 2 , 3 , //x
			4 , 7 , 6 , 5 , //-x

			0 , 4 , 5 , 1, //y
			2 , 6 , 7 , 3, //-y

			0 , 3 , 7 , 4 , // z
			1 , 5 , 6 , 2 , //-z
		};

		MeshUtility::FillNormalTangent_QuadList(inputLayoutDesc, &v[0], ARRAY_SIZE(v), &indices[0], ARRAY_SIZE(indices));
		data.vertexData.assign((uint8*)v, (uint8*)(v + ARRAY_SIZE(v)));
		data.indexData.assign(indices, indices + ARRAY_SIZE(indices));
#endif
		return true;
	}

	bool FMeshBuild::CubeOffset(Mesh& mesh, float halfLen, Vector3 const& offset)
	{
		MeshBuildData buildData;
		if (!CubeOffset(buildData, mesh.mInputLayoutDesc, halfLen, offset))
			return false;

#if 1
		mesh.mType = EPrimitive::TriangleList;
#else
		mesh.mType = EPrimitive::Quad;
#endif
		return buildData.initializeRHI(mesh);
	}

	bool FMeshBuild::CubeOffset(MeshBuildData& data, InputLayoutDesc& inputLayoutDesc, float halfLen, Vector3 const& offset)
	{
		inputLayoutDesc.clear();
		inputLayoutDesc.addElement(0, EVertex::ATTRIBUTE_POSITION, EVertex::Float3);
		inputLayoutDesc.addElement(0, EVertex::ATTRIBUTE_NORMAL, EVertex::Float3);
		inputLayoutDesc.addElement(0, EVertex::ATTRIBUTE_TEXCOORD, EVertex::Float2);
		inputLayoutDesc.addElement(0, EVertex::ATTRIBUTE_TANGENT, EVertex::Float4);
		struct MyVertex
		{
			Vector3 pos;
			Vector3 normal;
			Vector2 uv;
			Vector4 tangent;
		};
		MyVertex vertices[] =
		{
			//x
			{ halfLen * Vector3(1,1,1),Vector3(1,0,0),{0,0} },
			{ halfLen * Vector3(1,-1,1),Vector3(1,0,0),{0,1} },
			{ halfLen * Vector3(1,-1,-1),Vector3(1,0,0),{1,1} },
			{ halfLen * Vector3(1,1,-1),Vector3(1,0,0),{1,0} },
			//-x
			{ halfLen * Vector3(-1,1,1),Vector3(-1,0,0),{0,0} },
			{ halfLen * Vector3(-1,1,-1),Vector3(-1,0,0),{1,0} },
			{ halfLen * Vector3(-1,-1,-1),Vector3(-1,0,0),{1,1} },
			{ halfLen * Vector3(-1,-1,1),Vector3(-1,0,0),{0,1} },
			//y
			{ halfLen * Vector3(1,1,1),Vector3(0,1,0),{1,1} },
			{ halfLen * Vector3(1,1,-1),Vector3(0,1,0),{1,0} },
			{ halfLen * Vector3(-1,1,-1),Vector3(0,1,0),{0,0} },
			{ halfLen * Vector3(-1,1,1),Vector3(0,1,0),{0,1} },
			//-y
			{ halfLen * Vector3(1,-1,1),Vector3(0,-1,0),{1,1} },
			{ halfLen * Vector3(-1,-1,1),Vector3(0,-1,0),{0,1} },
			{ halfLen * Vector3(-1,-1,-1),Vector3(0,-1,0),{0,0} },
			{ halfLen * Vector3(1,-1,-1),Vector3(0,-1,0),{1,0} },
			//z
			{ halfLen * Vector3(1,1,1),Vector3(0,0,1),{1,1} },
			{ halfLen * Vector3(-1,1,1),Vector3(0,0,1),{0,1} },
			{ halfLen * Vector3(-1,-1,1),Vector3(0,0,1),{0,0} },
			{ halfLen * Vector3(1,-1,1),Vector3(0,0,1),{1,0} },
			//-z
			{ halfLen * Vector3(1,1,-1),Vector3(0,0,-1),{1,1} },
			{ halfLen * Vector3(1,-1,-1),Vector3(0,0,-1),{1,0} },
			{ halfLen * Vector3(-1,-1,-1),Vector3(0,0,-1),{0,0} },
			{ halfLen * Vector3(-1,1,-1),Vector3(0,0,-1),{0,1} },
		};
		for (auto& v : vertices)
		{
			v.pos += offset;
		}

#if 1
		uint32 indices[] =
		{
			0 , 1 , 2 , 0 , 2 , 3 ,
			4 , 5 , 6 , 4 , 6 , 7 ,
			8 , 9 , 10 , 8 , 10 , 11 ,
			12 , 13 , 14 , 12 , 14 , 15 ,
			16 , 17 , 18 , 16 , 18 , 19 ,
			20 , 21 , 22 , 20 , 22 , 23 ,
		};

		MeshUtility::FillTangent_TriangleList(inputLayoutDesc, &vertices[0], 6 * 4, &indices[0], 6 * 6);
		data.vertexData.assign((uint8*)vertices, (uint8*)(vertices + ARRAY_SIZE(vertices)));
		data.indexData.assign(indices, indices + ARRAY_SIZE(indices));
#else
		uint32 indices[] =
		{
			0 , 1 , 2 , 3 ,
			4 , 5 , 6 , 7 ,
			8 , 9 , 10 , 11 ,
			12 , 13 , 14 , 15 ,
			16 , 17 , 18 , 19 ,
			20 , 21 , 22 , 23 ,
		};

		MeshUtility::FillTangent_QuadList(inputLayoutDesc, &vertices[0], 6 * 4, &indices[0], 6 * 4);
		data.vertexData.assign((uint8*)vertices, (uint8*)(vertices + ARRAY_SIZE(vertices)));
		data.indexData.assign(indices, indices + ARRAY_SIZE(indices));
#endif

		return true;
	}

	bool FMeshBuild::CubeLineOffset(Mesh& mesh, float halfLen, Vector3 const& offset)
	{
		MeshBuildData buildData;
		if (!CubeLineOffset(buildData, mesh.mInputLayoutDesc, halfLen, offset))
			return false;

		mesh.mType = EPrimitive::LineList;
		return buildData.initializeRHI(mesh);
	}

	bool FMeshBuild::CubeLineOffset(MeshBuildData& data, InputLayoutDesc& inputLayoutDesc, float halfLen, Vector3 const& offset)
	{
		inputLayoutDesc.clear();
		inputLayoutDesc.addElement(0, EVertex::ATTRIBUTE_POSITION, EVertex::Float3);
		struct MyVertex
		{
			Vector3 pos;
		};
		MyVertex vertices[] =
		{
			//x
			{ halfLen * Vector3(1,1,1)},
			{ halfLen * Vector3(1,-1,1)},
			{ halfLen * Vector3(1,-1,-1)},
			{ halfLen * Vector3(1,1,-1)},
			//-x
			{ halfLen * Vector3(-1,1,1)},
			{ halfLen * Vector3(-1,1,-1)},
			{ halfLen * Vector3(-1,-1,-1)},
			{ halfLen * Vector3(-1,-1,1)},
		};
		for (auto& v : vertices)
		{
			v.pos += offset;
		}

		uint32 indices[] =
		{
			0,1, 1,2, 2,3, 3,0,
			4,5, 5,6, 6,7, 7,4,
			0,4, 1,5, 2,6, 3,7,
		};

		data.vertexData.assign((uint8*)vertices, (uint8*)(vertices + ARRAY_SIZE(vertices)));
		data.indexData.assign(indices, indices + ARRAY_SIZE(indices));

		return true;
	}

	bool FMeshBuild::Cube(Mesh& mesh, float halfLen)
	{
		return CubeOffset(mesh, halfLen, Vector3::Zero());
	}

	bool FMeshBuild::Cube(MeshBuildData& data, InputLayoutDesc& inputLayoutDesc, float halfLen)
	{
		return CubeOffset(data, inputLayoutDesc, halfLen, Vector3::Zero());
	}

	bool FMeshBuild::Doughnut(Mesh& mesh, float radius, float ringRadius, int rings, int sectors)
	{
		MeshBuildData buildData;
		if (!Doughnut(buildData, mesh.mInputLayoutDesc, radius, ringRadius, rings, sectors))
			return false;

		mesh.mType = EPrimitive::TriangleList;
		return buildData.initializeRHI(mesh);
	}

	bool FMeshBuild::Doughnut(MeshBuildData& data, InputLayoutDesc& inputLayoutDesc, float radius, float ringRadius, int rings, int sectors)
	{
		inputLayoutDesc.clear();
		inputLayoutDesc.addElement(0, EVertex::ATTRIBUTE_POSITION, EVertex::Float3);
		inputLayoutDesc.addElement(0, EVertex::ATTRIBUTE_NORMAL, EVertex::Float3);
		inputLayoutDesc.addElement(0, EVertex::ATTRIBUTE_TANGENT, EVertex::Float4);

		int vertexSize = inputLayoutDesc.getVertexSize() / sizeof(float);
		int nV = rings * sectors;
		data.vertexData.resize(nV * inputLayoutDesc.getVertexSize());

		float* pVertexData = (float*)data.vertexData.data();
		auto GetVertexPtr = [&](int s, int r) { return pVertexData + (s * rings + r) * vertexSize; };

		float sf = 2 * Math::PI / sectors;
		float rf = 2 * Math::PI / rings;
		for (int s = 0; s < sectors; ++s)
		{
			float sx, sy;
			Math::SinCos(s * sf, sx, sy);
			for (int r = 0; r < rings; ++r)
			{
				float rs, rc;
				Math::SinCos(r * rf, rs, rc);

				float* v = GetVertexPtr(s, r);
				// Position
				v[0] = (radius + ringRadius * rc) * sx;
				v[1] = (radius + ringRadius * rc) * sy;
				v[2] = ringRadius * rs;

				// Normal (along vector from ring segment center to vertex)
				v[3] = rc * sx;
				v[4] = rc * sy;
				v[5] = rs;

				// Tangent (along major circle)
				v[6] = -sy;
				v[7] = sx;
				v[8] = 0;
				v[9] = 1.0f;
			}
		}

		data.indexData.resize(sectors * rings * 6);
		uint32* pIdx = data.indexData.data();
		for (int s = 0; s < sectors; ++s)
		{
			int s_next = (s + 1) % sectors;
			for (int r = 0; r < rings; ++r)
			{
				int r_next = (r + 1) % rings;

				uint32 i00 = s * rings + r;
				uint32 idx10 = s_next * rings + r;
				uint32 idx01 = s * rings + r_next;
				uint32 idx11 = s_next * rings + r_next;

				pIdx[0] = i00;
				pIdx[1] = idx10;
				pIdx[2] = idx11;

				pIdx[3] = i00;
				pIdx[4] = idx11;
				pIdx[5] = idx01;
				pIdx += 6;
			}
		}

		return true;
	}


	bool FMeshBuild::Plane(Mesh& mesh, Vector3 const& offset, Vector3 const& normal, Vector3 const& dirY, Vector2 const& size, float texFactor)
	{
		MeshBuildData buildData;
		if (!Plane(buildData, mesh.mInputLayoutDesc, offset, normal, dirY, size, texFactor))
			return false;

		mesh.mType = EPrimitive::TriangleList;
		return buildData.initializeRHI(mesh);
	}

	bool FMeshBuild::Plane(MeshBuildData& data, InputLayoutDesc& inputLayoutDesc, Vector3 const& offset, Vector3 const& normal, Vector3 const& dirY, Vector2 const& size, float texFactor)
	{
		inputLayoutDesc.clear();
		inputLayoutDesc.addElement(0, EVertex::ATTRIBUTE_POSITION, EVertex::Float3);
		inputLayoutDesc.addElement(0, EVertex::ATTRIBUTE_NORMAL, EVertex::Float3);
		inputLayoutDesc.addElement(0, EVertex::ATTRIBUTE_TEXCOORD, EVertex::Float2);
		inputLayoutDesc.addElement(0, EVertex::ATTRIBUTE_TANGENT, EVertex::Float4);

		Vector3 axisZ = Math::GetNormal(normal);
		Vector3 axisY = dirY - axisZ * (axisZ.dot(dirY));
		axisY.normalize();
		Vector3 axisX = axisY.cross(axisZ);
		axisX.normalize();
		struct MyVertex
		{
			Vector3 v;
			Vector3 n;
			float st[2];
			float tangent[4];
		};

		Vector3 v1 = size.x * axisX + size.y * axisY;
		Vector3 v2 = -size.x * axisX + size.y * axisY;
		MyVertex v[] =
		{
			{ offset + v1 , axisZ , { texFactor , texFactor } , } ,
			{ offset + v2 , axisZ , { 0 , texFactor } ,  } ,
			{ offset - v1 , axisZ , { 0 , 0 } , } ,
			{ offset - v2 , axisZ , { texFactor , 0 } , } ,
		};

		uint32 idx[6] = { 0 , 1 , 2 , 0 , 2 , 3 };
		MeshUtility::FillTangent_TriangleList(inputLayoutDesc, &v[0], 4, &idx[0], 6);

		data.vertexData.assign((uint8*)v, (uint8*)(v + 4));
		data.indexData.assign(idx, idx + 6);

		return true;
	}


	bool FMeshBuild::Disc(Mesh& mesh, float radius, int sectors)
	{
		MeshBuildData buildData;
		if (!Disc(buildData, mesh.mInputLayoutDesc, radius, sectors))
			return false;

		mesh.mType = EPrimitive::TriangleList;
		return buildData.initializeRHI(mesh);
	}

	bool FMeshBuild::Disc(MeshBuildData& data, InputLayoutDesc& inputLayoutDesc, float radius, int sectors)
	{
		inputLayoutDesc.clear();
		inputLayoutDesc.addElement(0, EVertex::ATTRIBUTE_POSITION, EVertex::Float3);
		inputLayoutDesc.addElement(0, EVertex::ATTRIBUTE_NORMAL, EVertex::Float3);
		inputLayoutDesc.addElement(0, EVertex::ATTRIBUTE_TEXCOORD, EVertex::Float2);
		inputLayoutDesc.addElement(0, EVertex::ATTRIBUTE_TANGENT, EVertex::Float4);

		int nV = sectors + 1;
		struct MyVertex
		{
			Vector3 pos;
			Vector3 normal;
			Vector2 uv;
			Vector4 tangent;
		};
		data.vertexData.resize(nV * sizeof(MyVertex));
		MyVertex* v = (MyVertex*)data.vertexData.data();

		// Center
		v[0].pos = Vector3::Zero();
		v[0].normal = Vector3(0, 0, 1);
		v[0].uv = Vector2(0.5f, 0.5f);
		v[0].tangent = Vector4(1, 0, 0, 1);

		for (int i = 0; i < sectors; ++i)
		{
			float angle = 2 * Math::PI * i / sectors;
			float s, c;
			Math::SinCos(angle, s, c);
			v[i + 1].pos = Vector3(radius * c, radius * s, 0);
			v[i + 1].normal = Vector3(0, 0, 1);
			v[i + 1].uv = Vector2(0.5f + 0.5f * c, 0.5f + 0.5f * s);
			v[i + 1].tangent = Vector4(1, 0, 0, 1);
		}

		data.indexData.resize(3 * sectors);
		uint32* idx = data.indexData.data();
		for (int i = 0; i < sectors; ++i)
		{
			idx[3 * i] = 0;
			idx[3 * i + 1] = i + 1;
			idx[3 * i + 2] = (i + 1 == sectors) ? 1 : i + 2;
		}

		return true;
	}


	bool FMeshBuild::SimpleSkin(Mesh& mesh, float width, float height, int nx, int ny)
	{
		MeshBuildData buildData;
		if (!SimpleSkin(buildData, mesh.mInputLayoutDesc, width, height, nx, ny))
			return false;

		mesh.mType = EPrimitive::TriangleList;
		return buildData.initializeRHI(mesh);
	}

	bool FMeshBuild::SimpleSkin(MeshBuildData& data, InputLayoutDesc& inputLayoutDesc, float width, float height, int nx, int ny)
	{
		struct FVertex
		{
			Vector3 pos;
			Vector3 normal;
			Vector4 tangent;
			float   uv[2];
			uint8   boneIndices[4];
			Vector4 blendWeight;
		};
		inputLayoutDesc.clear();
		inputLayoutDesc.addElement(0, EVertex::ATTRIBUTE_POSITION, EVertex::Float3);
		inputLayoutDesc.addElement(0, EVertex::ATTRIBUTE_NORMAL, EVertex::Float3);
		inputLayoutDesc.addElement(0, EVertex::ATTRIBUTE_TANGENT, EVertex::Float4);
		inputLayoutDesc.addElement(0, EVertex::ATTRIBUTE_TEXCOORD, EVertex::Float2);
		inputLayoutDesc.addElement(0, EVertex::ATTRIBUTE_BONEINDEX, EVertex::UByte4);
		inputLayoutDesc.addElement(0, EVertex::ATTRIBUTE_BLENDWEIGHT, EVertex::Float4);

		float du = 1.0 / (nx - 1);
		float dv = 1.0 / (ny - 1);
		float dw = width / (nx - 1);
		float dh = height / (ny - 1);

		data.vertexData.resize(nx * ny * sizeof(FVertex));
		FVertex* pDataV = (FVertex*)data.vertexData.data();
		for (int i = 0; i < nx; ++i)
		{
			for (int j = 0; j < ny; ++j)
			{
				FVertex& v = *pDataV++;
				v.pos = Vector3(i * dw - 0.5 * width, j * dh - 0.5 * height, 0);
				v.normal = Vector3(0, 0, 1);
				v.tangent = Vector4(1, 0, 0, 1);
				v.uv[0] = i * du;
				v.uv[1] = j * dv;
				v.boneIndices[0] = 0;
				v.boneIndices[1] = 1;
				v.boneIndices[2] = 0;
				v.boneIndices[3] = 0;
				v.blendWeight = Vector4(1 - du, du, 0, 0);

			}
		}

		data.indexData.resize(6 * (nx - 1) * (ny - 1));
		uint32* pIdx = data.indexData.data();

		for (int i = 0; i < nx - 1; ++i)
		{
			for (int j = 0; j < ny - 1; ++j)
			{
				int i00 = j * nx + i;
				int i10 = i00 + 1;
				int i01 = (j + 1) * nx + i;
				int i11 = i01 + 1;
				pIdx[0] = i00; pIdx[1] = i10; pIdx[2] = i11;
				pIdx[3] = i00; pIdx[4] = i11; pIdx[5] = i01;
				pIdx += 6;
			}
		}

		return true;
	}

	bool FMeshBuild::ObjFileMesh(Mesh& mesh, MeshData& data)
	{
		int maxNumVertex = data.numPosition * data.numNormal;
		TArray< int > idxMap;
		idxMap.resize(maxNumVertex, -1);
		TArray< float > vertices;
		TArray< uint32 > indices(data.numIndex);
		vertices.reserve(maxNumVertex);

		int numVertices = 0;
		for (int i = 0; i < data.numIndex; ++i)
		{
			int idxPos = data.indices[2 * i] - 1;
			int idxNormal = data.indices[2 * i + 1] - 1;
			int idxToVertex = idxPos + idxNormal * data.numPosition;
			int idx = idxMap[idxToVertex];
			if (idx == -1)
			{
				idx = numVertices;
				++numVertices;
				idxMap[idxToVertex] = idx;

				float* p = data.position + 3 * idxPos;
				vertices.push_back(p[0]);
				vertices.push_back(p[1]);
				vertices.push_back(p[2]);
				float* n = data.normal + 3 * idxNormal;
				vertices.push_back(n[0]);
				vertices.push_back(n[1]);
				vertices.push_back(n[2]);
			}
			indices[i] = idx;
		}
		mesh.mInputLayoutDesc.clear();
		mesh.mInputLayoutDesc.addElement(0, EVertex::ATTRIBUTE_POSITION, EVertex::Float3);
		mesh.mInputLayoutDesc.addElement(0, EVertex::ATTRIBUTE_NORMAL, EVertex::Float3);
		if (!mesh.createRHIResource(vertices.data(), numVertices, indices.data(), data.numIndex))
			return false;
		return true;
	}


	class MaterialStringStreamReader : public tinyobj::MaterialReader
	{
	public:
		std::string operator() (
			const std::string& matId,
			std::vector< tinyobj::material_t >& materials,
			std::map<std::string, int>& matMap) override
		{
			return "";
		}
	};

	bool FMeshBuild::LoadObjectFile(Mesh& mesh, char const* path, Matrix4* pTransform, OBJMaterialBuildListener* listener, int* skip)
	{
		//std::ifstream objStream(path);
		//if( !objStream.is_open() )
			//return false;

		MaterialStringStreamReader matSSReader;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;

		char const* dirPos = FFileUtility::GetFileName(path);

		std::string dir = std::string(path, dirPos - path);

		std::string err = tinyobj::LoadObj(shapes, materials, path, dir.c_str());

		if (shapes.empty())
			return true;

		int numVertices = 0;
		int numIndices = 0;

		int shapeIndex = 0;

		if (shapes.size() > 1)
		{
			shapeIndex = 0;
		}
		int nSkip = 0;
		for (int i = 0; i < shapes.size(); ++i)
		{
			if (skip && skip[nSkip] == i)
			{
				++nSkip;
				continue;
			}
			tinyobj::mesh_t& objMesh = shapes[i].mesh;
			numVertices += objMesh.positions.size() / 3;
			numIndices += objMesh.indices.size();
		}

		if (numVertices == 0)
			return false;

		mesh.mInputLayoutDesc.clear();
		mesh.mInputLayoutDesc.addElement(0, EVertex::ATTRIBUTE_POSITION, EVertex::Float3);
		mesh.mInputLayoutDesc.addElement(0, EVertex::ATTRIBUTE_NORMAL, EVertex::Float3);

		if (!shapes[0].mesh.texcoords.empty())
		{
			mesh.mInputLayoutDesc.addElement(0, EVertex::ATTRIBUTE_TEXCOORD, EVertex::Float2);
			mesh.mInputLayoutDesc.addElement(0, EVertex::ATTRIBUTE_TANGENT, EVertex::Float4);
		}

		int vertexSize = mesh.mInputLayoutDesc.getVertexSize() / sizeof(float);
		TArray< float > vertices(numVertices * vertexSize);
		TArray< uint32 > indices;
		indices.reserve(numIndices);


#define USE_SAME_MATERIAL_MERGE 1

#if USE_SAME_MATERIAL_MERGE
		struct ObjMeshSection
		{
			int matId;
			TArray< int > indices;
		};
		std::map< int, ObjMeshSection > meshSectionMap;
#endif
		int vOffset = 0;
		nSkip = 0;
		for (int idx = 0; idx < shapes.size(); ++idx)
		{
			if (skip && skip[nSkip] == idx)
			{
				++nSkip;
				continue;
			}

			tinyobj::mesh_t& objMesh = shapes[idx].mesh;

			int nvCur = vOffset / vertexSize;

#if USE_SAME_MATERIAL_MERGE
			ObjMeshSection& meshSection = meshSectionMap[objMesh.material_ids[0]];
			meshSection.matId = objMesh.material_ids[0];
			int startIndex = meshSection.indices.size();
			meshSection.indices.insert(meshSection.indices.end(), objMesh.indices.begin(), objMesh.indices.end());
			if (idx != 0)
			{
				int nvCur = vOffset / vertexSize;
				for (int i = startIndex; i < meshSection.indices.size(); ++i)
				{
					meshSection.indices[i] += nvCur;
				}
			}

#else

			MeshSection section;
			section.count = objMesh.indices.size();
			section.indexStart = startIndex;
			mesh.mSections.push_back(section);

			int startIndex = indices.size();
			indices.insert(indices.end(), objMesh.indices.begin(), objMesh.indices.end());
			if (idx != 0)
			{
				int nvCur = vOffset / vertexSize;
				for (int i = startIndex; i < indices.size(); ++i)
				{
					indices[i] += nvCur;
				}
			}
#endif //USE_SAME_MATERIAL_MERGE

			int objNV = objMesh.positions.size() / 3;
			float* tex = nullptr;
			if (!objMesh.texcoords.empty())
			{
				tex = &objMesh.texcoords[0];
			}
			if (tex)
			{
				float* v = &vertices[vOffset] + 6;
				for (int i = 0; i < objNV; ++i)
				{
					v[0] = tex[0]; v[1] = tex[1];
					//v[3] = n[0]; v[4] = n[1]; v[5] = n[2];
					tex += 2;
					v += vertexSize;
				}
			}

			float* p = &objMesh.positions[0];
			float* v = &vertices[vOffset];
			if (objMesh.normals.empty())
			{
				for (int i = 0; i < objNV; ++i)
				{
					v[0] = p[0]; v[1] = p[1]; v[2] = p[2];
					//v[3] = n[0]; v[4] = n[1]; v[5] = n[2];
					p += 3;
					v += vertexSize;
				}

			}
			else
			{
				float* n = &objMesh.normals[0];
				for (int i = 0; i < objNV; ++i)
				{
					v[0] = p[0]; v[1] = p[1]; v[2] = p[2];
					v[3] = n[0]; v[4] = n[1]; v[5] = n[2];
					p += 3;
					n += 3;
					v += vertexSize;
				}

			}
			vOffset += objNV * vertexSize;
		}

#if USE_SAME_MATERIAL_MERGE
		TArray< ObjMeshSection* > sortedMeshSctions;
		for (auto& pair : meshSectionMap)
		{
			sortedMeshSctions.push_back(&pair.second);
		}
		std::sort(sortedMeshSctions.begin(), sortedMeshSctions.end(),
			[](ObjMeshSection const* m1, ObjMeshSection const* m2) -> bool
		{
			return m1->matId < m2->matId;
		});
		for (auto meshSection : sortedMeshSctions)
		{
			MeshSection section;
			section.count = meshSection->indices.size();
			section.indexStart = indices.size();
			mesh.mSections.push_back(section);
			indices.insert(indices.end(), meshSection->indices.begin(), meshSection->indices.end());
		}
#endif //  USE_SAME_MATERIAL_MERGE

		if (shapes[0].mesh.normals.empty())
		{
			if (pTransform)
			{
				float*v = &vertices[0];
				for (int i = 0; i < numVertices; ++i)
				{
					Vector3& pos = *reinterpret_cast<Vector3*>(v);
					pos = Math::TransformPosition(pos, *pTransform);
					v += vertexSize;
				}
			}
			if (!shapes[0].mesh.texcoords.empty())
				MeshUtility::FillNormalTangent_TriangleList(mesh.mInputLayoutDesc, &vertices[0], numVertices, &indices[0], indices.size());
			else
				MeshUtility::FillNormal_TriangleList(mesh.mInputLayoutDesc, &vertices[0], numVertices, &indices[0], indices.size());
		}
		else
		{
			if (pTransform)
			{
				float* v = &vertices[0];
				for (int i = 0; i < numVertices; ++i)
				{
					Vector3& pos = *reinterpret_cast<Vector3*>(v);
					Vector3& noraml = *reinterpret_cast<Vector3*>(v + 3);
					pos = Math::TransformPosition(pos, *pTransform);
					//#TODO: non-uniform scale
					noraml = Math::TransformVector(noraml, *pTransform);
					noraml.normalize();
					v += vertexSize;
				}
			}
			else
			{
				float* v = &vertices[0];
				for (int i = 0; i < numVertices; ++i)
				{
					Vector3& noraml = *reinterpret_cast<Vector3*>(v + 3);
					noraml.normalize();
					v += vertexSize;
				}
			}
			if (!shapes[0].mesh.texcoords.empty())
			{
				MeshUtility::FillTangent_TriangleList(mesh.mInputLayoutDesc, &vertices[0], numVertices, &indices[0], indices.size());
			}
		}
		if (!mesh.createRHIResource(&vertices[0], numVertices, indices.data(), indices.size()))
			return false;

		if (listener)
		{
			for (int i = 0; i < sortedMeshSctions.size(); ++i)
			{
				if (sortedMeshSctions[i]->matId != -1)
				{
					tinyobj::material_t  const& mat = materials[sortedMeshSctions[i]->matId];

					OBJMaterialInfo info;
					info.name = mat.name.c_str();
					info.ambient = Vector3(mat.ambient);
					info.diffuse = Vector3(mat.diffuse);
					info.specular = Vector3(mat.specular);
					info.transmittance = Vector3(mat.transmittance);
					info.emission = Vector3(mat.emission);
					info.ambientTextureName = mat.ambient_texname.empty() ? nullptr : mat.ambient_texname.c_str();
					info.diffuseTextureName = mat.diffuse_texname.empty() ? nullptr : mat.diffuse_texname.c_str();
					info.specularTextureName = mat.specular_texname.empty() ? nullptr : mat.specular_texname.c_str();
					info.normalTextureName = mat.normal_texname.empty() ? nullptr : mat.normal_texname.c_str();
					for (auto& pair : mat.unknown_parameter)
					{
						std::pair< char const*, char const* > parmPair;
						parmPair.first = pair.first.c_str();
						parmPair.second = pair.second.c_str();
						info.unknownParameters.push_back(parmPair);
					}
					listener->build(i, &info);
				}
				else
				{
					listener->build(i, nullptr);
				}

			}
		}

		return true;
	}

	bool FMeshBuild::PlaneZ(Mesh& mesh, float halfLen, float texFactor)
	{
		MeshBuildData buildData;
		if (!PlaneZ(buildData, mesh.mInputLayoutDesc, halfLen, texFactor))
			return false;

		mesh.mType = EPrimitive::TriangleList;
		return buildData.initializeRHI(mesh);
	}

	bool FMeshBuild::PlaneZ(MeshBuildData& data, InputLayoutDesc& inputLayoutDesc, float halfLen, float texFactor)
	{
		inputLayoutDesc.clear();
		inputLayoutDesc.addElement(0, EVertex::ATTRIBUTE_POSITION, EVertex::Float3);
		inputLayoutDesc.addElement(0, EVertex::ATTRIBUTE_NORMAL, EVertex::Float3);
		inputLayoutDesc.addElement(0, EVertex::ATTRIBUTE_TEXCOORD, EVertex::Float2);
		inputLayoutDesc.addElement(0, EVertex::ATTRIBUTE_TANGENT, EVertex::Float4);
		struct MyVertex
		{
			Vector3 v;
			Vector3 n;
			float st[2];
			float tangent[4];
		};
		MyVertex v[] =
		{
			{ halfLen * Vector3(1,1,0) , Vector3(0,0,1) , { texFactor , texFactor } , } ,
			{ halfLen * Vector3(-1,1,0) , Vector3(0,0,1) , { 0 , texFactor } ,  } ,
			{ halfLen * Vector3(-1,-1,0) , Vector3(0,0,1) , { 0 , 0 } , } ,
			{ halfLen * Vector3(1,-1,0) , Vector3(0,0,1) , { texFactor , 0 } , } ,
		};

		uint32 indices[6] = { 0 , 1 , 2 , 0 , 2 , 3 };

		MeshUtility::FillTangent_TriangleList(inputLayoutDesc, &v[0], 4, &indices[0], 6);

		data.vertexData.assign((uint8*)v, (uint8*)(v + 4));
		data.indexData.assign(indices, indices + 6);

		return true;
	}

	bool FMeshBuild::Cone(Mesh& mesh, float height, float radius, int numSide)
	{
		MeshBuildData buildData;
		if (!Cone(buildData, mesh.mInputLayoutDesc, height, radius, numSide))
			return false;

		mesh.mType = EPrimitive::TriangleList;
		return buildData.initializeRHI(mesh);
	}

	bool FMeshBuild::Cone(MeshBuildData& data, InputLayoutDesc& inputLayoutDesc, float height, float radius, int numSide)
	{
		// Tip at (0, 0, height), base at Z=0 with given radius

		inputLayoutDesc.clear();
		inputLayoutDesc.addElement(0, EVertex::ATTRIBUTE_POSITION, EVertex::Float3);
		inputLayoutDesc.addElement(0, EVertex::ATTRIBUTE_NORMAL, EVertex::Float3);

		struct MyVertex { Vector3 pos; Vector3 normal; };

		// Side base ring + tip + bottom cap ring + bottom center
		int numVerts = numSide * 2 + 2;
		data.vertexData.resize(numVerts * sizeof(MyVertex));
		MyVertex* verts = (MyVertex*)data.vertexData.data();

		float rf = 2.0f * Math::PI / numSide;

		// Side vertices (base ring in XY plane, outward+forward normals)
		for (int i = 0; i < numSide; ++i)
		{
			float s, c;
			Math::SinCos(i * rf, s, c);
			verts[i].pos = Vector3(radius * c, radius * s, 0.0f);
			Vector3 outward(c, s, 0.5f);
			verts[i].normal = outward.getNormal();
		}
		// Tip vertex
		verts[numSide].pos = Vector3(0, 0, height);
		verts[numSide].normal = Vector3(0, 0, 1);

		// Bottom cap: duplicate base ring with -Z normal
		for (int i = 0; i < numSide; ++i)
		{
			float s, c;
			Math::SinCos(i * rf, s, c);
			verts[numSide + 1 + i].pos = Vector3(radius * c, radius * s, 0.0f);
			verts[numSide + 1 + i].normal = Vector3(0, 0, -1);
		}
		// Bottom center
		verts[numSide * 2 + 1].pos = Vector3(0, 0, 0);
		verts[numSide * 2 + 1].normal = Vector3(0, 0, -1);

		data.indexData.resize(numSide * 6);
		uint32* iPtr = data.indexData.data();

		int tipIdx = numSide;
		// Side triangles
		for (int i = 0; i < numSide; ++i)
		{
			int next = (i + 1) % numSide;
			iPtr[0] = i;
			iPtr[1] = next;
			iPtr[2] = tipIdx;
			iPtr += 3;
		}
		// Bottom cap triangles
		int centerIdx = numSide * 2 + 1;
		int baseOffset = numSide + 1;
		for (int i = 0; i < numSide; ++i)
		{
			int next = (i + 1) % numSide;
			iPtr[0] = baseOffset + next;
			iPtr[1] = baseOffset + i;
			iPtr[2] = centerIdx;
			iPtr += 3;
		}

		return true;
	}

	//t^2 - t - 1 = 0
	static float const IcoFactor = 1.6180339887498948482;
	static float const IcoA = 0.52573111121191336060;
	static float const IcoB = IcoFactor * IcoA;
	static float const IcoVertex[] =
	{
		-IcoA,IcoB,0, IcoA,IcoB,0, -IcoA,-IcoB,0, IcoA,-IcoB,0,
		0,-IcoA,IcoB, 0,IcoA,IcoB, 0,-IcoA,-IcoB, 0,IcoA,-IcoB,
		IcoB,0,-IcoA, IcoB,0,IcoA, -IcoB,0,-IcoA, -IcoB,0,IcoA,
	};
	static int IcoIndex[] =
	{
		0,11,5, 0,5,1, 0,1,7, 0,7,10, 0,10,11,
		1,5,9,  5,11,4, 11,10,2, 10,7,6, 7,1,8,
		3,9,4,  3,4,2, 3,2,6, 3,6,8, 3,8,9,
		4,9,5, 2,4,11, 6,2,10, 8,6,7, 9,8,1,
	};
	static int const IcoFaceNum = ARRAY_SIZE(IcoIndex) / 3;
	static int const IcoVertexNum = ARRAY_SIZE(IcoVertex) / 3;

	template< class VertexTraits >
	class TIcoSphereBuilder
	{
	public:
		using VertexType = typename VertexTraits::Type;
		void init(int numDiv, float radius)
		{
			mRadius = radius;
			int nv = 10 * (1 << (2 * numDiv)) + 2;
			mNumV = IcoVertexNum;
			mVertices.resize(nv);
			float const* pV = IcoVertex;
			for (int i = 0; i < IcoVertexNum; ++i)
			{
				VertexType& vtx = mVertices[i];
				VertexTraits::SetVertex(vtx, radius, *reinterpret_cast<Vector3 const*>(pV));
				pV += 3;
			}
		}
		int addVertex(Vector3 const& v)
		{
			int idx = mNumV++;
			VertexType& vtx = mVertices[idx];
			VertexTraits::SetVertex(vtx, mRadius, v);
			return idx;
		}
		int divVertex(uint32 i1, uint32 i2)
		{
			KeyType key = Math::SymmetricPairingFunction(i1, i2);
			KeyMap::iterator iter = mKeyMap.find(key);
			if (iter != mKeyMap.end())
			{
				return iter->second;
			}
			int idx = addVertex(0.5 * (mVertices[i1].v + mVertices[i2].v));
			mKeyMap.insert(iter, std::make_pair(key, idx));
			return idx;
		}

		bool build(MeshBuildData& data, float radius, int numDiv)
		{
			init(numDiv, radius);

			int nIdx = 3 * IcoFaceNum * (1 << (2 * numDiv));
			TArray< uint32 > indices(2 * nIdx);
			std::copy(IcoIndex, IcoIndex + ARRAY_SIZE(IcoIndex), &indices[0]);

			uint32* pIdx = &indices[0];
			uint32* pIdxPrev = &indices[nIdx];
			int numFace = IcoFaceNum;
			for (int i = 0; i < numDiv; ++i)
			{
				std::swap(pIdx, pIdxPrev);

				uint32* pTri = pIdxPrev;
				uint32* pIdxFill = pIdx;
				for (int n = 0; n < numFace; ++n)
				{
					int i0 = divVertex(pTri[1], pTri[2]);
					int i1 = divVertex(pTri[2], pTri[0]);
					int i2 = divVertex(pTri[0], pTri[1]);

					pIdxFill[0] = pTri[0]; pIdxFill[1] = i2; pIdxFill[2] = i1; pIdxFill += 3;
					pIdxFill[0] = i0; pIdxFill[1] = pTri[2]; pIdxFill[2] = i1; pIdxFill += 3;
					pIdxFill[0] = i0; pIdxFill[1] = i2; pIdxFill[2] = pTri[1]; pIdxFill += 3;
					pIdxFill[0] = i0; pIdxFill[1] = i1; pIdxFill[2] = i2; pIdxFill += 3;

					pTri += 3;
				}
				numFace *= 4;
			}

			data.vertexData.assign((uint8*)mVertices.data(), (uint8*)(mVertices.data() + mNumV));
			data.indexData.assign(pIdx, pIdx + nIdx);
			return true;
		}
		int    mNumV;
		float  mRadius;
		using KeyType = uint32;
		using KeyMap = std::unordered_map< KeyType, int >;
		TArray< VertexType > mVertices;
		KeyMap  mKeyMap;
	};


	struct VertexTraits_PN
	{
		struct Type
		{
			Vector3 v;
			Vector3 n;
		};
		static void SetVertex(Type& vtx, float radius, Vector3 const& pos)
		{
			vtx.n = Math::GetNormal(pos);
			vtx.v = radius * vtx.n;
		}
	};

	bool FMeshBuild::IcoSphere(Mesh& mesh, float radius, int numDiv)
	{
		MeshBuildData buildData;
		if (!IcoSphere(buildData, mesh.mInputLayoutDesc, radius, numDiv))
			return false;

		mesh.mType = EPrimitive::TriangleList;
		return buildData.initializeRHI(mesh);
	}

	bool FMeshBuild::IcoSphere(MeshBuildData& data, InputLayoutDesc& inputLayoutDesc, float radius, int numDiv)
	{
		inputLayoutDesc.clear();
		inputLayoutDesc.addElement(0, EVertex::ATTRIBUTE_POSITION, EVertex::Float3);
		inputLayoutDesc.addElement(0, EVertex::ATTRIBUTE_NORMAL, EVertex::Float3);

		TIcoSphereBuilder< VertexTraits_PN > builder;
		return builder.build(data, radius, numDiv);
	}


	template< class VertexTraits >
	class TOctSphereBuilder
	{
	public:
		using VertexType = typename VertexTraits::Type;
		TOctSphereBuilder(MeshBuildData& buildData)
			:mBuidlData(buildData)
		{

		}

		bool build(float radius, int level)
		{
			mVertexCount = 0;
			mRadius = radius;

			int const FaceCount = 4;
			mBuidlData.vertexData.resize(( 2 + FaceCount * Math::Square(level + 1)) * sizeof(VertexType) );
			mBuidlData.indexData.resize( 3 * 2 * FaceCount * Math::Square(level + 1));
			uint32* pIndex = mBuidlData.indexData.data();
			VertexType* pVertex = (VertexType*)mBuidlData.vertexData.data();

			auto AddVertex = [&](Vector3 const& v)
			{
				VertexTraits::SetVertex(pVertex[mVertexCount], mRadius, v);
				++mVertexCount;
			};

			auto AddTriangle = [&](uint32 i0, uint32 i1, uint32 i2)
			{
				pIndex[0] = i0;
				pIndex[1] = i1;
				pIndex[2] = i2;
				pIndex += 3;
			};
			auto AddTrianglePair = [&](uint32 i0, uint32 i1, uint32 i2)
			{
				AddTriangle(i0, i1, i2);
				AddTriangle(i0 + 1, i2 + 1, i1 + 1);
			};

			float const dPhi = 0.5 * Math::PI / float(level + 1);
			uint32 indexLayerPrev = 0;
			AddVertex(Vector3(0, 0, 1));
			AddVertex(Vector3(0, 0, -1));
			for (int layer = 0; layer < level; ++layer)
			{
				uint32 const indexLayer = mVertexCount;

				float r, z;
				Math::SinCos((layer + 1) * dPhi, r, z);

				int const faceLen = layer + 1;
				int const ringLen = FaceCount * faceLen;
				float const dTheta = 2 * Math::PI / float(ringLen);
				float const offsetTheta = 0.5 * dTheta * faceLen;

				uint32 iRing = 0;
				for (uint32 iFace = 0; iFace < FaceCount; ++iFace)
				{
					for (uint32 faceOffset = 0; faceOffset < faceLen; ++faceOffset)
					{
						float x, y;
						Math::SinCos(iRing * dTheta - offsetTheta, y, x);
						uint32 index0 = mVertexCount;
						AddVertex(Vector3(r * x, r * y, z));
						AddVertex(Vector3(r * x, r * y, -z));

						uint32 index1 = (iRing == ringLen - 1) ? indexLayer : index0 + 2;
						uint32 indexPrev = indexLayerPrev + 2 * (iFace * layer + faceOffset);
						uint32 indexPrev0 = (iRing == ringLen - 1) ? indexLayerPrev : indexPrev;
						AddTrianglePair(index0, index1, indexPrev0);

						if (faceOffset != faceLen - 1)
						{
							uint32 indexPrev1 = (iRing == ringLen - 2) ? indexLayerPrev : indexPrev + 2;
							AddTrianglePair(index1, indexPrev1, indexPrev);
						}
						++iRing;
					}
				}

				indexLayerPrev = indexLayer;
			}

			{
				//Mid Ring
				uint32 const indexLayer = mVertexCount;

				int const layer = level;
				int const faceLen = layer + 1;
				int const ringLen = FaceCount * faceLen;
				float const dTheta = 2 * Math::PI / float(ringLen);
				float const offsetTheta = 0.5 * dTheta * faceLen;

				uint32 iRing = 0;
				for (uint32 iFace = 0; iFace < FaceCount; ++iFace)
				{	
					for (uint32 faceOffset = 0; faceOffset < faceLen; ++faceOffset)
					{
						float x, y;
						Math::SinCos(iRing * dTheta - offsetTheta, y, x);
						uint32 index0 = mVertexCount;
						AddVertex(Vector3(x, y, 0));

						uint32 index1 = (iRing == ringLen - 1) ? indexLayer : index0 + 1;
						uint32 indexPrev = indexLayerPrev + 2 * (iFace * layer + faceOffset);
						uint32 indexPrev0 = (iRing == ringLen - 1) ? indexLayerPrev : indexPrev;
						AddTriangle(index0, index1, indexPrev0);
						AddTriangle(index0, indexPrev0 + 1, index1);

						if (faceOffset != faceLen - 1)
						{
							uint32 indexPrev1 = (iRing == ringLen - 2) ? indexLayerPrev : indexPrev + 2;
							AddTriangle(index1, indexPrev1, indexPrev);
							AddTriangle(index1, indexPrev + 1, indexPrev1 + 1);
						}

						++iRing;
					}
				}
			}

			return true;
		}

		int    mVertexCount;
		float  mRadius;

		MeshBuildData& mBuidlData;
	};


	bool FMeshBuild::OctSphere(Mesh& mesh, float radius, int level)
	{
		MeshBuildData buildData;
		if (!OctSphere(buildData, mesh.mInputLayoutDesc, radius, level))
		{
			return false;
		}

		mesh.mType = EPrimitive::TriangleList;
		return buildData.initializeRHI(mesh);
	}

	bool FMeshBuild::OctSphere(MeshBuildData& data, InputLayoutDesc& inputLayoutDesc, float radius, int level)
	{
		inputLayoutDesc.clear();
		inputLayoutDesc.addElement(0, EVertex::ATTRIBUTE_POSITION, EVertex::Float3);
		inputLayoutDesc.addElement(0, EVertex::ATTRIBUTE_NORMAL, EVertex::Float3);

		TOctSphereBuilder< VertexTraits_PN > builder(data);
		return builder.build(radius, level);
	}

	bool FMeshBuild::LightSphere(Mesh& mesh)
	{
		MeshBuildData buildData;
		if (!LightSphere(buildData, mesh.mInputLayoutDesc))
			return false;

		return buildData.initializeRHI(mesh);
	}

	bool FMeshBuild::LightSphere(MeshBuildData& data, InputLayoutDesc& inputLayoutDesc)
	{
		inputLayoutDesc.addElement(0, EVertex::ATTRIBUTE_POSITION, EVertex::Float3);
		struct VertexTraits
		{
			struct Type
			{
				Vector3 v;
			};
			static void SetVertex(Type& vtx, float radius, Vector3 const& pos)
			{
				vtx.v = radius * Math::GetNormal(pos);
			}
		};
		TIcoSphereBuilder< VertexTraits > builder;
		return builder.build(data, 1.00, 4);
	}

	bool FMeshBuild::LightCone(Mesh& mesh)
	{
		MeshBuildData buildData;
		if (!LightCone(buildData, mesh.mInputLayoutDesc))
			return false;

		return buildData.initializeRHI(mesh);
	}

	bool FMeshBuild::LightCone(MeshBuildData& data, InputLayoutDesc& inputLayoutDesc)
	{
		int numSide = 96;
		inputLayoutDesc.clear();
		inputLayoutDesc.addElement(0, EVertex::ATTRIBUTE_POSITION, EVertex::Float3);

		int vertexSize = inputLayoutDesc.getVertexSize() / sizeof(float);

		int nV = numSide + 2;
		data.vertexData.resize(nV * inputLayoutDesc.getVertexSize());
		float const sf = 2 * Math::PI / numSide;

		float* v = (float*)data.vertexData.data() + inputLayoutDesc.getAttributeOffset(EVertex::ATTRIBUTE_POSITION) / sizeof(float);

		for (int i = 0; i < numSide; ++i)
		{
			float s, c;
			Math::SinCos(sf * i, s, c);
			v[0] = c;
			v[1] = s;
			v[2] = 1.0f;

			v += vertexSize;
		}

		{
			v[0] = 0;
			v[1] = 0;
			v[2] = 1.0f;

			v += vertexSize;

			v[0] = 0;
			v[1] = 0;
			v[2] = 0;

			v += vertexSize;
		}
		data.indexData.resize(2 * numSide * 3);
		uint32* idx = data.indexData.data();

		int idxPrev = numSide - 1;
		int idxA = numSide;
		int idxB = numSide + 1;
		for (int i = 0; i < numSide; ++i)
		{
			idx[0] = idxPrev;
			idx[1] = i;
			idx[2] = idxA;

			idx[3] = i;
			idx[4] = idxPrev;
			idx[5] = idxB;
			idx += 6;

			idxPrev = i;
		}

		return true;
	}


	bool FMeshBuild::SpritePlane(Mesh& mesh)
	{
		MeshBuildData buildData;
		if (!SpritePlane(buildData, mesh.mInputLayoutDesc))
			return false;

		return buildData.initializeRHI(mesh);
	}

	bool FMeshBuild::SpritePlane(MeshBuildData& data, InputLayoutDesc& inputLayoutDesc)
	{
		inputLayoutDesc.clear();
		inputLayoutDesc.addElement(0, EVertex::ATTRIBUTE_POSITION, EVertex::Float3);

		Vector3 v[] =
		{
			Vector3(1,1,0),
			Vector3(-1,1,0) ,
			Vector3(-1,-1,0),
			Vector3(1,-1,0),
		};
		uint32 idx[6] = { 0 , 1 , 2 , 0 , 2 , 3 };

		data.vertexData.assign((uint8*)v, (uint8*)(v + 4));
		data.indexData.assign(idx, idx + 6);

		return true;
	}

}//namespace Render