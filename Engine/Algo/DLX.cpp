#include "DLX.h"

#include "LogSystem.h"
#include "ProfileSystem.h"
#include "MacroCommon.h"
#include "TemplateMisc.h"

namespace DLX
{
	void Matrix::build(int nRow, int nCol, uint8 data[])
	{
		mRowCount = nRow;
		header.initHeader();

		int numNode = std::count_if(data, data + nRow * nCol, [](uint8 a) { return !!a; });
		mCols.resize(nCol);

		//TODO : optimize it
		for (int i = 0; i < nCol; ++i)
		{
			auto& matCol = mCols[i];
			matCol.count = 0;
			matCol.col = i;
			matCol.rowLink.initHeader();
			header.add(matCol.colLink);
		}
		mNodes.reserve(numNode);
		int index = 0;
		for (int row = 0; row < nRow; ++row)
		{
			Node* fristRowNode = nullptr;
			for (int col = 0; col < nCol; ++col, ++index)
			{
				if (data[index] == 0)
					continue;

				Node* node = mNodes.addUninitialized(1);
				node->row = row;
				node->col = col;

				auto& matCol = mCols[col];
				matCol.rowLink.add(node->rowLink);
				matCol.count += 1;
				if (fristRowNode)
				{
					fristRowNode->colLink.add(node->colLink);
				}
				else
				{
					node->colLink.initHeader();
					fristRowNode = node;
				}
			}
		}
	}

	void Matrix::cover(MatrixColumn& matCol)
	{
#if _DEBUG
		CHECK(matCol.bCovered == false);
		matCol.bCovered = true;
#endif
		matCol.colLink.unlink();
		for (NodeLink& linkRow : matCol.rowLink)
		{
			Node& node = Node::GetFromRow(linkRow);
			for (NodeLink& linkCol : node.colLink)
			{
				Node& nodeCol = Node::GetFromCol(linkCol);
				nodeCol.rowLink.unlink();
				mCols[nodeCol.col].count -= 1;
			}
		}
	}

	void Matrix::uncover(MatrixColumn& matCol)
	{
#if _DEBUG
		CHECK(matCol.bCovered == true);
		matCol.bCovered = false;
#endif
		for (NodeLink& linkRow : MakeReverseView(matCol.rowLink))
		{
			Node& node = Node::GetFromRow(linkRow);
			for (NodeLink& linkCol : MakeReverseView(node.colLink))
			{
				Node& nodeCol = Node::GetFromCol(linkCol);
				nodeCol.rowLink.relink();
				mCols[nodeCol.col].count += 1;
			}
		}
		matCol.colLink.relink();
	}

	void Matrix::cover(Node& selectedNode)
	{
		for (NodeLink& linkCol : selectedNode.colLink)
		{
			Node& nodeCol = Node::GetFromCol(linkCol);
			cover(mCols[nodeCol.col]);
		}
	}

	void Matrix::uncover(Node& selectedNode)
	{
		for (NodeLink& linkCol : MakeReverseView(selectedNode.colLink))
		{
			Node& nodeCol = Node::GetFromCol(linkCol);
			uncover(mCols[nodeCol.col]);
		}
	}

	void Matrix::removeUnsedColumns()
	{
		for (NodeLink& linkCol : header)
		{
			MatrixColumn& matCol = MatrixColumn::GetFromCol(linkCol);
			if (matCol.count == 0)
			{
				matCol.colLink.unlink();
			}
		}
	}

