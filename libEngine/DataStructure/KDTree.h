#pragma once
#ifndef KDTree_H_2B427EF1_6D32_40F3_8DCF_E3D9B30FCC2B
#define KDTree_H_2B427EF1_6D32_40F3_8DCF_E3D9B30FCC2B

#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/GeometryPrimitive.h"

template< int >
struct DimTraits {};

template<>
struct DimTraits<2>
{
	typedef Math::Vector2 VectorType;
	
};
template<>
struct DimTraits<3>
{
	typedef Math::Vector3 VectorType;
};


template< int Dim >
class TKDTree
{
public:
	static int const EMPTY_LEAF_ID = std::numeric_limits<int>::min();
	typedef typename DimTraits< Dim >::VectorType VectorType;
	typedef Math::TBoundBox< VectorType > BoundType;
	typedef Math::TRay< VectorType > RayType;

	TKDTree()
	{
		mRootId = EMPTY_LEAF_ID;
	}

	struct Node
	{
		int   axis;
		float value;
		int   idxLeft;
		int   idxRight;
	};

	struct Leaf
	{
		std::vector<int> dataIndices;
	};

	struct Edge
	{
		float value;
		bool  bStart;
		int   indexData;
	};

	struct PrimitiveData
	{
		int id;
		BoundType BBox;
	};

	BoundType mBound;

	void build()
	{
		std::vector< int > dataIndices;
		dataIndices.resize(mDataVec.size());

		BoundType bound;
		mBound = mDataVec[0].BBox;
		for( int i = 0; i < dataIndices.size(); ++i )
		{
			dataIndices[i] = i;
			mBound += mDataVec[i].BBox;
		}
		mRootId = buildNode_R(dataIndices, mBound);
	}

	struct RayResult
	{
		int   indexData;
		float dist;
	};

	bool raycast(RayType const& ray, RayResult& result)
	{
		float distanceRange[2];

		if( !Math::LineAABBTest(ray.pos, ray.dir, mBound.min, mBound.max, distanceRange) )
			return false;

		if( distanceRange[0] < 0 )
		{
			if( distanceRange[1] < 0 )
				return false;
			distanceRange[0] = 0;
		}

		struct PathData
		{
			int   nodeId;
			float range[2];
		};

		std::vector< PathData > pathStack;

		VectorType invRayDir = Vector2(1, 1).div(ray.dir);

		PathData curPath;
		curPath.nodeId = mRootId;
		curPath.range[0] = distanceRange[0];
		curPath.range[1] = distanceRange[1];

		bool bUseStack = false;
		do
		{
			if( bUseStack )
			{
				curPath = pathStack.back();
				pathStack.pop_back();
				bUseStack = false;
			}

			int nodeId = curPath.nodeId;

			if( IsLeaf(nodeId) )
			{
				bUseStack = true;

				if( !IsEmptyLeaf(nodeId) )
				{
					Leaf const& leaf = getLeaf(nodeId);

					BoundType rayBound;
					rayBound.min = rayBound.max = ray.getPosition(curPath.range[0]);
					rayBound += ray.getPosition(curPath.range[1]);

					float minDist = Math::MaxFloat;
					int   indexDataMin = -1;
					for( int i = 0; i < leaf.dataIndices.size(); ++i )
					{
						int idxData = leaf.dataIndices[i];
						PrimitiveData const& data = mDataVec[idxData];

						if( !rayBound.isIntersect(data.BBox) )
							continue;

						float outDists[2];
						if( ray.testIntersect(data.BBox, outDists) )
						{
							if( outDists[0] < minDist )
							{
								if( outDists[0] >= curPath.range[0] && outDists[0] <= curPath.range[1] )
								{
									indexDataMin = idxData;
									minDist = outDists[0];
								}
							}
						}
					}

					if( indexDataMin != -1 )
					{
						result.dist = minDist;
						result.indexData = indexDataMin;
						return true;
					}
				}
			}
			else
			{
				Node const& node = mNodes[nodeId];

				int const axis = node.axis;

				float dist = (node.value - ray.pos[axis]) * invRayDir[axis];

				int nextNodeId[2];
				if( (ray.pos[axis] < node.value) ||
					(ray.pos[axis] == node.value && ray.dir[axis] <= 0) )
				{
					nextNodeId[0] = node.idxLeft;
					nextNodeId[1] = node.idxRight;
				}
				else
				{
					nextNodeId[0] = node.idxRight;
					nextNodeId[1] = node.idxLeft;
				}


				if( dist > curPath.range[1] || dist <= 0 )
				{
					curPath.nodeId = nextNodeId[0];
				}

				else if( dist < curPath.range[0] )
				{
					curPath.nodeId = nextNodeId[1];
				}
				else
				{

					PathData path;
					path.nodeId = nextNodeId[1];
					path.range[0] = dist;
					path.range[1] = curPath.range[1];
					pathStack.push_back(path);

					curPath.nodeId = nextNodeId[0];
					curPath.range[1] = dist;
				}
			}
		} while( !bUseStack || !pathStack.empty() );

		return false;
	}

