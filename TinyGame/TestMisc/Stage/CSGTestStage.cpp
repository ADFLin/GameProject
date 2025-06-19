#include "Stage/TestStageHeader.h"
#include "Stage/TestRenderStageBase.h"

#include "Math/Vector3.h"
#include "Math/Vector2.h"
#include "Math/GeometryPrimitive.h"
#include "GameRenderSetup.h"
#include "RHI/RHICommand.h"
#include "ProfileSystem.h"

#define DEBUG_CSG 1
#if DEBUG_CSG
#include "Async/Coroutines.h"
#include "Misc/DebugDraw.h"
int GDebugIndex = 0;
#define DEUBG_STOP_INDEX 97
#define DEBUG_RUNNING() Coroutines::IsRunning()
#define DEBUG_YEILD() if (DEBUG_RUNNING()){ CO_YEILD(nullptr); }
#define DEBUG_CHECK_STOP() (DEBUG_RUNNING() && GDebugIndex >= DEUBG_STOP_INDEX)
#define DEBUG_INC_INDEX() if (DEBUG_RUNNING()){  ++GDebugIndex; }
#define DEBUG_INDEX_BREAK() if (DEBUG_RUNNING() && GDebugIndex == DEUBG_STOP_INDEX){ ::DebugBreak(); }
#else
#define DEBUG_YEILD()
#define DEBUG_CHECK_STOP() false
#define DEBUG_RUNNING() false
#define DEBUG_INC_INDEX()
#define DEBUG_INDEX_BREAK()
#endif
#include "RHI/RHIGraphics2D.h"


namespace CSG
{
	using Math::Vector3;
	using Math::Vector2;
	using Math::Quaternion;
	using Math::Plane;
	namespace EPlaneSide = Math::EPlaneSide;


	void BuildLPS(char const* subStr, int lenSub, int* outLPS)
	{
		int indexSub = 0;
		int index = 1;
		while (index < lenSub)
		{
			if (subStr[index] == subStr[indexSub])
			{
				++indexSub;
				outLPS[index] = indexSub;
				++index;
			}
			else if (indexSub != 0)
			{
				indexSub = outLPS[indexSub - 1];
			}
			else
			{
				outLPS[index] = 0;
				++index;
			}
		}
	}

	int FindSubString(char const* str, int len, char const* subStr, int lenSub)
	{
		int* LPS = (int*)alloca(sizeof(int) * lenSub);
		BuildLPS(subStr, lenSub, LPS);

		int index = 0;
		int indexSub = 0;
		while (index < len)
		{
			if (str[index] == subStr[indexSub])
			{
				++indexSub;
				++index;
				if (indexSub == lenSub)
				{
					return index - indexSub;
					//indexSub = LPS[indexSub - 1];
				}
			}
			else if (indexSub != 0)
			{
				indexSub = LPS[indexSub - 1];
			}
			else
			{
				++index;
			}
		}

		return INDEX_NONE;
	}

	char const* FindSubString(char const* str, char const* subStr, int lenSub)
	{
		int* LPS = (int*)alloca(sizeof(int) * lenSub);
		BuildLPS(subStr, lenSub, LPS);

		int indexSub = 0;
		while (*str)
		{
			if (*str == subStr[indexSub])
			{
				++indexSub;
				++str;
				if (indexSub == lenSub)
				{
					return str - indexSub;
					//indexSub = LPS[indexSub - 1];
				}
			}
			else if (indexSub != 0)
			{
				indexSub = LPS[indexSub - 1];
			}
			else
			{
				++str;
			}
		}

		return nullptr;
	}

	char const* FindSubString(char const* str, char const* subStr)
	{
		return FindSubString(str, subStr, FCString::Strlen(subStr));
	}

	struct Transform
	{
		Transform(Vector3 const& inP, Quaternion const& inR = Quaternion(ForceInit), Vector3 const& inS = Vector3(1, 1, 1))
			:location(inP),roataion(inR),scale(inS){}

		Vector3 location;
		Quaternion roataion;
		Vector3  scale;

		static Transform Identity()
		{
			return { Vector3::Zero(), Quaternion(ForceInit), Vector3(1,1,1) };
		}

		Vector3 transformPos(Vector3 const& pos) const
		{
			return  location + roataion.rotate(scale * pos);
		}
		Vector3 transformVector(Vector3 const& pos) const
		{
			return  roataion.rotate(scale * pos);
		}
	};

	bool IsInsidePolygon(Vector3 const& p, Vector3 const v[], int nV)
	{
		for (int32 i = 0; i < nV; ++i)
		{
			float dot = Math::Dot(v[i], p - v[i]);
			if (dot > 0.0f)
			{
				return false;
			}
		}
		return true;
	}

#define USE_LAZY_INVERSE 1
	
	class Polygon
	{
	public:
#if USE_LAZY_INVERSE
		bool bInverse = false;
#endif
		uint32 surfaceId;
		TArray< Vector3 > vertices;
		void inverse()
		{
#if USE_LAZY_INVERSE
			bInverse = !bInverse;
#else
			std::reverse(vertices.begin(), vertices.end());
#endif
		}

		void standardize()
		{
#if USE_LAZY_INVERSE
			if (bInverse)
			{
				std::reverse(vertices.begin(), vertices.end());
				bInverse = false;
			}
#endif
		}

		bool checkVaild() const
		{
			for (int i = 0; i < vertices.size() - 1; ++i)
			{
				if (vertices[i] == vertices[i + 1])
				{
					return false;
				}
			}
			if (vertices[0] == vertices[vertices.size() - 1])
			{
				return false;
			}
			return true;
		}
		Plane getPlane() const
		{
			CHECK(vertices.size() >= 3);
#if USE_LAZY_INVERSE
			if (bInverse)
			{
				return Plane(vertices[0], vertices[2], vertices[1]);
			}
#endif
			return Plane(vertices[0], vertices[1], vertices[2]);
		}

