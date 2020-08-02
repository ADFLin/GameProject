#include "SpatialIndexTest.h"

REGISTER_STAGE("Spatial Index Test", SpatialIndexTestStage, EStageGroup::Test);


void SpatialIndexTestStage::onRender(float dFrame)
{
	Graphics2D& g = Global::GetGraphics2D();

	RenderUtility::SetPen(g, EColor::Null);
	RenderUtility::SetBrush(g, EColor::Gray);
	g.drawRect(Vec2i(0, 0), ::Global::GetScreenSize());

	static int const ColorMap[] =
	{
		EColor::Blue , EColor::Green , EColor::Pink ,
		EColor::Orange , EColor::Red  , EColor::Yellow ,
		EColor::Purple
	};
	for( int i = 0; i < mTree.mDataVec.size(); ++i )
	{
		KDTree::PrimitiveData& data = mTree.mDataVec[i];

		if( i == mRayResult.indexData )
		{
			RenderUtility::SetPen(g, EColor::Black);
			RenderUtility::SetBrush(g, EColor::White);

		}
		else if( i == mIndexNearset )
		{
			RenderUtility::SetPen(g, EColor::Black);
			RenderUtility::SetBrush(g, EColor::Black);
		}
		else
		{
			RenderUtility::SetPen(g, EColor::White);
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
			RenderUtility::SetPen(g, EColor::Red);
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
	if( mRayResult.indexData == INDEX_NONE)
	{
		RenderUtility::SetPen(g, EColor::Orange);
	}
	else
	{
		rayDist = mRayResult.dist;
		RenderUtility::SetPen(g, EColor::Pink);
	}
	g.drawLine(convertToScreen(mTestRay.pos), convertToScreen(mTestRay.getPosition(rayDist)));
}
