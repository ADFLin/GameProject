#include "Bsp2D.h"
#include "Math/PrimitiveTest.h"

//#include "DebugDraw.h"

#define DEBUG_BSP2D 0
#include "Misc/DebugDraw.h"
#if DEBUG_BSP2D
#include "Async/Coroutines.h"
#include "Misc/DebugDraw.h"
#define DEBUG_BREAK() CO_YEILD(nullptr)
#define DEBUG_POINT(V, C) DrawDebugPoint(V, C, 5);
#define DEBUG_LINE(V1, V2, C) DrawDebugLine(V1, V2, C, 2);
#define DEBUG_TEXT(V, TEXT, C) DrawDebugText(V, TEXT, C);
#else
#define DEBUG_BREAK()
#define DEBUG_POINT(V, C)
#define DEBUG_LINE(V1, V2)
#define DEBUG_TEXT(V, TEXT, C)
#endif
#include "InlineString.h"


namespace Bsp2D
{

	void Plane::init( Vector2 const& v1 , Vector2 const& v2 )
	{
		*this = static_cast<Plane const&>(FromPosition(v1, v2));
	}

	Side Plane::split( Vector2 v[2] , Vector2 vSplit[2] ) const
	{
		EPlaneSide::Type s0 = testSide(v[0], WallThickness);
		float dist;
		EPlaneSide::Type s1 = testSide(v[1], WallThickness, dist);
		if ( s0 + s1 != 0 )
			return ( s0 ) ? Side( s0 ) : Side(s1);

		if ( s0 == SIDE_IN )
			return SIDE_IN;

		Vector2 dir = v[0] - v[1];

		//plane.normal( v[1] + dir * t ) + d = 0;
		float t = - dist / mNormal.dot( dir );
		Vector2 p = v[1] + t * dir;

		if ( s0 == SIDE_FRONT )
		{
			vSplit[0] = p;
			vSplit[1] = v[1];
			v[1] = p;
		}
		else
		{
			vSplit[0] = v[0];
			vSplit[1] = p;
			v[0] = p;
		}

		return SIDE_SPLIT;
	}

	int Plane::clip(bool bFront, Vector2 inoutV[2]) const
	{
		EPlaneSide::Type clipSide = bFront ? EPlaneSide::Front : EPlaneSide::Back;
		EPlaneSide::Type s0 = testSide(inoutV[0], WallThickness);
		float dist;
		EPlaneSide::Type s1 = testSide(inoutV[1], WallThickness, dist);
		if (s0 + s1 != 0)
		{
			return (((s0) ? s0 : s1) == clipSide) ? 1 : 0;
		}

		if (s0 == SIDE_IN)
			return false;

		Vector2 dir = inoutV[0] - inoutV[1];

		//plane.normal( v[1] + dir * t ) + d = 0;
		float t = -dist / mNormal.dot(dir);
		Vector2 p = inoutV[1] + t * dir;
		inoutV[s0 == clipSide] = p;

		return 2;
	}

	Side Plane::testSegment( Vector2 const& v0 , Vector2 const& v1 ) const
	{
		float dist;
		EPlaneSide::Type s0 = testSide( v0 , WallThickness, dist );
		EPlaneSide::Type s1 = testSide( v1 , WallThickness, dist );

		if ( s0 + s1 != 0 )
			return ( s0 ) ? Side( s0 ) : Side(s1);

		if ( s0 == EPlaneSide::In )
			return SIDE_IN;

		return SIDE_SPLIT;
	}

	void Tree::clear()
	{
		if ( mRoot )
		{
			for (auto node : mNodes)
			{
				delete node;
			}
			mNodes.clear();
			mRoot = nullptr;
		}
		mEdges.clear();
		mLeaves.clear();
	}