		void swap(Polygon& other)
		{
			using std::swap;
#if USE_LAZY_INVERSE
			swap(bInverse, other.bInverse);
#endif
			swap(vertices, other.vertices);
			swap(surfaceId, other.surfaceId);
		}
	};

	static float GetConvexArea(TArray<Vector3> const& vertixes)
	{
		if (vertixes.size() < 3)
			return 0.0;

		Vector3 d1 = vertixes[1] - vertixes[0];
		Vector3 d2 = vertixes[2] - vertixes[0];
		Vector3 dPrev = d2;
		Vector3 normal = d1.cross(d2);
		float result = normal.normalize();
		for (int i = 3; i < vertixes.size(); ++i)
		{
			Vector3 d = vertixes[i] - vertixes[0];
			result += normal.dot(dPrev.cross(d));
			dPrev = d;
		}
		return 0.5 * result;
	}

	struct BSPNode
	{
		Plane plane;
		TArray<Polygon> polygons;
		int indexFront = INDEX_NONE;
		int indexBack = INDEX_NONE;

		void inverse()
		{
			plane.inverse();
			for (auto& poly : polygons)
			{
				poly.inverse();
			}
		}
	};

	template< typename TFunc, typename T, typename ...Ts >
	FORCEINLINE static void Invoke(TFunc& func, T& t, Ts& ...ts)
	{
		func(t);
		Invoke(func, ts...);
	}
	template< typename TFunc >
	FORCEINLINE static void Invoke(TFunc& func) {}

	using namespace Render;
#if DEBUG_CSG


	template <typename T>
	struct TPrimitiveVisualDebuger
	{
		template <typename T>
		struct Primitive : T
		{
			Primitive(T const& obj, LinearColor const& c) :T(obj), color(c) {}
			LinearColor color;
		};

		void add(T const& obj, Color4f const& color)
		{
			mPrimitives.emplace_back(obj, color);
		}

		void clear() { mPrimitives.clear(); }

		void draw(RHICommandList& command)
		{
			for (auto const& primitive : mPrimitives)
			{
				FDrawer::Draw(command, primitive, primitive.color);
			}
		}

		TArray<Primitive<T>> mPrimitives;
	};

	template <typename ...Mixins>
	struct TVisualDebugerMixins : Mixins...
	{
		using Mixins::add...;

		struct DrawFunc
		{
			DrawFunc(RHICommandList& commandlist)
				:commandlist(commandlist)
			{
			}
			template< typename T >
			void operator()(T& t) { t.draw(commandlist); }
			RHICommandList& commandlist;
		};
		void draw(RHICommandList& commandlist, ViewInfo const& view)
		{
			FDrawer::SetupView(view);
			Invoke(DrawFunc(commandlist), static_cast<Mixins&>(*this)...);
		}

		struct ClearFunc
		{
			template< typename T >
			void operator()(T& t) { t.clear(); }
		};
		void clear()
		{
			Invoke(ClearFunc(), static_cast<Mixins&>(*this)...);
		}
	};

	template <typename ...Ts>
	struct TVisualDebuger : TVisualDebugerMixins< TPrimitiveVisualDebuger<Ts>... >
	{
	};

	using VisualDebuger = TVisualDebuger<Plane, Polygon, Vector3>;
	VisualDebuger GDebuger;
#define DEBUG_SHOW(OBJ , COLOR) GDebuger.add(OBJ, COLOR)


#else
#define DEBUG_SHOW(OBJ , COLOR)
#endif

	struct FDrawer
	{
		static void Draw(RHICommandList& commandList, Polygon const& poly, LinearColor const& color)
		{
			TRenderRT< RTVF_XYZ >::Draw(commandList, EPrimitive::Polygon, poly.vertices.data(), poly.vertices.size(), color);
		}

		static void Draw(RHICommandList& commandList, Plane const& plane, LinearColor const& color)
		{
			Vector3 center = plane.getAnyPoint();
			Vector3 const& normal = plane.getNormal();
			Vector3 uAxis = Vector3(1, 0, 0);
			float NoU = normal.dot(uAxis);
			if (1 - Math::Abs(NoU) < 1e-4)
			{
				uAxis = Vector3(0, 0, 1);
				NoU = normal.dot(uAxis);
			}
			float length = 50.0;
			uAxis = length * Math::GetNormal(uAxis - NoU * normal);
			Vector3 vAxis = normal.cross(uAxis);

			Vector3 v[] =
			{
				center - uAxis - vAxis,
				center + uAxis - vAxis,
				center + uAxis + vAxis,
				center - uAxis + vAxis,
			};
			TRenderRT< RTVF_XYZ >::Draw(commandList, EPrimitive::Quad, v, 4, color);
		}



		static ViewInfo const* mView;
		static Vector3 ViewAxisX;
		static Vector3 ViewAxisY;
		static void SetupView(ViewInfo const& view)
		{
			mView = &view;
			ViewAxisX = Math::TransformVectorInverse(Vector3(1, 0, 0), mView->worldToView);
			ViewAxisY = Math::TransformVectorInverse(Vector3(0, 1, 0), mView->worldToView);
		}

