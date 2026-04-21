#include "HashLifeAlgo.h"

#include "ProfileSystem.h"

#include <cassert>

namespace Life
{
	namespace
	{
		int CalcPow2Level(int value)
		{
			int size = 1;
			int level = 0;
			while (size < value)
			{
				size <<= 1;
				++level;
			}
			return level;
		}

		bool GetLeafValue(HashLifeAlgo::Node* node)
		{
			return node->population != 0;
		}

		void FillLevel1Cells(HashLifeAlgo::Node* node, bool outCells[2][2])
		{
			outCells[0][0] = GetLeafValue(node->nw);
			outCells[0][1] = GetLeafValue(node->ne);
			outCells[1][0] = GetLeafValue(node->sw);
			outCells[1][1] = GetLeafValue(node->se);
		}
	}

	HashLifeAlgo::HashLifeAlgo(int32 x, int32 y)
		:mWidth(x)
		,mHeight(y)
	{
		mRule.bWarpX = false;
		mRule.bWarpY = false;

		mDeadLeaf = new Node;
		mAliveLeaf = new Node;

		mDeadLeaf->population = 0;
		mAliveLeaf->population = 1;

		mAllNodes.push_back(mDeadLeaf);
		mAllNodes.push_back(mAliveLeaf);
		mEmptyCache.push_back(mDeadLeaf);

		mRootLevel = Math::Max(2, CalcPow2Level(Math::Max(x, y)) + 2);
		mRoot = makeEmpty(mRootLevel);
		mBoundCache.invalidate();
	}

	HashLifeAlgo::~HashLifeAlgo()
	{
		for (Node* node : mAllNodes)
		{
			delete node;
		}
	}

	HashLifeAlgo::Node* HashLifeAlgo::getLeaf(bool bAlive)
	{
		return bAlive ? mAliveLeaf : mDeadLeaf;
	}

	HashLifeAlgo::Node* HashLifeAlgo::makeNode(Node* nw, Node* ne, Node* sw, Node* se)
	{
		assert(nw && ne && sw && se);
		assert(nw->level == ne->level);
		assert(nw->level == sw->level);
		assert(nw->level == se->level);

		NodeKey key{ nw, ne, sw, se };
		auto iter = mNodeMap.find(key);
		if (iter != mNodeMap.end())
		{
			return iter->second;
		}

		Node* node = new Node;
		node->level = nw->level + 1;
		node->population = nw->population + ne->population + sw->population + se->population;
		node->nw = nw;
		node->ne = ne;
		node->sw = sw;
		node->se = se;

		mAllNodes.push_back(node);
		mNodeMap.emplace(key, node);
		return node;
	}

	HashLifeAlgo::Node* HashLifeAlgo::makeEmpty(uint32 level)
	{
		if (level == 0)
		{
			return mDeadLeaf;
		}
		while (mEmptyCache.size() <= level)
		{
			mEmptyCache.push_back(nullptr);
		}
		for (uint32 i = 1; i <= level; ++i)
		{
			if (mEmptyCache[i] == nullptr)
			{
				Node* child = makeEmpty(i - 1);
				mEmptyCache[i] = makeNode(child, child, child, child);
			}
		}
		return mEmptyCache[level];
	}

	HashLifeAlgo::Node* HashLifeAlgo::setCell(Node* node, uint32 level, int x, int y, bool value)
	{
		if (node == nullptr)
		{
			node = makeEmpty(level);
		}
		if (level == 0)
		{
			return getLeaf(value);
		}

		int halfSize = 1 << (level - 1);
		Node* nw = node->nw;
		Node* ne = node->ne;
		Node* sw = node->sw;
		Node* se = node->se;

		if (y < halfSize)
		{
			if (x < halfSize)
				nw = setCell(nw, level - 1, x, y, value);
			else
				ne = setCell(ne, level - 1, x - halfSize, y, value);
		}
		else
		{
			if (x < halfSize)
				sw = setCell(sw, level - 1, x, y - halfSize, value);
			else
				se = setCell(se, level - 1, x - halfSize, y - halfSize, value);
		}

		return makeNode(nw, ne, sw, se);
	}

