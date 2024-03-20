#include "StageRegister.h"
#include "Algo/DLX.h"

void DLXTest()
{
	uint8 data[] =
	{
		0, 0, 1, 0, 1, 1, 0,
		1, 0, 0, 1, 0, 0, 1,
		0, 1, 1, 0, 0, 1, 0,
		1, 0, 0, 1, 0, 0, 0,
		0, 1, 0, 0, 0, 0, 1,
		0, 0, 0, 1, 1, 0, 1,
	};

	DLX::Matrix mat;
	mat.build(6, 7, data);

	FMiscTestUtil::RegisterRender([&mat](IGraphics2D& g)
	{
		float len = 50;

		g.setTextColor(Color3f(1, 1, 0));
		for (auto const& matCol : mat.mCols)
		{
			g.drawText(len * Vector2(matCol.col, 0) ,InlineString<>::Make("%d", matCol.count));
		}

		g.beginXForm();

		g.translateXForm(200, 200);
		mat.visitNode([&](DLX::MatrixColumn& matCol, DLX::Node& node)
		{
			g.setBrush(Color3f(1, 0, 0));
			g.setPen(Color3f(0, 0, 0));
			g.drawRect(len * Vector2(matCol.col, node.row), Vector2(len - 5, len - 5));
		});

		g.finishXForm();
	});

	FMiscTestUtil::PauseThread();
	mat.cover(mat.mCols[0]);
	FMiscTestUtil::PauseThread();
	DLX::Node& node = DLX::Node::GetFromRow(*mat.mCols[0].rowLink.next);
	mat.cover(node);
	FMiscTestUtil::PauseThread();

	mat.uncover(node);
	FMiscTestUtil::PauseThread();

	mat.uncover(mat.mCols[0]);
	FMiscTestUtil::PauseThread();

	DLX::Solver solver(mat);
	solver.solveAll();
	FMiscTestUtil::PauseThread();

}

REGISTER_MISC_TEST_ENTRY("DLX Test", DLXTest);