	int  makeLeaf(std::vector< int >& dataIndices)
	{
		Leaf leaf;
		leaf.dataIndices = std::move(dataIndices);

		int indexLeaf = mLeaves.size();
		mLeaves.push_back(std::move(leaf));
		return -(indexLeaf + 1);
	}
	int choiceAxis(Math::Vector2 const& size)
	{
		return (size.x >= size.y) ? 0 : 1;
	}
	int choiceAxis(Math::Vector3 const& size)
	{
		if ( size.x > size.y )
			return (size.x >= size.z) ? 0 : 2;
		return (size.y >= size.z) ? 1 : 2;
	}
	int  buildNode_R(std::vector< int >& dataIndices, BoundType const& bound)
	{
		if( dataIndices.size() < 3 )
		{
			return makeLeaf(dataIndices);
		}
		VectorType size = bound.max - bound.min;
		int axis = choiceAxis(size);

		std::vector< Edge > edges;
		edges.reserve(2 * dataIndices.size());
		for( int i = 0; i < dataIndices.size(); ++i )
		{
			int index = dataIndices[i];
			PrimitiveData const& data = mDataVec[index];

			Edge edge;
			edge.indexData = index;

			edge.value = data.BBox.min[axis];
			edge.bStart = true;
			edges.push_back(edge);
			edge.value = data.BBox.max[axis];
			edge.bStart = false;
			edges.push_back(edge);
		}


		std::sort(edges.begin(), edges.end(),
			[](Edge const& lhs, Edge const& rhs)->bool
			{
				return lhs.value < rhs.value;
			}
		);

		int numEdge = dataIndices.size() * 2;
		int numShared = 0;
		int numLeft = 0;

		int minScore = std::numeric_limits<int>::max();
		int indexBestChoice = -1;
		int numBestShared = 0;
		for( int i = 0; i < numEdge; ++i )
		{
			Edge const& edge = edges[i];
			if( edge.bStart )
			{
				++numShared;
			}
			else
			{
				--numShared;
				++numLeft;
			}

			if( edge.value <= bound.min[axis] )
				continue;

			if( edge.value >= bound.max[axis] )
				break;

			int const numRight = dataIndices.size() - numShared - numLeft;
			int score = std::abs(numLeft - numRight) + 5 * numShared;

			if( score < minScore )
			{
				minScore = score;
				indexBestChoice = i;
				numBestShared = numShared;
			}
		}

		if( indexBestChoice == -1 || numShared == dataIndices.size() )
		{
			return makeLeaf(dataIndices);
		}

		std::vector< int > leftIndices;
		std::vector< int > rightIndices;
		{
			int idxCur = 0;
			for( ; idxCur < indexBestChoice; ++idxCur )
			{
				if( edges[idxCur].bStart )
				{
					leftIndices.push_back(edges[idxCur].indexData);
				}
			}

			++idxCur;

			for( ; idxCur < numEdge; ++idxCur )
			{
				if( !edges[idxCur].bStart )
				{
					rightIndices.push_back(edges[idxCur].indexData);
				}
			}
		}

		float splitValue = edges[indexBestChoice].value;
		int idxNode = mNodes.size();
		mNodes.push_back(Node());

		int idxLeftChild;
		if( !leftIndices.empty() )
		{
			BoundType leftBound = bound;
			leftBound.max[axis] = splitValue;
			idxLeftChild = buildNode_R(leftIndices, leftBound);
		}
		else
		{
			idxLeftChild = EMPTY_LEAF_ID;
		}

		int idxRightChild;
		if( !rightIndices.empty() )
		{
			BoundType rightBound = bound;
			rightBound.min[axis] = splitValue;
			idxRightChild = buildNode_R(rightIndices, rightBound);
		}
		else
		{
			idxRightChild = EMPTY_LEAF_ID;
		}

		Node& node = mNodes[idxNode];
		node.idxLeft = idxLeftChild;
		node.idxRight = idxRightChild;
		node.axis = axis;
		node.value = splitValue;

		return idxNode;
	}


