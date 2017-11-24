#include "SpatialIndexTest.h"

REGISTER_STAGE("Spatial Index Test", SpatialIndexTestStage, EStageGroup::Test);

void KDTree::build()
{
	std::vector< int > dataIndices;
	dataIndices.resize(mDataVec.size());

	BoundBox2D bound;
	mBound = mDataVec[0].BBox;
	for( int i = 0; i < dataIndices.size(); ++i )
	{
		dataIndices[i] = i;
		mBound += mDataVec[i].BBox;
	}
	mRootId = buildNode_R(dataIndices, mBound);
}

bool KDTree::raycast(Ray const& ray, RayResult& result)
{
	float distanceRange[2];

	if( !Math2D::LineBoxTest(ray.pos, ray.dir, mBound.min, mBound.max, distanceRange) )
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

	Vector2 invRayDir = Vector2(1,1).div(ray.dir);

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

		if( nodeId < 0 )
		{
			bUseStack = true;

			if( nodeId != EMPTY_LEAF_ID )
			{
				Leaf const& leaf = mLeaves[-(nodeId + 1)];

				BoundBox2D rayBound;
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
			if ( ( ray.pos[axis] < node.value ) || 
				 (ray.pos[axis] == node.value && ray.dir[axis] <= 0 ) )  
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
	} 
	while( !bUseStack || !pathStack.empty() );

	return false;
}

int KDTree::makeLeaf(std::vector< int >& dataIndices)
{
	Leaf leaf;
	leaf.dataIndices = std::move(dataIndices);

	int indexLeaf = mLeaves.size();
	mLeaves.push_back(std::move(leaf));
	return -(indexLeaf + 1);
}

int KDTree::buildNode_R(std::vector< int >& dataIndices, BoundBox2D bound)
{
	if( dataIndices.size() < 3 )
	{
		return makeLeaf(dataIndices);
	}
	Vector2 size = bound.max - bound.min;
	int axis = (size.x >= size.y) ? 0 : 1;

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
	});

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

		for(  ; idxCur < numEdge; ++idxCur )
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
		BoundBox2D leftBound = bound;
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
		BoundBox2D rightBound = bound;
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

void SpatialIndexTestStage::onRender(float dFrame)
{
	Graphics2D& g = Global::getGraphics2D();

	RenderUtility::SetPen(g, Color::eNull);
	RenderUtility::SetBrush(g, Color::eGray);
	g.drawRect(Vec2i(0, 0), ::Global::getDrawEngine()->getScreenSize());

	static int const ColorMap[] =
	{
		Color::eBlue , Color::eGreen , Color::ePink ,
		Color::eOrange , Color::eRed  , Color::eYellow ,
		Color::ePurple
	};
	for( int i = 0; i < mTree.mDataVec.size(); ++i )
	{
		PrimitiveData& data = mTree.mDataVec[i];



		if( i == mRayResult.indexData )
		{
			RenderUtility::SetPen(g, Color::eBlack);
			RenderUtility::SetBrush(g, Color::eWhite);

		}
		else
		{
			RenderUtility::SetPen(g, Color::eWhite);
			RenderUtility::SetBrush(g, ColorMap[data.id % ARRAY_SIZE(ColorMap)]);
		}


		Vector2 rMin = convertToScreen(data.BBox.min);
		Vector2 rMax = convertToScreen(data.BBox.max);
		g.drawRect(rMin, rMax - rMin);
	}

	struct TreeDrawer
	{
		TreeDrawer(Graphics2D& g) :g(g) {}

		void operator()(KDTree::Node const& node, BoundBox2D bound)
		{
			RenderUtility::SetPen(g, Color::eRed);
			if( node.axis == 0 )
			{
				Vector2 p1 = convertToScreen(Vector2(node.value, bound.min.y));
				Vector2 p2 = convertToScreen(Vector2(node.value, bound.max.y));
				g.drawLine(p1, p2);
			}
			else
			{
				Vector2 p1 = convertToScreen(Vector2(bound.min.x, node.value));
				Vector2 p2 = convertToScreen(Vector2(bound.max.x, node.value));
				g.drawLine(p1, p2);
			}
		}
		void operator()(KDTree::Leaf const& node, BoundBox2D bound)
		{

		}
		Graphics2D& g;
	};

	TreeDrawer treeDrawer(g);
	mTree.visitNode(treeDrawer);



	float rayDist = 200;
	if( mRayResult.indexData == -1 )
	{
		RenderUtility::SetPen(g, Color::eOrange);


	}
	else
	{
		rayDist = mRayResult.dist;
		RenderUtility::SetPen(g, Color::ePink);
	}
	g.drawLine(convertToScreen(mTestRay.pos), convertToScreen(mTestRay.getPosition(rayDist)));
}