		static void Draw(RHICommandList& commandList, Vector3 const& point, LinearColor const& color)
		{
			Vector3 viewPoint = point * mView->worldToView;
			float len = 5 * viewPoint.z / mView->rectSize.x;
			Vector3 xAxis = len * ViewAxisX;
			Vector3 yAxis = len * ViewAxisY;
			Vector3 v[] =
			{
				point - xAxis - yAxis,
				point - xAxis + yAxis,
				point + xAxis + yAxis,
				point + xAxis - yAxis,
			};
			TRenderRT< RTVF_XYZ >::Draw(commandList, EPrimitive::Quad, v, 4, color);
		}
	};
	ViewInfo const* FDrawer::mView;
	Vector3 FDrawer::ViewAxisX;
	Vector3 FDrawer::ViewAxisY;

	struct FGeometry
	{

		static uint32 NewSurfaceId()
		{
			static uint32 StaticId = 0;
			return StaticId++;
		}
		static void Rect(Transform const& xform, Vector3 const& center, Vector3 const& offsetU, Vector3 const& offsetV, Polygon& outPoly)
		{
			Rect(xform.transformPos(center), xform.transformVector(offsetU), xform.transformVector(offsetV), outPoly);
		}

		static void Rect(Vector3 const& center, Vector3 const& offsetU, Vector3 const& offsetV, Polygon& outPoly)
		{
			auto& vertices = outPoly.vertices;
			vertices.push_back(center - offsetU - offsetV);
			vertices.push_back(center + offsetU - offsetV);
			vertices.push_back(center + offsetU + offsetV);
			vertices.push_back(center - offsetU + offsetV);
			outPoly.surfaceId = NewSurfaceId();
		}

		static void RegularPolygon(Transform const& xform, float radius, int sides, bool bAlignToSide, Polygon& outPoly)
		{
			float deltaTheta = 2 * Math::PI / float(sides);
			float theta = bAlignToSide ? -0.5f * deltaTheta : 0.0f;
			outPoly.vertices.resize(sides);
			for (int i = 0; i < sides; ++i)
			{
				float c, s;
				Math::SinCos(theta, s, c);
				outPoly.vertices[i] = xform.transformPos(Vector3(radius * c, radius * s, 0));
				theta += deltaTheta;
			}
			outPoly.surfaceId = NewSurfaceId();
		}

		static void Box(Transform const& xform, Vector3 const& halfExtent, TArray<Polygon>& outPolyList)
		{
			Polygon poly;
			Rect(xform, Vector3(halfExtent.x, 0, 0), Vector3(0, halfExtent.y, 0), Vector3(0, 0, halfExtent.z), poly);
			outPolyList.push_back(std::move(poly));
			Rect(xform, Vector3(-halfExtent.x, 0, 0), Vector3(0, 0, halfExtent.z), Vector3(0, halfExtent.y, 0), poly);
			outPolyList.push_back(std::move(poly));
			Rect(xform, Vector3(0, halfExtent.y, 0), Vector3(0, 0, halfExtent.z), Vector3(halfExtent.x, 0, 0), poly);
			outPolyList.push_back(std::move(poly));
			Rect(xform, Vector3(0, -halfExtent.y, 0), Vector3(halfExtent.x, 0, 0), Vector3(0, 0, halfExtent.z), poly);
			outPolyList.push_back(std::move(poly));
			Rect(xform, Vector3(0, 0, halfExtent.z), Vector3(halfExtent.x, 0, 0), Vector3(0, halfExtent.y, 0), poly);
			outPolyList.push_back(std::move(poly));
			Rect(xform, Vector3(0, 0, -halfExtent.z), Vector3(0, halfExtent.y, 0), Vector3(halfExtent.x, 0, 0), poly);
			outPolyList.push_back(std::move(poly));
		}

		static void Cylinder(Transform const& xform, float radius, float height, int sides, bool bAlignToSide, TArray<Polygon>& outPolyList)
		{
			float zOffset = 0.5 * height;

			float deltaTheta = 2 * Math::PI / float(sides);

			Vector3* pSideV = (Vector3*)_alloca(sides * sizeof(Vector3));

			float theta = bAlignToSide ? -0.5f * deltaTheta : 0.0f;
			for (int i = 0; i < sides; ++i)
			{
				float c, s;
				Math::SinCos(theta, s, c);
				pSideV[i] = xform.transformPos(Vector3(radius * c, radius * s, 0));
				theta += deltaTheta;
			}

			Vector3 upOffset = xform.transformVector(Vector3(0, 0, zOffset));
			Polygon poly;
			poly.vertices.resize(sides);
			for (int i = 0; i < sides; ++i)
			{
				poly.vertices[i] = pSideV[i] + upOffset;
			}
			poly.surfaceId = NewSurfaceId();
			outPolyList.push_back(std::move(poly));
			poly.vertices.resize(sides);
			for (int i = 0; i < sides; ++i)
			{
				poly.vertices[sides - i - 1] = pSideV[i] - upOffset;
			}
			poly.surfaceId = NewSurfaceId();
			outPolyList.push_back(std::move(poly));

			for (int i = 0; i < sides; ++i)
			{
				Vector3 const& v0 = pSideV[i];
				Vector3 const& v1 = pSideV[(i + 1) % sides];
				Rect(0.5 * (v1 + v0), 0.5 * (v1 - v0), upOffset, poly);
				outPolyList.push_back(std::move(poly));
			}
		}

