#include "AStarStage.h"

namespace AStar
{
	uint8 MapData[] =
	{
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,1,1,1,1,1,1,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	};

	void TestStage::restart()
	{
		mAStar.inputMap(15, 15, MapData);
		mAStar.mClearance = 1;
		mStartPos = Vec2i(0, 0);
		mGlobalNode = nullptr;
	}

	void TestStage::onRender(float dFrame)
	{
		Graphics2D& g = Global::GetGraphics2D();

		MyAStar::TileMap& map = mAStar.mMap;

		RenderUtility::SetPen(g, EColor::Black);

		g.setTextColor(Color3ub(125, 125, 0));
		for (int i = 0; i < map.getSizeX(); ++i)
		{
			for (int j = 0; j < map.getSizeY(); ++j)
			{
				MyAStar::Tile& tile = map.getData(i, j);

				Vec2i renderPos = convertToScreen(Vec2i(i, j));

				if (tile.terrain)
				{
					RenderUtility::SetBrush(g, EColor::Gray);
				}
				else
				{
					RenderUtility::SetBrush(g, EColor::White);
				}

				g.drawRect(renderPos, Vec2i(TileLen, TileLen));

				InlineString< 32 > str;
				str.format("%d", tile.clearance);
				g.drawText(renderPos, Vec2i(TileLen, TileLen), str);
			}
		}

		if (mGlobalNode)
		{
			MyAStar::MapType& map = mAStar.getMap();

			for (MyAStar::MapType::iterator iter = map.begin(), itEnd = map.end();
				iter != itEnd; ++iter)
			{
				MyAStar::NodeType* node = *iter;

				if (!node->parent)
					continue;

				if (!node->isClose())
				{
					RenderUtility::SetPen(g, EColor::Green);
					Vec2i p1 = convertToScreen(node->state) + Vec2i(TileLen, TileLen) / 2;
					Vec2i p2 = convertToScreen(node->parent->state) + Vec2i(TileLen, TileLen) / 2;
					g.drawLine(p1, p2);
				}
				else if (!node->isPath())
				{
					RenderUtility::SetPen(g, EColor::Blue);
					Vec2i p1 = convertToScreen(node->state) + Vec2i(TileLen, TileLen) / 2;
					Vec2i p2 = convertToScreen(node->parent->state) + Vec2i(TileLen, TileLen) / 2;
					g.drawLine(p1, p2);
				}
			}

			RenderUtility::SetPen(g, EColor::Red);


			Vec2i prevPos = convertToScreen(mGlobalNode->state) + Vec2i(TileLen, TileLen) / 2;
			Node* node = mGlobalNode->parent;
			while (node)
			{
				Vec2i pos = convertToScreen(node->state) + Vec2i(TileLen, TileLen) / 2;
				g.drawLine(pos, prevPos);
				node = node->parent;
				prevPos = pos;
			}
		}
	}


}//namespace AStar