	void Tree::build( PolyArea* polys[] , int num , Vector2 const& bbMin , Vector2 const& bbMax )
	{
		clear();


		int numEdge = 0;
		for( int polyIndex = 0 ; polyIndex < num ; ++polyIndex )
		{
			PolyArea* poly = polys[polyIndex];

			numEdge += poly->getVertexNum();

			int prev = poly->getVertexNum() - 1;
			for( int cur = 0 ;cur < poly->getVertexNum() ; ++cur )
			{
				assert( IsInRect( poly->getVertex( prev ) , bbMin , bbMax ) );
				assert( IsInRect( poly->getVertex( cur ) , bbMin , bbMax ) );

				addEdge( poly->getVertex( prev ) , poly->getVertex( cur ) , polyIndex );
				prev = cur;
			}
		}


		Vector2 edgeBB[2] = { Vector2( bbMin.x , bbMax.y ) , Vector2( bbMax.x , bbMin.y ) };

		addEdge( bbMin , edgeBB[0] , -1 );
		addEdge( edgeBB[0] , bbMax , -1 );
		addEdge( bbMax , edgeBB[1] , -1 );
		addEdge( edgeBB[1] , bbMin , -1 );

		numEdge += 4;
		struct CountFunc
		{
			int current = 0;
			int operator()() { return current++; }
		};
		IndexList idxEdges( numEdge );
		std::generate_n( idxEdges.begin() , numEdge , CountFunc() );

		assert( mRoot == nullptr );

		mRoot = contructTree_R( idxEdges );
		mRoot->parent = nullptr;
	}

	Tree::Node* Tree::contructTree_R( IndexList& edgeIndices )
	{
		if ( edgeIndices.empty() )
			return nullptr;

		int idx = choiceBestSplitEdge( edgeIndices );

		Node* node = new Node;

		node->idxEdge   = idx;

		if ( idx < 0 ) // leaf
		{
			node->tag   = ~uint32( mLeaves.size() );
			node->front = nullptr;
			node->back  = nullptr;

			mLeaves.push_back( Leaf() );
			Leaf& data = mLeaves.back();
			data.node = node;
			data.edges = std::move( edgeIndices );
			return node;
		}
		else
		{
			node->tag = uint32( mNodes.size() );
			mNodes.push_back( node );
		}

		//triList.erase( cIter );

		IndexList idxFronts;
		IndexList idxBacks;

		Plane& plane = mEdges[ idx ].plane;

		int countFront = 0;
		int countBack = 0;
		int countSplitted = 0;
		int countOnPlane = 0;

		for(int indexEdge : edgeIndices)
		{
			Edge& edgeTest = mEdges[ indexEdge ];

			Vector2 vSplit[2];
			switch ( plane.split( edgeTest.v , vSplit ) )
			{
			case SIDE_FRONT:
				++countFront;
				idxFronts.push_back(indexEdge);
				break;
			case SIDE_IN:
				++countOnPlane;			
#if 0
				idxFronts.push_back(indexEdge);
#else
				if ( Math::Dot(plane.getNormal(), edgeTest.plane.getNormal()) > 0 )
					idxFronts.push_back(indexEdge);
				else
					idxBacks.push_back(indexEdge);
#endif
				break;
			case SIDE_BACK:
				++countBack;
				idxBacks.push_back(indexEdge);
				break;
			case SIDE_SPLIT:
				{
					++countSplitted;
					idxFronts.push_back( indexEdge );
					idxBacks.push_back( (int)mEdges.size() );
					mEdges.push_back( Edge() );
					Edge& edge = mEdges.back();
					edge.v[0] = vSplit[0];
					edge.v[1] = vSplit[1];
					edge.plane = edgeTest.plane;
					edge.polyIndex = edgeTest.polyIndex;
				}
				break;
			}				
		}

		node->front = contructTree_R( idxFronts );
		if ( node->front )
			node->front->parent = node;

		node->back  = contructTree_R( idxBacks );
		if ( node->back )
			node->back->parent = node;

		//node->tag   = 0;

		return node;
	}

