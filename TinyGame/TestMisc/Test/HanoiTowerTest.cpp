#include "Async/Coroutines.h"
#include "StageRegister.h"

const int BlockCount = 8;
class HanoiTowerSolver
{
public:
	HanoiTowerSolver()
	{
		for (int i = 0; i < BlockCount; ++i)
			state[i] = 0;
	}

	void solve()
	{
		moveRecursive(0, 2, 1);
	}

	void doMove(int index, int to)
	{
		FMiscTestUtil::Pause();
		int from = state[index];
		state[index] = to;
		LogMsg("Move %d form %d to %d", index, from, to);
	}

	void moveRecursive(int index, int to, int other)
	{
		if (index >= BlockCount)
			return;

		int from = state[index];
		CHECK(from != to);
		int nextIndex = index + 1;
		moveRecursive(nextIndex, other, to);
		doMove(index, to);
		moveRecursive(nextIndex, to, from);
	}

	int state[BlockCount];
};

void SolveHanoiTowerTest()
{
	HanoiTowerSolver solver;

	static const float Offset = 200;
	static const float BlockHeight = 20;
	static const float Border = 10;

	auto renderScope = FMiscTestUtil::RegisterRender([&solver](IGraphics2D& g)
	{
		int count[3] = { 0 , 0 , 0 };
		g.setBrush(Color3f(1, 0, 0));
		g.setPen(Color3f(0, 0, 0));

		g.translateXForm(Border + Offset / 2, Border + ( BlockCount - 1 ) * BlockHeight);
		g.setBrush(Color3f(0, 1, 1));
		for (int i = 0; i < 3; ++i)
		{
			float sizeX = 10;
			g.drawRect(Vector2(i * Offset - sizeX / 2, -(BlockCount - 1) * BlockHeight), Vector2(sizeX, BlockCount * BlockHeight));
		}
		g.setBrush(Color3f(1, 0, 0));
		for (int i = 0; i < BlockCount; ++i)
		{
			int state = solver.state[i];
			float sizeX = (BlockCount - i) * 20;
			g.drawRect(Vector2(state * Offset - sizeX / 2, -count[state] * BlockHeight), Vector2(sizeX, BlockHeight));
			count[state] += 1;
		}
	}, Vec2i(Border * 2 + Offset * 3, BlockCount * BlockHeight + Border * 2));

	solver.solve();
	FMiscTestUtil::Pause();
}

REGISTER_MISC_TEST_ENTRY("Hanoi Tower", SolveHanoiTowerTest);
