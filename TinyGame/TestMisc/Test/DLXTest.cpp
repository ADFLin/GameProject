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
	static const float Length = 50;
	static const float Border = 5;
	auto renderScope = FExecutionUtil::RegisterRender([&mat](IGraphics2D& g)
	{
		g.setTextColor(Color3f(1, 1, 0));
		g.translateXForm(Border, Border + 20);
		mat.visitNode(
			TOverloaded
			{
				[&](DLX::MatrixColumn& matCol)
				{
					g.setBrush(Color3f(1, 0, 0));
					g.setPen(Color3f(0, 0, 0));
					g.drawText(Length * Vector2(matCol.col, 0) - Vector2(0,20), InlineString<>::Make("%d", matCol.count));
				},
				[&](DLX::Node& node)
				{
					g.setBrush(Color3f(1, 0, 0));
					g.setPen(Color3f(0, 0, 0));
					g.drawRect(Length * Vector2(node.col, node.row), Vector2(Length - 5, Length - 5));
				},
			}
		);
	}, Vec2i(mat.getColSize() * Length + 2 * Border, mat.getRowSize() * Length + 2 * Border + 20) );

	FExecutionUtil::Pause();
	mat.cover(mat.mCols[0]);
	FExecutionUtil::Pause();
	DLX::Node& node = DLX::Node::GetFromRow(*mat.mCols[0].rowLink.next);
	mat.cover(node);
	FExecutionUtil::Pause();

	mat.uncover(node);
	FExecutionUtil::Pause();

	mat.uncover(mat.mCols[0]);
	FExecutionUtil::Pause();

	DLX::Solver solver(mat);
	solver.solveAll();
	FExecutionUtil::Pause();

}

REGISTER_MISC_TEST_ENTRY("DLX Test", DLXTest);