	int Tree::choiceBestSplitEdge( IndexList const& edgeIndices )
	{
		int minVal = INT_MAX;
		int indexBest = INDEX_NONE;

		for( int indexCur : edgeIndices )
		{
			int countFront = 0;
			int countBack = 0;
			int countSplitted = 0;
			int countOnPlane = 0;

			Edge& edge = mEdges[indexCur];
			Plane& plane = edge.plane;

			for( int indexTest : edgeIndices )
			{
				Edge& edgeTest = mEdges[indexTest];
				switch( plane.testSegment( edgeTest.v ) )
				{
				case SIDE_FRONT: ++countFront; break;
				case SIDE_BACK:  ++countBack;  break;
				case SIDE_SPLIT: ++countSplitted; break;
				case SIDE_IN:    ++countOnPlane; break;
				}
			}

			auto CanUse = [&]()
			{
				if (countBack != 0)
					return true;

				if ((countSplitted == 0) && (countBack == 0 || countFront == 0))
					return false;

				return true;
			};
			if (!CanUse())
				continue;

			int score = abs( countFront - countBack ) + 2 * countSplitted;
			if ( score < minVal )
			{
				indexBest = indexCur;
				minVal = score;
			}
		}
		return indexBest;
	}



	class Tree::SegmentColSolver
	{
	public:
		bool solve( Tree& tree , Vector2 const& start , Vector2 const& end , ColInfo& info )
		{
			if ( !tree.getRoot() )
				return false;

			mTree  = &tree;
			mInfo  = &info;
			mStart = start;
			mEnd   = end;
			return solve_R( mTree->getRoot() );
		}

	private:

		bool solve_R( Node* node )
		{
			assert( node );

			if ( node->isLeaf() )
			{
				Leaf& leaf = mTree->getLeaf( node );
				for( int i = 0 ; i < leaf.edges.size(); ++i )
				{
					int idxEdge = leaf.edges[i];
					Edge& edge = mTree->getEdge( idxEdge );


					float frac[2];
					if ( Math::SegmentSegmentTest(mStart, mEnd, edge.v[0], edge.v[1], frac) )
					{
						if (WallThickness <= frac[1] && frac[1] <= 1 - WallThickness)
						{
							mInfo->indexEdge = idxEdge;
							mInfo->frac = frac[2];
							return true;
						}
					}

				}
				return false;
			}

			Edge& edge = mTree->getEdge( node->idxEdge );

			float dist;
			EPlaneSide::Type s0 = edge.plane.testSide(mStart, WallThickness, dist);
			EPlaneSide::Type s1 = edge.plane.testSide(mEnd, WallThickness, dist);

			if ( s0 == EPlaneSide::In )
			{
				if ( solve_R( node->front ) )
					return true;
				if ( s1 == EPlaneSide::Back )
					return solve_R( node->back );
			}
			else if ( s0 == s1 )
			{
				if ( s0 == EPlaneSide::Front )
				{
					return solve_R( node->front );
				}
				else
				{
					return solve_R( node->back  );
				}
			}
			else
			{
				if ( s0 == EPlaneSide::Front )
				{
					if ( solve_R( node->front ) )
						return true;
					if ( solve_R( node->back ) )
						return true;
				}
				else
				{
					if ( solve_R( node->back ) )
						return true;
					if ( solve_R( node->front ) )
						return true;
				}
			}

			return false;
		}

		Tree::ColInfo* mInfo;
		Tree*  mTree;
		Vector2  mStart;
		Vector2  mEnd;
	};

	bool Tree::segmentTest( Vector2 const& start , Vector2 const& end , ColInfo& info )
	{
		SegmentColSolver solver;
		return solver.solve( *this , start , end , info );
	}

	class Tree::LeafFinder
	{
	public:
		Node* find(Tree& tree, Vector2 const& pos)
		{
			if (!tree.getRoot())
				return nullptr;

			mTree = &tree;
			mPos = pos;
			return find_R(mTree->getRoot());
		}