	bool HashLifeAlgo::getCell(Node* node, uint32 level, int x, int y) const
	{
		if (node->population == 0)
			return false;
		if (level == 0)
			return node->population != 0;

		int halfSize = 1 << (level - 1);
		if (y < halfSize)
		{
			if (x < halfSize)
				return getCell(node->nw, level - 1, x, y);
			return getCell(node->ne, level - 1, x - halfSize, y);
		}
		if (x < halfSize)
			return getCell(node->sw, level - 1, x, y - halfSize);
		return getCell(node->se, level - 1, x - halfSize, y - halfSize);
	}

	HashLifeAlgo::Node* HashLifeAlgo::centeredSubnode(Node* node)
	{
		return makeNode(node->nw->se, node->ne->sw, node->sw->ne, node->se->nw);
	}

	HashLifeAlgo::Node* HashLifeAlgo::centeredHorizontal(Node* left, Node* right)
	{
		return makeNode(left->ne->se, right->nw->sw, left->se->ne, right->sw->nw);
	}

	HashLifeAlgo::Node* HashLifeAlgo::centeredVertical(Node* top, Node* bottom)
	{
		return makeNode(top->sw->se, top->se->sw, bottom->nw->ne, bottom->ne->nw);
	}

	HashLifeAlgo::Node* HashLifeAlgo::centeredSubSubnode(Node* node)
	{
		return makeNode(node->nw->se->se, node->ne->sw->sw, node->sw->ne->ne, node->se->nw->nw);
	}

	HashLifeAlgo::Node* HashLifeAlgo::middleHorizontal(Node* left, Node* right)
	{
		return makeNode(left->ne, right->nw, left->se, right->sw);
	}

	HashLifeAlgo::Node* HashLifeAlgo::middleVertical(Node* top, Node* bottom)
	{
		return makeNode(top->sw, top->se, bottom->nw, bottom->ne);
	}

	HashLifeAlgo::Node* HashLifeAlgo::expandRoot(Node* node)
	{
		Node* empty = makeEmpty(node->level - 1);
		Node* nw = makeNode(empty, empty, empty, node->nw);
		Node* ne = makeNode(empty, empty, node->ne, empty);
		Node* sw = makeNode(empty, node->sw, empty, empty);
		Node* se = makeNode(node->se, empty, empty, empty);
		return makeNode(nw, ne, sw, se);
	}

	HashLifeAlgo::Node* HashLifeAlgo::calcBaseSuccessor(Node* node)
	{
		bool cells[4][4] = {};
		bool block[2][2];

		FillLevel1Cells(node->nw, block);
		cells[0][0] = block[0][0]; cells[0][1] = block[0][1];
		cells[1][0] = block[1][0]; cells[1][1] = block[1][1];

		FillLevel1Cells(node->ne, block);
		cells[0][2] = block[0][0]; cells[0][3] = block[0][1];
		cells[1][2] = block[1][0]; cells[1][3] = block[1][1];

		FillLevel1Cells(node->sw, block);
		cells[2][0] = block[0][0]; cells[2][1] = block[0][1];
		cells[3][0] = block[1][0]; cells[3][1] = block[1][1];

		FillLevel1Cells(node->se, block);
		cells[2][2] = block[0][0]; cells[2][3] = block[0][1];
		cells[3][2] = block[1][0]; cells[3][3] = block[1][1];

		Node* result[2][2];
		for (int y = 0; y < 2; ++y)
		{
			for (int x = 0; x < 2; ++x)
			{
				int cx = x + 1;
				int cy = y + 1;
				uint32 count = 0;
				for (int oy = -1; oy <= 1; ++oy)
				{
					for (int ox = -1; ox <= 1; ++ox)
					{
						if (ox == 0 && oy == 0)
							continue;
						count += cells[cy + oy][cx + ox] ? 1 : 0;
					}
				}
				result[y][x] = getLeaf(mRule.getEvoluteValue(count, cells[cy][cx]) != 0);
			}
		}

		return makeNode(result[0][0], result[0][1], result[1][0], result[1][1]);
	}