		static void Cone(Transform const& xform, float radius, float height, int sides, bool bAlignToSide, TArray<Polygon>& outPolyList)
		{
			float zOffset = 0.5 * height;
			float deltaTheta = 2 * Math::PI / float(sides);
			Vector3 upOffset = xform.transformVector(Vector3(0, 0, zOffset));
			Vector3* pSideV = (Vector3*)_alloca(sides * sizeof(Vector3));
	
			float theta = bAlignToSide ? -0.5f * deltaTheta : 0.0f;
			for (int i = 0; i < sides; ++i)
			{
				float c, s;
				Math::SinCos(theta, s, c);
				pSideV[i] = xform.transformPos(Vector3(radius * c, radius * s, 0)) - upOffset;
				theta += deltaTheta;
			}

			Polygon poly;
			poly.vertices.resize(sides);
			for (int i = 0; i < sides; ++i)
			{
				poly.vertices[sides - i - 1] = pSideV[i];
			}
			poly.surfaceId = NewSurfaceId();
			outPolyList.push_back(std::move(poly));
			Vector3 vertexPos = xform.location + upOffset;

			for (int i = 0; i < sides; ++i)
			{
				Vector3 const& v0 = pSideV[i];
				Vector3 const& v1 = pSideV[(i + 1) % sides];
				auto& vertices = poly.vertices;
				vertices.push_back(v0);
				vertices.push_back(v1);
				vertices.push_back(vertexPos);
				poly.surfaceId = NewSurfaceId();
				outPolyList.push_back(std::move(poly));
			}
		}

