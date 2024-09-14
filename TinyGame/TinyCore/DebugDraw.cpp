#include "DebugDraw.h"
#include "DataStructure/Array.h"
#include "GameGraphics2D.h"

class DebugDrawBatcher : public IDebugDrawBatcher
{
public:
	DebugDrawBatcher()
	{
		IDebugDrawBatcher::Set(this);
	}
	~DebugDrawBatcher()
	{
		IDebugDrawBatcher::Set(nullptr);
	}

	void addPoint(Vector2 const& pos, Color3ub const& color, float size)
	{
		mPoints.push_back({ pos , color , size > 0.0 ? size: 1 });
	}
	void addLine(Vector2 const& posStart, Vector2 const& posEnd, Color3ub const& color, float thickneess)
	{
		mLines.push_back({ posStart, posEnd, color , thickneess > 0 ? thickneess : 1 });
	}
	void addText(Vector2 const& pos, char const* text, Color3ub const& color)
	{
		mTexts.push_back({ pos , text, color });
	}

	void clear()
	{
		mPoints.clear();
		mLines.clear();
		mTexts.clear();
	}

	struct PointElement
	{
		Vector2  pos;
		Color3ub color;
		float    size;
	};

	struct LineElement
	{
		Vector2 posStart;
		Vector2 posEnd;
		Color3ub color;
		float thickneess;
	};

	struct TextElement
	{
		Vector2  pos;
		std::string content;
		Color3ub color;
	};

	void render(IGraphics2D& g)
	{
		for (auto const& point : mPoints)
		{
			g.setPen(point.color);
			g.setBrush(point.color);
			g.drawRect(point.pos - Vector2(point.size, point.size) / 2, Vector2(point.size, point.size));
		}

		for (auto const& line : mLines)
		{
			g.setPen(line.color, line.thickneess);
			g.drawLine(line.posStart, line.posEnd);
		}

		for (auto const& text : mTexts)
		{
			g.setTextColor(text.color);
			g.drawText(text.pos, text.content.c_str());
		}
	}
	TArray< PointElement > mPoints;
	TArray< LineElement > mLines;
	TArray< TextElement > mTexts;

}GDebugDraw;

void DrawDebugCommit(IGraphics2D& g)
{
	GDebugDraw.render(g);
}