	MatrixColumn* Solver::selectColumn()
	{
#if 1
		NodeLink* colLink = mMat->header.next;
		MatrixColumn* result;
		int minCount;
		{
			MatrixColumn& matCol = MatrixColumn::GetFromCol(*colLink);
			if (matCol.count == 0)
				return nullptr;

			result = &matCol;
			minCount = matCol.count;
		}

		for (; colLink != &mMat->header; colLink = colLink->next)
		{
			MatrixColumn& matCol = MatrixColumn::GetFromCol(*colLink);
			if (matCol.count == 0)
				return nullptr;

			if (matCol.count < minCount)
			{
				result = &matCol;
				minCount = matCol.count;
			}
		}
#else
		MatrixColumn* result = nullptr;
		int minCount = std::numeric_limits<int>::max();
		for (NodeLink& linkCol : mMat->header)
		{
			MatrixColumn& matCol = MatrixColumn::GetFromCol(linkCol);
			if (matCol.count == 0)
				return nullptr;

			if (matCol.count < minCount)
			{
				result = &matCol;
				minCount = matCol.count;
			}
		}
#endif
		return result;
	}

	void Solver::solveAll()
	{
		Profile_GetTicks(&timeStart);
		mSolutionCount = 0;
		mSolution.reserve(mMat->getColSize());

		if (mMat->isEmpty())
			return;

		MatrixColumn* selectedCol = selectColumn();
		if (selectedCol)
		{
			if (bRecursive)
			{
				solveInternal<true>(selectedCol);
			}
			else
			{
				mLevelStack.reserve(mMat->getColSize());
				solveInternal<false>(selectedCol);
			}
		}
	}

	template< bool bRecursive>
	void Solver::solveInternal(MatrixColumn* startCol)
	{
		mMat->cover(*startCol);
		int index = 0;
		for (NodeLink& rowLink : startCol->rowLink)
		{
			Node& node = Node::GetFromRow(rowLink);
			mSolution.push_back(node.row);
			mMat->cover(node);

			if (mMat->isEmpty())
			{
				handleSolutionFound();
			}
			else if (MatrixColumn* selectedCol = selectColumn())
			{
				if constexpr (bRecursive)
				{
					solveRec(selectedCol);
				}
				else
				{
					solveStack(selectedCol);
				}			
			}

			mMat->uncover(node);
			mSolution.pop_back();

			++index;
			float pct = 100.f * index / startCol->count;
			uint64 timeEnd;
			Profile_GetTicks(&timeEnd);
			double time = double(timeEnd - timeStart) / Profile_GetTickRate();
			LogMsg("Solve progress = %03.2f%% , Duration = %.2lf", pct, time);
		}
		mMat->uncover(*startCol);
	}


	void Solver::solveRec(MatrixColumn* curCol)
	{
		mMat->cover(*curCol);
		for (NodeLink& rowLink : curCol->rowLink)
		{
			Node& node = Node::GetFromRow(rowLink);
			mSolution.push_back(node.row);
			mMat->cover(node);

			if (mMat->isEmpty())
			{
				handleSolutionFound();
			}
			else if (MatrixColumn* selectedCol = selectColumn())
			{
				solveRec(selectedCol);
			}

			mMat->uncover(node);
			mSolution.pop_back();
		}
		mMat->uncover(*curCol);
	}

	void Solver::solveStack(MatrixColumn* curCol)
	{
		mMat->cover(*curCol);
		NodeLink* curLink = curCol->rowLink.next;
		for (;;)
		{
			if (curLink != &curCol->rowLink)
			{
				Node& node = Node::GetFromRow(*curLink);
				mSolution.push_back(node.row);
				mMat->cover(node);

				if (mMat->isEmpty())
				{
					handleSolutionFound();
				}
				else if (MatrixColumn* selectedCol = selectColumn())
				{
					mLevelStack.push_back({ curCol , curLink });
					curCol = selectedCol;
					mMat->cover(*curCol);
					curLink = curCol->rowLink.next;
					continue;
				}
			}
			else
			{
				mMat->uncover(*curCol);
				if (mLevelStack.empty())
					break;

				LevelInfo& info = mLevelStack.back();
				curCol = info.matCol;
				curLink = info.link;
				mLevelStack.pop_back();
			}

			Node& node = Node::GetFromRow(*curLink);
			mMat->uncover(node);
			mSolution.pop_back();
			curLink = curLink->next;
		}
	}

}//namespace DLX