	HashLifeAlgo::Node* HashLifeAlgo::calcSuccessor(Node* node)
	{
		assert(node);
		assert(node->level >= 2);

		if (node->next)
			return node->next;
		if (node->population == 0)
			return node->next = makeEmpty(node->level - 1);
		if (node->level == 2)
			return node->next = calcBaseSuccessor(node);

		Node* n00 = centeredSubnode(node->nw);
		Node* n01 = centeredHorizontal(node->nw, node->ne);
		Node* n02 = centeredSubnode(node->ne);
		Node* n10 = centeredVertical(node->nw, node->sw);
		Node* n11 = centeredSubSubnode(node);
		Node* n12 = centeredVertical(node->ne, node->se);
		Node* n20 = centeredSubnode(node->sw);
		Node* n21 = centeredHorizontal(node->sw, node->se);
		Node* n22 = centeredSubnode(node->se);

		Node* nodeNW = makeNode(n00, n01, n10, n11);
		Node* nodeNE = makeNode(n01, n02, n11, n12);
		Node* nodeSW = makeNode(n10, n11, n20, n21);
		Node* nodeSE = makeNode(n11, n12, n21, n22);

		assert(nodeNW->level + 1 == node->level);
		assert(nodeNE->level + 1 == node->level);
		assert(nodeSW->level + 1 == node->level);
		assert(nodeSE->level + 1 == node->level);

		Node* nw = calcSuccessor(nodeNW);
		Node* ne = calcSuccessor(nodeNE);
		Node* sw = calcSuccessor(nodeSW);
		Node* se = calcSuccessor(nodeSE);

		node->next = makeNode(nw, ne, sw, se);
		assert(node->next->level + 1 == node->level);
		return node->next;
	}

	HashLifeAlgo::Node* HashLifeAlgo::calcFastSuccessor(Node* node)
	{
		assert(node);
		assert(node->level >= 2);

		if (node->population == 0)
			return makeEmpty(node->level - 1);
		if (node->level == 2)
			return calcBaseSuccessor(node);

		Node* x0 = calcFastSuccessor(node->nw);
		Node* x1 = calcFastSuccessor(middleHorizontal(node->nw, node->ne));
		Node* x2 = calcFastSuccessor(node->ne);
		Node* x3 = calcFastSuccessor(middleVertical(node->nw, node->sw));
		Node* x4 = calcFastSuccessor(centeredSubnode(node));
		Node* x5 = calcFastSuccessor(middleVertical(node->ne, node->se));
		Node* x6 = calcFastSuccessor(node->sw);
		Node* x7 = calcFastSuccessor(middleHorizontal(node->sw, node->se));
		Node* x8 = calcFastSuccessor(node->se);

		return makeNode(
			calcFastSuccessor(makeNode(x0, x1, x3, x4)),
			calcFastSuccessor(makeNode(x1, x2, x4, x5)),
			calcFastSuccessor(makeNode(x3, x4, x6, x7)),
			calcFastSuccessor(makeNode(x4, x5, x7, x8))
		);
	}

	HashLifeAlgo::Node* HashLifeAlgo::calcAdvance(Node* node, uint32 exp)
	{
		assert(node);
		assert(node->level >= exp + 2);

		AdvanceKey key{ node, exp };
		auto iter = mAdvanceMap.find(key);
		if (iter != mAdvanceMap.end())
		{
			return iter->second;
		}

		if (node->population == 0)
			return makeEmpty(node->level - 1);

		if (node->level == exp + 2)
		{
			Node* result = (exp == 0) ? calcSuccessor(node) : calcFastSuccessor(node);
			mAdvanceMap.emplace(key, result);
			return result;
		}

		Node* n00 = centeredSubnode(node->nw);
		Node* n01 = centeredHorizontal(node->nw, node->ne);
		Node* n02 = centeredSubnode(node->ne);
		Node* n10 = centeredVertical(node->nw, node->sw);
		Node* n11 = centeredSubSubnode(node);
		Node* n12 = centeredVertical(node->ne, node->se);
		Node* n20 = centeredSubnode(node->sw);
		Node* n21 = centeredHorizontal(node->sw, node->se);
		Node* n22 = centeredSubnode(node->se);

		Node* nodeNW = makeNode(n00, n01, n10, n11);
		Node* nodeNE = makeNode(n01, n02, n11, n12);
		Node* nodeSW = makeNode(n10, n11, n20, n21);
		Node* nodeSE = makeNode(n11, n12, n21, n22);

		Node* nw = calcAdvance(nodeNW, exp);
		Node* ne = calcAdvance(nodeNE, exp);
		Node* sw = calcAdvance(nodeSW, exp);
		Node* se = calcAdvance(nodeSE, exp);

		Node* result = makeNode(nw, ne, sw, se);
		mAdvanceMap.emplace(key, result);
		return result;
	}

	bool HashLifeAlgo::setCell(int x, int y, uint8 value)
	{
		if (x < 0 || y < 0 || x >= mWidth || y >= mHeight)
			return false;

		mRoot = setCell(mRoot, mRootLevel, x, y, value != 0);
		mBoundDirty = true;
		return true;
	}