		Node* find_R(Node* node)
		{
			assert(node);

			while (node)
			{
				if (node->isLeaf())
				{
					break;
				}

				Edge& edge = mTree->getEdge(node->idxEdge);
				float dist;
				EPlaneSide::Type side = edge.plane.testSide(mPos, WallThickness, dist);
				switch (side)
				{
				case EPlaneSide::Front:
					node = node->front;
					break;
				case EPlaneSide::Back:
					node = node->back;
					break;
				case EPlaneSide::In:
					{
						if (dist >= 0)
							node = node->front;
						else
							node = node->back;
					}
					break;
				}
			}

			return node;
		}

		Tree*   mTree;
		Vector2 mPos;
	};

	Tree::Node* Tree::getLeaf(Vector2 const& pos)
	{
		LeafFinder finder;
		return finder.find(*this, pos);
	}


	static bool IsContain(Vector2 const a[], Vector2 const b[])
	{
		Vector2 dirA = a[1] - a[0];
#if 0
		Vector2 dirB = b[1] - b[0];
		if ( Math::Abs(Math::Dot(dirA, dirB)) < 1 - FLOAT_DIV_ZERO_EPSILON)
			return false;
#endif

		float length = dirA.length();
		dirA /= length;

		Vector2 dir0 = b[0] - a[0];
		if ( Math::Abs(dirA.cross(dir0)) > FLOAT_DIV_ZERO_EPSILON)
			return false;
		float d0 = Math::Dot(dirA , dir0);
		if (d0 < -WallThickness || d0 > length + WallThickness)
			return false;

		Vector2 dir1 = b[1] - a[0];
		if ( Math::Abs(dirA.cross(dir1)) > FLOAT_DIV_ZERO_EPSILON)
			return false;
		float d1 = Math::Dot(dirA, dir1);
		if (d1 < -WallThickness || d1 > length + WallThickness)
			return false;

		return true;
	}
	void PortalBuilder::build(Vector2 const& min, Vector2 const& max)
	{
		cleanup();

		int numNodes = mTree->mNodes.size();
		for( int i = 0 ; i < numNodes ; ++i )
		{
			Node* node = mTree->mNodes[i];

			if ( node->isLeaf() )
				continue;

			SuperPlane* splane = createSuperPlane(node, min, max);
			if (splane)
			{
				generatePortal_R(node->front, splane);
				generatePortal_R(node->back, splane);


				Edge edgeTemp;
				for (int indexPortal = 0; indexPortal < splane->portals.size(); ++indexPortal)
				{
					auto& portal = splane->portals[indexPortal];
					Leaf& leafFront = mTree->getLeafByTag(portal.frontTag);
					for (int i = 0; i < leafFront.edges.size(); ++i)
					{
						Edge const& edge = mTree->getEdge(leafFront.edges[i]);
						if (IsContain(edge.v, portal.v))
						{
							edgeTemp = edge;
							goto Remove;
						}
					}
					Leaf& leafBack = mTree->getLeafByTag(portal.backTag);
					for (int i = 0; i < leafBack.edges.size(); ++i)
					{
						Edge const& edge = mTree->getEdge(leafBack.edges[i]);
						if (IsContain(edge.v, portal.v))
						{
							edgeTemp = edge;
							goto Remove;
						}
					}

					continue;

				Remove:
#if 0
					static int Count = 0;
					++Count;
					if ( Count == 1 )
					{
						DrawDebugLine(portal.v[0], portal.v[1], Color3f(0.5, 0.5, 1), 2);
						DrawDebugPoint(0.5 * (portal.v[0] + portal.v[1]), Color3f(1, 0.5, 0.5), 4);
					}
#endif
					splane->portals.removeIndexSwap(indexPortal);
					--indexPortal;
				}
			}
		}
	}