	static bool IsLeaf(int nodeId) { return nodeId < 0; }
	static bool IsEmptyLeaf(int nodeId) { return nodeId == EMPTY_LEAF_ID; }

	template< class DistFun >
	int  findNearst(VectorType const& pos, DistFun& distFun , float& outDistSqr )
	{
		struct StackData
		{
			int nodeId;
			BoundType bound;

		};
		std::vector< StackData > stack;
		stack.push_back({ mRootId , mBound });

		float minDistSqr = Math::MaxFloat;
		int   indexMinDist = -1;

		while( !stack.empty() )
		{
			StackData stackData = stack.back();
			stack.pop_back();

			float distSqrNode = Math::DistanceSqure( stackData.bound , pos);
			if( distSqrNode > minDistSqr )
				continue;

			if( IsLeaf(stackData.nodeId) )
			{
				if( !IsEmptyLeaf(stackData.nodeId) )
				{
					Leaf const& leaf = getLeaf(stackData.nodeId);

					for( auto index : leaf.dataIndices )
					{
						auto& data = mDataVec[index];
						float distSqr = distFun(data, pos, minDistSqr);

						if( minDistSqr > distSqr )
						{
							indexMinDist = index;
							minDistSqr = distSqr;
						}
					}
				}
			}
			else
			{
				Node const& node = mNodes[stackData.nodeId];
				BoundType childBoundLeft = stackData.bound;
				childBoundLeft.max[node.axis] = node.value;
				BoundType childBoundRight = stackData.bound;
				childBoundRight.min[node.axis] = node.value;
				float dist = pos[node.axis] - node.value;
				if(  dist >= 0 )
				{
					if ( dist * dist < minDistSqr )
						stack.push_back({ node.idxLeft, childBoundLeft });

					stack.push_back({ node.idxRight, childBoundRight });
				}
				else
				{
					if( dist * dist < minDistSqr )
						stack.push_back({ node.idxRight, childBoundRight });

					stack.push_back({ node.idxLeft, childBoundLeft });
				}
			}
		}

		outDistSqr = minDistSqr;
		return indexMinDist;
	}

	template< class TFunc >
	struct Visitor
	{
		Visitor(TFunc& inFunc, TKDTree& inTree)
			:func(inFunc), tree(inTree)
		{

		}

		void visit(int nodeId, BoundType bound)
		{
			if (IsLeaf(nodeId))
			{
				if (!IsEmptyLeaf(nodeId))
				{
					Leaf const& leaf = tree.mLeaves[-(nodeId + 1)];
					func(leaf, bound);
				}
				return;
			}

			Node const& node = tree.mNodes[nodeId];

			func(node, bound);

			{
				BoundType childBound = bound;
				childBound.max[node.axis] = node.value;
				visit(node.idxLeft, childBound);
			}

			{
				BoundType childBound = bound;
				childBound.min[node.axis] = node.value;
				visit(node.idxRight, childBound);
			}

		}

		TFunc&   func;
		TKDTree& tree;
	};

	template< class TFunc >
	void visitNode(TFunc& visitFunc) const
	{
		visitNode_R(mRootId, visitFunc, mBound);
	}

	template< class TFunc >
	void visitNode_R(int nodeId, TFunc& visitFunc, BoundType bound) const
	{
		if( IsLeaf(nodeId) )
		{
			if( !IsEmptyLeaf(nodeId) )
			{
				Leaf const& leaf = getLeaf(nodeId);
				visitFunc(leaf, bound);
			}
			return;
		}

		Node const& node = mNodes[nodeId];

		visitFunc(node, bound);

		{
			BoundType childBound = bound;
			childBound.max[node.axis] = node.value;
			visitNode_R(node.idxLeft, visitFunc, childBound);
		}

		{
			BoundType childBound = bound;
			childBound.min[node.axis] = node.value;
			visitNode_R(node.idxRight, visitFunc, childBound);
		}
	}

	Leaf const& getLeaf(int nodeId) const
	{
		assert(IsLeaf(nodeId) && !IsEmptyLeaf(nodeId));
		return mLeaves[-(nodeId + 1)];
	}

	int mRootId;


	std::vector< Node > mNodes;
	std::vector< Leaf > mLeaves;
	std::vector< PrimitiveData > mDataVec;
};

#endif // KDTree_H_2B427EF1_6D32_40F3_8DCF_E3D9B30FCC2B