	uint8 HashLifeAlgo::getCell(int x, int y)
	{
		if (x < 0 || y < 0 || x >= mWidth || y >= mHeight)
			return 0;
		return getCell(mRoot, mRootLevel, x, y) ? 1 : 0;
	}

	void HashLifeAlgo::clearCell()
	{
		mRoot = makeEmpty(mRootLevel);
		mBoundDirty = true;
	}

	BoundBox HashLifeAlgo::getBound()
	{
		if (mBoundDirty)
		{
			updateBoundCache();
		}
		return mBoundCache;
	}

	BoundBox HashLifeAlgo::getLimitBound()
	{
		BoundBox result;
		result.min = Vec2i::Zero();
		result.max = Vec2i(mWidth - 1, mHeight - 1);
		return result;
	}

	template< class Func >
	void HashLifeAlgo::visitLiveCells(Node* node, int x, int y, int size, Func&& func) const
	{
		if (node->population == 0)
			return;
		if (size == 1)
		{
			func(x, y);
			return;
		}

		int half = size / 2;
		visitLiveCells(node->nw, x, y, half, func);
		visitLiveCells(node->ne, x + half, y, half, func);
		visitLiveCells(node->sw, x, y + half, half, func);
		visitLiveCells(node->se, x + half, y + half, half, func);
	}

	void HashLifeAlgo::updateBoundCache()
	{
		mBoundCache.invalidate();
		if (mRoot->population == 0)
		{
			mBoundDirty = false;
			return;
		}

		visitLiveCells(mRoot, 0, 0, 1 << mRootLevel, [this](int x, int y)
		{
			if (x < mWidth && y < mHeight)
			{
				mBoundCache.addPoint(Vec2i(x, y));
			}
		});
		mBoundDirty = false;
	}

	void HashLifeAlgo::getPattern(TArray<Vec2i>& outList)
	{
		visitLiveCells(mRoot, 0, 0, 1 << mRootLevel, [this, &outList](int x, int y)
		{
			if (x < mWidth && y < mHeight)
			{
				outList.push_back(Vec2i(x, y));
			}
		});
	}

	void HashLifeAlgo::getPattern(BoundBox const& bound, TArray<Vec2i>& outList)
	{
		BoundBox boundClip = bound.intersection(getLimitBound());
		if (!boundClip.isValid())
			return;

		visitLiveCells(mRoot, 0, 0, 1 << mRootLevel, [&boundClip, &outList](int x, int y)
		{
			Vec2i pos(x, y);
			if (boundClip.isInside(pos))
			{
				outList.push_back(pos);
			}
		});
	}

	void HashLifeAlgo::step()
	{
		PROFILE_ENTRY("HashLife.Step");
		if (mRoot->population == 0)
			return;

		mRoot = expandRoot(mRoot);
		mRoot = calcSuccessor(mRoot);
		mRootLevel = mRoot->level;
		mBoundDirty = true;
	}

	void HashLifeAlgo::evolate(int nStep)
	{
		if (nStep <= 0)
			return;

		PROFILE_ENTRY("HashLife.Evaluate");
		while (nStep > 0)
		{
			if (mRoot->population == 0)
				break;

			uint32 exp = 0;
			while ((1 << (exp + 1)) <= nStep)
			{
				++exp;
			}

			uint32 const maxExp = (mRootLevel > 0) ? (mRootLevel - 1) : 0;
			if (exp > maxExp)
			{
				exp = maxExp;
			}

			Node* workRoot = expandRoot(mRoot);
			mRoot = calcAdvance(workRoot, exp);
			mRootLevel = mRoot->level;
			nStep -= (1 << exp);
		}
		mBoundDirty = true;
	}

	void HashLifeAlgo::draw(IRenderer& renderer, Viewport const& viewport, BoundBox const& boundRender)
	{
		BoundBox boundClip = boundRender.intersection(getBound());
		if (!boundClip.isValid())
			return;

		visitLiveCells(mRoot, 0, 0, 1 << mRootLevel, [this, &renderer, &boundClip](int x, int y)
		{
			if (x < mWidth && y < mHeight)
			{
				Vec2i pos(x, y);
				if (boundClip.isInside(pos))
				{
					renderer.drawCell(x, y);
				}
			}
		});
	}
}