	PortalBuilder::SuperPlane* PortalBuilder::createSuperPlane( Node* node , Vector2 const& min , Vector2 const& max )
	{
		CHECK( node && !node->isLeaf() );

		auto const& plane = mTree->getEdge(node->idxEdge).plane;
		Vector2 dir = plane.getDirection();
		Vector2 pos = plane.getAnyPoint();

		float distances[2];
		if (!Math::LineAABBTest(pos, dir, min, max, distances))
		{
			return nullptr;
		}

		Portal portal;

		portal.v[0] = pos + distances[0] * dir;
		portal.v[1] = pos + distances[1] * dir;

		Node* cur = node;
		Node* parent = node->parent;
		while (parent)
		{
			auto const& curPlane = mTree->getEdge(parent->idxEdge).plane;
			bool bFront = parent->front == cur;
			if (!curPlane.clip(bFront, portal.v))
				return nullptr;


			cur = parent;
			parent = cur->parent;
		}


		portal.frontTag = node->front->tag;
		portal.backTag = node->back->tag;

		SuperPlane* splane = new SuperPlane;
		splane->node  = node;
		splane->plane = mTree->getEdge( node->idxEdge ).plane;
		splane->tag   = mSPlanes.size();
		splane->portals.push_back(portal);
		splane->debugPos[0] = portal.v[0];
		splane->debugPos[1] = portal.v[1];
		mSPlanes.push_back(splane);
		return splane;
	}

	bool SegmentSegmentTest(Vector2 const& posA1, Vector2 const& posA2, Vector2 const& posB1, Vector2 const& posB2, float error)
	{
		Vector2 dirA = posA2 - posA1;
		Vector2 dirB = posB2 - posB1;

		float det = dirA.y * dirB.x - dirA.x * dirB.y;
		if (Math::Abs(det) < FLOAT_DIV_ZERO_EPSILON)
			return false;

		Vector2 d = posB1 - posA1;
		float tA = (d.y * dirB.x - d.x * dirB.y) / det;
		if (tA > 1 + error || tA < 0 - error)
			return false;
		float tB = (d.y * dirA.x - d.x * dirA.y) / det;
		if (tB > 1 + error || tB < 0 - error)
			return false;

		return true;
	}