		static void IcoSphere(Transform const& xform, float radius, int numDiv, TArray<Polygon>& outPolyList)
		{

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

			class IcoSphereBuilder
			{
			public:
				IcoSphereBuilder(TArray<Polygon>& outPolyList)
					:outPolyList(outPolyList)
				{

				}

				static void SetVertex(Vector3& vtx, float radius, Vector3 const& pos)
				{
					Vector3 normal = Math::GetNormal(pos);
					vtx = radius * normal;
				}

				void init(int numDiv, float radius)
				{
					mRadius = radius;
					int nv = 10 * (1 << (2 * numDiv)) + 2;
					mNumV = IcoVertexNum;
					mVertices.resize(nv);
					float const* pV = IcoVertex;
					for (int i = 0; i < IcoVertexNum; ++i)
					{
						Vector3& vtx = mVertices[i];
						SetVertex(vtx, radius, *reinterpret_cast<Vector3 const*>(pV));
						pV += 3;
					}
				}
				int addVertex(Vector3 const& v)
				{
					int idx = mNumV++;
					Vector3& vtx = mVertices[idx];
					SetVertex(vtx, mRadius, v);
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
					int idx = addVertex(0.5 * (mVertices[i1] + mVertices[i2]));
					mKeyMap.insert(iter, std::make_pair(key, idx));
					return idx;
				}

				void build(Transform const& xform, float radius, int numDiv)
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

					for (auto& v : mVertices)
					{
						v = xform.transformPos(v);
					}

					Polygon poly;
					for (int i = 0; i < nIdx; i += 3)
					{
						auto& vertices = poly.vertices;
						uint32 i0 = pIdx[0];
						uint32 i1 = pIdx[1];
						uint32 i2 = pIdx[2];
						vertices.push_back(mVertices[i0]);
						vertices.push_back(mVertices[i1]);
						vertices.push_back(mVertices[i2]);
						poly.surfaceId = NewSurfaceId();
						outPolyList.push_back(std::move(poly));
						pIdx += 3;
					}
				}
				int    mNumV;
				float  mRadius;
				using KeyType = uint32;
				using KeyMap = std::unordered_map< KeyType, int >;
				TArray< Vector3 > mVertices;
				KeyMap  mKeyMap;
				TArray<Polygon>& outPolyList;
			};
			IcoSphereBuilder builder(outPolyList);
			builder.build(xform, radius, numDiv);
		}
	};

	enum ESplitResult
	{
		Front = EPlaneSide::Front,
		In    = EPlaneSide::In,
		Back  = EPlaneSide::Back,
		Split = 2,
	};


	struct BrushBase
	{



	};


	class BSPTree
	{
	public:

		void add(TArray<Polygon>&& polygonList)
		{
			if (mNodes.empty())
			{
				BSPNode* node = new (mNodes) BSPNode;
			}
			add(0, polygonList);
		}

		void add(int indexNode, TArray<Polygon>& polygonList)
		{
			TArray<Polygon> backList;
			TArray<Polygon> frontList;
			{
				BSPNode& node = mNodes[indexNode];
				int index = 0;
				if (node.polygons.empty())
				{
					node.plane = polygonList[0].getPlane();
					node.polygons.push_back(std::move(polygonList[0]));
					++index;
				}
				Polygon splitPolygon;
				for (; index < polygonList.size(); ++index)
				{
					auto& polygon = polygonList[index];
					switch (SplitPolygon(node.plane, polygonList[index], splitPolygon))
					{
					case ESplitResult::Front:
						frontList.push_back(std::move(polygon));
						break;
					case ESplitResult::Back:
						backList.push_back(std::move(polygon));
						break;
					case ESplitResult::In:
						node.polygons.push_back(std::move(polygon));
						break;
					case ESplitResult::Split:
						frontList.push_back(std::move(polygon));
						backList.push_back(std::move(splitPolygon));
						break;
					}
				}
			}

			if (!frontList.empty())
			{
				BSPNode& node = mNodes[indexNode];
				int index = node.indexFront;
				if (index == INDEX_NONE)
				{
					index = mNodes.size();
					node.indexFront = index;
					BSPNode* node = new (mNodes) BSPNode;
				}
				add(index, frontList);
			}
			if (!backList.empty())
			{
				BSPNode& node = mNodes[indexNode];
				int index = node.indexBack;
				if (index == INDEX_NONE)
				{
					index = mNodes.size();
					node.indexBack = index;
					BSPNode* node = new (mNodes) BSPNode;
				}
				add(index, backList);
			}
		}


		class ClipOp
		{
		public:
			ClipOp(BSPTree& tree, bool bReserveFront, TArray<Polygon>& outPolyList)
				:tree(tree)
				,bReserveFront(bReserveFront)
				,outPolyList(outPolyList)
			{

			}

			void execute(BSPTree& other)
			{
				TArray<Polygon> polyList;
				for (auto& otherNode : other.mNodes)
				{
					for (auto& polygon : otherNode.polygons)
					{
						polyList.push_back(std::move(polygon));
					}
				}
				executeNode(tree.mNodes[0], polyList);
			}

			void executeNode(BSPNode const& node, TArray<Polygon>& polyList)
			{
				TArray<Polygon> backList;
				TArray<Polygon> frontList;

				for (auto& polygon : polyList)
				{
					DEBUG_INC_INDEX();
					if (DEBUG_CHECK_STOP())
					{
						DEBUG_SHOW(polygon, Color3f(1, 0.5, 0.5));
						DEBUG_SHOW(node.plane, Color3f(1, 1, 1));
						DEBUG_YEILD();
					}

					switch (SplitPolygon(node.plane, polygon, splitPolygon))
					{
					case ESplitResult::Front:
						frontList.push_back(std::move(polygon));
						break;
					case ESplitResult::Back:
						backList.push_back(std::move(polygon));
						break;
					case ESplitResult::In:
						{
							bool bSameSide = node.plane.getNormal().dot(polygon.getPlane().getNormal()) > 0;
							auto& list = (bSameSide == bReserveFront) ? frontList : backList;
							list.push_back(std::move(polygon));
						}
						break;
					case ESplitResult::Split:

						if (DEBUG_CHECK_STOP())
						{					
							DEBUG_SHOW(node.plane, Color3f(0.5, 0.5, 0.5));
							DEBUG_SHOW(polygon, Color3f(0, 1, 0.5));
							DEBUG_SHOW(splitPolygon, Color3f(0, 0.5, 1));
							DEBUG_YEILD();
						}

						frontList.push_back(std::move(polygon));
						backList.push_back(std::move(splitPolygon));
						break;
					}

				}

				if (node.indexFront != INDEX_NONE)
				{
					executeNode(tree.mNodes[node.indexFront], frontList);
				}
				else if (bReserveFront)
				{
					for (auto poly : frontList)
					{
						outPolyList.push_back(std::move(poly));
					}
				}

				if (node.indexBack != INDEX_NONE)
				{
					executeNode(tree.mNodes[node.indexBack], backList);
				}
				else if (!bReserveFront)
				{
					for (auto poly : backList)
					{
						outPolyList.push_back(std::move(poly));
					}
				}
			}

			BSPTree& tree;
			TArray<Polygon>&  outPolyList;
			Polygon splitPolygon;
			bool    bReserveFront;
		};

		void clip(BSPTree& other, bool bReserveFront, TArray<Polygon>& outPolyList)
		{
			CHECK(mNodes.size() > 0);
			ClipOp op(*this, bReserveFront, outPolyList);
			op.execute(other);
		}


		static ESplitResult SplitPolygon(Plane const& plane, Polygon& polygon, Polygon& splitPolygon)
		{
			float Error = 1e-4;
			ESplitResult result = ESplitResult::In;
			for (int index = 0; index < polygon.vertices.size(); ++index)
			{
				auto const& vertex = polygon.vertices[index];
				float dist;
				auto side = plane.testSide(vertex, Error, dist);
				if (side != EPlaneSide::In)
				{
					if (result == ESplitResult::In)
					{
						result = (ESplitResult)side;
					}
					else if (side != result)
					{
						result = ESplitResult::Split;
						break;
					}
				}
			}
			if (result != ESplitResult::Split)
				return result;

			TArray<Vector3> front;
			TArray<Vector3> back;

			int indexPrev = polygon.vertices.size() - 1;
			auto sidePrev = plane.testSide(polygon.vertices[indexPrev], Error);
			for (int index = 0; index < polygon.vertices.size(); ++index)
			{
				auto const& vertex = polygon.vertices[index];
				float dist;
				auto side = plane.testSide(vertex, Error, dist);
				if (side != sidePrev && sidePrev != EPlaneSide::In && side != EPlaneSide::In)
				{
					auto clipV = plane.getIntersection(polygon.vertices[indexPrev], polygon.vertices[index]);
					front.push_back(clipV);
					back.push_back(clipV);
				}

				switch (side)
				{
				case EPlaneSide::Front:
					front.push_back(vertex);
					break;
				case EPlaneSide::In:
					front.push_back(vertex);
					back.push_back(vertex);
					break;
				case EPlaneSide::Back:
					back.push_back(vertex);
					break;
				}
				indexPrev = index;
				sidePrev = side;
			}
			float AreaError = 1e-4;
			if (GetConvexArea(front) < AreaError)
			{
				return ESplitResult::Back;
			}
			if (GetConvexArea(back) < AreaError)
			{
				return ESplitResult::Front;
			}
			polygon.vertices = std::move(front);
			splitPolygon.vertices = std::move(back);
			splitPolygon.surfaceId = polygon.surfaceId;
#if USE_LAZY_INVERSE
			splitPolygon.bInverse = polygon.bInverse;
#endif

			polygon.checkVaild();
			splitPolygon.checkVaild();
			return result;
		}

		void inverse()
		{
			for (auto& node : mNodes)
			{
				node.inverse();
			}
		}

		void clone(BSPTree& outTree) const
		{
			outTree.mNodes = mNodes;
		}

		TArray< BSPNode > mNodes;
	};
	

	static int indexColor = 0;

	struct CSGOp
	{
		// A U B
		static void Union(BSPTree const& A, BSPTree const& B, TArray<Polygon>& outResult)
		{
			BSPTree copyA;
			A.clone(copyA);
			BSPTree copyB;
			B.clone(copyB);
			copyB.clip(copyA, true, outResult);
			copyA.clip(copyB, true, outResult);
		}

		// A ¡ä B
		static void Intersection(BSPTree const& A, BSPTree const& B, TArray<Polygon>& outResult)
		{
			BSPTree copyA;
			A.clone(copyA);
			BSPTree copyB;
			B.clone(copyB);
			copyB.clip(copyA, false, outResult);
			copyA.clip(copyB, false, outResult);
		}

		// A - B
		static void Difference(BSPTree const& A, BSPTree const& B, TArray<Polygon>& outResult)
		{
			BSPTree copyA;
			A.clone(copyA);
			BSPTree copyB;
			B.clone(copyB);
			copyB.clip(copyA, true, outResult);
			copyB.inverse();
			copyA.clip(copyB, false, outResult);
		}
		static void Copy(Polygon& polyA, int indexA, Polygon& polyB, int indexB, int num)
		{
			for (int i = num; i ; --i)
			{
				polyA.vertices[indexA % polyA.vertices.size()] = polyB.vertices[indexB % polyB.vertices.size()];
				++indexA;
				++indexB;
			}
		}
		static bool TryMerge(Polygon& polyA, Polygon& polyB)
		{
			for (int i = 0; i < polyA.vertices.size(); ++i)
			{
				auto& vA = polyA.vertices[i];
				for (int j = 0; j < polyB.vertices.size(); ++j)
				{
					auto& vB = polyB.vertices[j];

					if ( !(vA == vB) )
						continue;

					int iNext = (i + 1) % polyA.vertices.size();
					int jPrev = (j + polyB.vertices.size() - 1) % polyB.vertices.size();
					auto& vANext = polyA.vertices[iNext];

					if (!(vANext == polyB.vertices[jPrev]))
						continue;
#if 0
					DEBUG_INC_INDEX();
					if (DEBUG_CHECK_STOP())
					{
						DEBUG_SHOW(polyA, Color3f(1, 1, 1));

						for (int i = 0; i < polyA.vertices.size(); ++i)
						{
							DEBUG_SHOW(polyA.vertices[i], (i % 2) ? Color3f(1, 0.5, 0) : Color3f(0, 0.5, 1));
							DEBUG_YEILD();
						}
						for (int i = 0; i < polyB.vertices.size(); ++i)
						{
							DEBUG_SHOW(polyB.vertices[i], (i % 2) ? Color3f(1, 0.5, 0) : Color3f(0, 0.5, 1));
							DEBUG_YEILD();
						}

						DEBUG_SHOW(polyB, Color3f(0, 0, 0));
						DEBUG_YEILD();
					}
#endif
					auto CheckCollinear = [&](int ia, int ib, Vector3 const& v)
					{
						Vector3 const& a = polyA.vertices[ia];
						Vector3 const& b = polyB.vertices[ib];

						Vector3 va = v - a;
						if (va.normalize() == 0)
							return true;
						Vector3 ba = b - a;
						if (ba.normalize() == 0)
							return true;

						return Math::Abs(1 - va.dot(ba)) < 1e-6;
					};
					int nInsert = polyB.vertices.size() - 2;
					int indexInsert = iNext;
#if 1
					if (CheckCollinear((i + polyA.vertices.size() - 1) % polyA.vertices.size(), (j + 1) % polyB.vertices.size(), vA))
					{
						--nInsert;
						indexInsert = i;
					}
					if (CheckCollinear((iNext + 1) % polyA.vertices.size(), (jPrev + polyB.vertices.size() - 1) % polyB.vertices.size(), vANext))
					{
						--nInsert;
					}
#endif

					if (LIKELY(nInsert > 0))
					{
						polyA.vertices.insertAt(indexInsert, nInsert, Vector3());
					}
					else if (nInsert < 0)
					{
						CHECK(nInsert == -1 && polyB.vertices.size() == 3);
						polyA.vertices.erase(polyA.vertices.begin() + iNext);
						if (indexInsert > iNext)
						{
							--indexInsert;
						}
					}
					Copy(polyA, indexInsert, polyB, j + 1, polyB.vertices.size() - 2);

#if 0
					if (DEBUG_CHECK_STOP())
					{
						for (int i = 0; i < polyA.vertices.size(); ++i)
						{
							DEBUG_SHOW(polyA.vertices[i].pos, (i % 2) ? Color3f(1, 0.5, 0) : Color3f(0, 0.5, 1));
							DEBUG_YEILD();
						}
					}
#endif
					polyA.checkVaild();
					return true;
				}
			}

			return false;
		}

		static void Merge(TArray<Polygon>& outPolyList)
		{
			for (int i = 0; i < outPolyList.size(); ++i)
			{
				bool bRemoved = false;
				{
					auto& polyA = outPolyList[i];
					for (int j = i + 1; j < outPolyList.size(); ++j)
					{
						auto& polyB = outPolyList[j];
						if (polyA.surfaceId != polyB.surfaceId)
							continue;

#if 1
						DEBUG_INC_INDEX();
						if (DEBUG_CHECK_STOP())
						{
							DEBUG_SHOW(polyA, Color3f(1, 1, 1));
							DEBUG_SHOW(polyB, Color3f(0, 0, 0));
							DEBUG_YEILD();
						}
#endif

						if (!TryMerge(polyA, polyB))
							continue;

						outPolyList.removeIndexSwap(j);
						bRemoved = true;
						break;
					}
				}

				while (bRemoved)
				{
					bRemoved = false;
					auto& polyA = outPolyList[i];
					for (int j = 0; j < outPolyList.size(); ++j)
					{
						if (i == j)
							continue;

						auto& polyB = outPolyList[j];
						if (polyA.surfaceId != polyB.surfaceId)
							continue;

#if 1
						DEBUG_INC_INDEX();
						if (DEBUG_CHECK_STOP())
						{
							DEBUG_SHOW(polyA, Color3f(1, 1, 1));
							DEBUG_SHOW(polyB, Color3f(0, 0, 0));
							DEBUG_YEILD();
						}
#endif

						if (!TryMerge(polyA, polyB))
							continue;

						if (j < i)
						{
							if (j != i - 1)
							{
								outPolyList[j] = std::move(outPolyList[i - 1]);
							}
							outPolyList[i - 1] = std::move(outPolyList[i]);
							outPolyList.removeIndexSwap(i);
							--i;
						}
						else
						{
							outPolyList.removeIndexSwap(j);
						}
						bRemoved = true;
						break;
					}
				}
			}
		}
	};

	class TestStage : public TestRenderStageBase
	{
		using BaseClass = TestRenderStageBase;
	public:
		TestStage() {}

		bool onInit() override
		{
			if (!BaseClass::onInit())
				return false;
			::Global::GUI().cleanupWidget();

			FindSubString("abacdbabaabacababa", "abacababa");

			auto frame = WidgetUtility::CreateDevFrame();

			restart();
			return true;
		}

		Polygon mCubePlane;
		Polygon mCubePlane2;
		TArray<Polygon> mBox;
		TArray<Polygon> mCylinder;
		BSPTree mBoxBsp;
		BSPTree mBoxBspB;

		Polygon mTestPoly;
		TArray<Polygon> mClipPolyList;
		TArray<Polygon> mCone;
		TArray<Polygon> mSphere;

		TArray<Polygon> mOpResult;
		TArray<Polygon> mOpResult2;
		TArray<Polygon> mOpResult3;
		template< typename TOP >
		static void DoOp(TOP op, TArray<Polygon>& A, TArray<Polygon>& B, TArray<Polygon>& outResult)
		{
			BSPTree treeA;
			treeA.add(std::move(A));
			BSPTree treeB;
			treeB.add(std::move(B));
			op(treeA, treeB, outResult);
			Standardize(outResult);
		}

		static void Standardize(TArray<Polygon>& polyList)
		{
			for (auto& poly : polyList)
			{
				poly.standardize();
			}
		}

		void onEnd() override
		{
			BaseClass::onEnd();
		}

#if DEBUG_CSG
		Coroutines::ExecutionHandle mHandle;
#endif
		void restart()
		{
			Vector3 halfExtent = Vector3(5, 5, 5);
			auto& poly = mCubePlane;
			//FBuild::Plane(Transform::Identity(),Vector3(halfExtent.x, 0, 0), Vector3(0, halfExtent.y, 0), Vector3(0, 0, halfExtent.z), mCubePlane);
			FGeometry::Rect(Transform::Identity(), Vector3(-halfExtent.x, 0, 0), Vector3(0, halfExtent.y, 0), Vector3(0, 0, halfExtent.z), poly);
			BSPTree::SplitPolygon(Plane(Vector3(0, 0, 1), 0), mCubePlane, mCubePlane2);
			FGeometry::Box(Transform::Identity(), Vector3(5, 5, 5), mBox);
			auto box = mBox;
			mBoxBsp.add(std::move(box));

			TArray<Polygon> mBoxB;
			FGeometry::Box(Transform(Vector3(5,0,0)), Vector3(2, 20, 2), mBoxB);
			mBoxBspB.add(std::move(mBoxB));

			FGeometry::Cone(Transform(Vector3(0, 0, 0)), 5, 22, 13, true, mCone);
			{
				TIME_SCOPE("CSGOp");
				CSGOp::Difference(mBoxBsp, mBoxBspB, mOpResult);
				Standardize(mOpResult);
				CSGOp::Merge(mOpResult);				
				TArray<Polygon> temp = std::move(mOpResult);
				TArray<Polygon> cone = mCone;
				DoOp(CSGOp::Difference, temp, cone, mOpResult);
				CSGOp::Merge(mOpResult);
			}

			FGeometry::Cylinder(Transform(Vector3(20, 20, 0)), 5, 22, 8, true, mCylinder);

			TArray<Polygon> cylinder = mCylinder;
			TArray<Polygon> box2;
			FGeometry::Box(Transform(Vector3(20, 20, 0)), Vector3(10, 10, 10), box2);
			TArray<Polygon> cylinder2;
			FGeometry::Cylinder(Transform(Vector3(20, 20, 0), Quaternion::Rotate(Vector3(0,1,0), Math::PI /2)), 5, 22, 8, true, cylinder2);
			DoOp(CSGOp::Difference, box2, cylinder, mOpResult2);
			TArray<Polygon> temp = std::move(mOpResult2);
			DoOp(CSGOp::Difference, temp, cylinder2, mOpResult2);
#if DEBUG_CSG
			mHandle = Coroutines::Start([&]() {
#endif
				CSGOp::Merge(mOpResult2);
#if DEBUG_CSG
			});
#endif

			TArray<Polygon> box3;
			FGeometry::Box(Transform(Vector3(20, -20, 0)), Vector3(10, 10, 10), box3);
			FGeometry::IcoSphere(Transform(Vector3(10, -20, 0)), 10, 1, mSphere);
			TArray<Polygon> sphere = mSphere;
			DoOp(CSGOp::Difference, box3, sphere, mOpResult3);
			CSGOp::Merge(mOpResult3);

			{
				Vector3 center = Vector3(-20, -20, 0);
				//FGeometry::RegularPolygon(Transform(center), 5, 7, true, mTestPoly);

				float radius = 5;
				int sides = 7;
				bool bAlignToSide = true;
				Transform xform = Transform(center);

				float deltaTheta = 2 * Math::PI / float(sides);
				float theta = bAlignToSide ? -0.5f * deltaTheta : 0.0f;

				FGeometry::Rect(xform, Vector3::Zero(), Vector3(10,0,0), Vector3(0,10,0), mTestPoly);
				for (int i = 0; i < 2; ++i)
				{
					if (i == 1)
					{
						int aa = 1;
					}
					float c, s;
					Math::SinCos(theta, s, c);
					float c1, s1;
					Math::SinCos(theta + deltaTheta, s1, c1);

					Vector3 p0 = xform.transformPos(Vector3(radius * c, radius * s, 0));
					Vector3 p1 = xform.transformPos(Vector3(radius * c1, radius * s1, 0));
					Vector3 dir = p1 - p0;
					Polygon poly;
					BSPTree::SplitPolygon(Plane(Vector3(-dir.y, dir.x, 0), 0.5 * ( p1 + p0 )), mTestPoly, poly);
					mClipPolyList.push_back(std::move(poly));
					theta += deltaTheta;
				}
			}
		}

		void onUpdate(GameTimeSpan deltaTime) override
		{
			BaseClass::onUpdate(deltaTime);
		}

		static void Draw(RHICommandList& commandList, Polygon const& poly, LinearColor const& color)
		{
			TRenderRT< RTVF_XYZ >::Draw(commandList, EPrimitive::Polygon, poly.vertices.data(), poly.vertices.size(), color);
		}
		
		static void Draw(RHICommandList& commandList, Polygon const& poly)
		{
			static int ColorMap[] = { EColor::Red , EColor::Green , EColor::Blue , EColor::Orange , EColor::Yellow , EColor::Pink, EColor::Brown , EColor::Gold, EColor::Cyan };
			Draw(commandList, poly, Color3f(RenderUtility::GetColor(ColorMap[indexColor % ARRAY_SIZE(ColorMap)])));
			++indexColor;
		}

		static void Draw(RHICommandList& commandList, TArray<Polygon> const& polyList)
		{
			indexColor = 0;
			for (auto const& poly : polyList)
			{
				Draw(commandList, poly);
			}
		}
		static void Draw(RHICommandList& commandList, TArray<Polygon> const& polyList, LinearColor const& color)
		{
			for (auto const& poly : polyList)
			{
				Draw(commandList, poly, color);
			}
		}

		static void Draw(RHICommandList& commandList, BSPTree const& tree, LinearColor const& color)
		{
			indexColor = 0;
			for (auto const& node : tree.mNodes)
			{
				Draw(commandList, node.polygons, color);
			}
		}

		static void Draw(RHICommandList& commandList, BSPTree const& tree)
		{
			for (auto const& node : tree.mNodes)
			{
				Draw(commandList, node.polygons);
			}
		}

		void onRender(float dFrame) override
		{

			indexColor = 0;

			Vec2i screenSize = ::Global::GetScreenSize();
			RHICommandList& commandList = RHICommandList::GetImmediateList();
			initializeRenderState();

			RHISetFixedShaderPipelineState(commandList, AdjustProjectionMatrixForRHI(mView.worldToClip));
			DrawUtility::AixsLine(commandList, 10);
#if 0
			Draw(commandList, mCubePlane, LinearColor(1, 0, 0, 1));
			Draw(commandList, mCubePlane2, LinearColor(0, 1, 0, 1));
			Draw(commandList, mBox);

			Draw(commandList, mBoxBsp, LinearColor(1, 1, 0, 1));
			Draw(commandList, mBoxBspB, LinearColor(1, 0, 1, 1));
#endif
			Draw(commandList, mOpResult);

			Draw(commandList, mOpResult2);
			Draw(commandList, mTestPoly, LinearColor(1, 0, 0, 1));
			Draw(commandList, mClipPolyList);
			Draw(commandList, mOpResult3);
			//Draw(commandList, mCone);

			RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::None>::GetRHI());

#if DEBUG_CSG
			GDebuger.draw(commandList, mView);
#endif
			RHIFlushCommand(commandList);

			RHIGraphics2D& g = Global::GetRHIGraphics2D();
			g.beginRender();
#if DEBUG_CSG
			g.drawTextF( Vector2(10, 10), "Index = %d", GDebugIndex);
#endif

#if 0
			int N = 15;
			for (int i = 0; i < N; ++i)
			{
				for (int j = 0; j < N; ++j)
				{
					g.drawTextF(Vector2(10, 10) + 30 * Vector2(i, j), "%d", Math::SymmetricPairingFunction(i, j));
				}
			}
#endif
			g.endRender();

		}

		MsgReply onMouse(MouseMsg const& msg) override
		{
			return BaseClass::onMouse(msg);
		}


		MsgReply onKey(KeyMsg const& msg) override
		{
			if (msg.isDown())
			{
				switch (msg.getCode())
				{
				case EKeyCode::R: restart(); break;
#if DEBUG_CSG
				case EKeyCode::Q: GDebuger.clear(); Coroutines::Resume(mHandle); break;
#endif
				}
			}
			return BaseClass::onKey(msg);
		}

		bool onWidgetEvent(int event, int id, GWidget* ui) override
		{
			switch (id)
			{
			default:
				break;
			}

			return BaseClass::onWidgetEvent(event, id, ui);
		}
	protected:
	};



	REGISTER_STAGE_ENTRY("CSG Test", TestStage, EExecGroup::Dev, "Render");
}