	void PortalBuilder::generatePortal_R(Node* node, SuperPlane* splane)
	{
		auto const& clipPlane = mTree->getEdge(node->idxEdge).plane;
#if DEBUG_BSP2D

		if (node->isLeaf())
		{
			Leaf& leaf = mTree->getLeaf(node);
			for (int indexEdge = 0; indexEdge < leaf.edges.size(); ++indexEdge)
			{
				Edge const& edge = mTree->getEdge(leaf.edges[indexEdge]);
				Vector2 mid = 0.5 * (edge.v[0] + edge.v[1]);
				DEBUG_POINT(mid, Color3f(1, 0, 1));
			}
		}
		else
		{

			DEBUG_LINE(clipPlane.getAnyPoint() - 100 * clipPlane.getDirection(), clipPlane.getAnyPoint() + 100 * clipPlane.getDirection(), Color3f(1, 0, 0));
			DEBUG_LINE(clipPlane.getAnyPoint(), clipPlane.getAnyPoint() + clipPlane.getNormal(), Color3f(1, 0, 0));
		}

		DEBUG_LINE(splane->debugPos[0], splane->debugPos[1], Color3f(0, 1, 1));
		Vector2 debugPos = 0.5 * (splane->debugPos[0] + splane->debugPos[1]);
		DEBUG_LINE(debugPos, debugPos + splane->plane.getNormal(), Color3f(0, 1, 1));
		for (int indexPortal = 0; indexPortal < splane->portals.size(); ++indexPortal)
		{
			auto& portal = splane->portals[indexPortal];
			Vector2 mid = 0.5 * (portal.v[0] + portal.v[1]);
			InlineString<> str;
			if (Bsp2D::Tree::IsLeafTag(portal.frontTag))
			{
				if (Bsp2D::Tree::IsLeafTag(portal.backTag))
				{
					str.format("L[%u], L[%u]", ~portal.frontTag, ~portal.backTag);
				}
				else
				{
					str.format("L[%u], %u", ~portal.frontTag, portal.backTag);
				}
			}
			else if (Bsp2D::Tree::IsLeafTag(portal.backTag))
			{
				str.format("%u, L[%u]", portal.frontTag, ~portal.backTag);
			}
			else
			{
				str.format("%u, %u", portal.frontTag, portal.backTag);
			}

			DEBUG_TEXT(mid, str, Color3f(0, 1, 0));
		}
		DEBUG_BREAK();
#endif


		if (node->isLeaf())
		{
			Leaf& leaf = mTree->getLeaf(node);

			for (int indexPortal = 0; indexPortal < splane->portals.size(); ++indexPortal)
			{
				auto& portal = splane->portals[indexPortal];

				uint32* tagPtr;
				if (portal.frontTag == node->tag)
					tagPtr = &portal.frontTag;
				else if (portal.backTag == node->tag)
					tagPtr = &portal.backTag;
				else
				{
					continue;
				}

				for (int indexEdge = 0; indexEdge < leaf.edges.size(); ++indexEdge)
				{
					Edge const& edge = mTree->getEdge(leaf.edges[indexEdge]);

					DEBUG_LINE(edge.v[0], edge.v[1], Color3f(1, 0, 1));
					DEBUG_LINE(portal.v[0], portal.v[1], Color3f(0, 1, 1));
					DEBUG_BREAK();

					if ( SegmentSegmentTest(portal.v[0], portal.v[1], edge.v[0], edge.v[1], WallThickness) )
					{
						float dot = Math::Dot(edge.plane.getNormal(), splane->plane.getNormal());
						if (Math::Abs(1 - dot) < FLOAT_EPSILON)
						{
							continue;
						}

						Vector2 vSplitted[2];
						int clipStatus = edge.plane.clip(true, portal.v);
						if (!clipStatus)
						{
							splane->portals.removeIndexSwap(indexPortal);
							--indexPortal;
							if (indexPortal < 0)
								break;
						}

#if DEBUG_BSP2D
						if (clipStatus == 2)
						{
							DEBUG_LINE(edge.v[0], edge.v[1], Color3f(1, 0, 1));
							DEBUG_LINE(portal.v[0], portal.v[1], Color3f(0, 1, 1));
							DEBUG_BREAK();
						}
#endif
					}
					else
					{
						auto side = edge.plane.testSegment(portal.v);
						if (side == SIDE_BACK)
						{
							splane->portals.removeIndexSwap(indexPortal);
							--indexPortal;
							if (indexPortal < 0)
								break;
						}
					}
				}
			}
		}
		else
		{

			int numPortal = splane->portals.size();
			for (int indexPortal = 0; indexPortal < numPortal; ++indexPortal)
			{
				auto& portal = splane->portals[indexPortal];

				uint32* tagPtr;
				if (portal.frontTag == node->tag)
					tagPtr = &portal.frontTag;
				else if (portal.backTag == node->tag)
					tagPtr = &portal.backTag;
				else
				{
					continue;
				}

				Vector2 vSplitted[2];
				switch (clipPlane.split(portal.v, vSplitted))
				{
				case SIDE_FRONT:
					*tagPtr = node->front->tag;
					break;
				case SIDE_BACK:
					*tagPtr = node->back->tag;
					break;
				case SIDE_SPLIT:
					{
						Portal portalSplitted;
						auto frontTag = node->front->tag;
						auto backTag = node->back->tag;
						if (*tagPtr == portal.frontTag)
						{
							portal.frontTag = frontTag;
							portalSplitted.frontTag = backTag;
							portalSplitted.backTag = portal.backTag;
						}
						else
						{
							portal.backTag = frontTag;
							portalSplitted.backTag = backTag;
							portalSplitted.frontTag = portal.frontTag;
						}
						portalSplitted.v[0] = vSplitted[0];
						portalSplitted.v[1] = vSplitted[1];
						splane->portals.push_back(portalSplitted);
					}
					break;
				case SIDE_IN:
					if (Math::Dot(splane->plane.getNormal(), clipPlane.getNormal()) > 0)
					{
						*tagPtr = node->front->tag;
					}
					else
					{
						*tagPtr = node->back->tag;
					}
					break;
				} 
			}

			generatePortal_R(node->front, splane);
			generatePortal_R(node->back, splane);
		}
	}

}//namespace Bsp2D