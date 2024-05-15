#pragma once

#ifndef DLX_H_744134D9_7239_4D7F_BB2C_587E565EF220
#define DLX_H_744134D9_7239_4D7F_BB2C_587E565EF220

#include "Core/IntegerType.h"
#include "DataStructure/Array.h"
#include "TypeCast.h"

namespace DLX
{
	struct NodeLink
	{
		NodeLink* next;
		NodeLink* prev;

		void initHeader()
		{
			next = this;
			prev = this;
		}

		void unlink()
		{
			next->prev = prev;
			prev->next = next;
		}
		void relink()
		{
			next->prev = this;
			prev->next = this;
		}

		void add(NodeLink& link)
		{
			prev->next = &link;
			link.prev = prev;
			this->prev = &link;
			link.next = this;
		}
		struct IteratorBase
		{
			NodeLink* link;

			using reference = NodeLink&;
			using pointer = NodeLink*;

			reference operator*() { return *link; }
			pointer   operator->() { return link; }
			bool operator == (IteratorBase other) const { return link == other.link; }
			bool operator != (IteratorBase other) const { return link != other.link; }

		};
		struct Iterator : IteratorBase
		{
			Iterator  operator++() { NodeLink* tempLink = link; link = link->next; return Iterator{ tempLink }; }
			Iterator  operator--() { NodeLink* tempLink = link; link = link->prev; return Iterator{ tempLink }; }
			Iterator& operator++(int) { link = link->next; return *this; }
			Iterator& operator--(int) { link = link->prev; return *this; }
		};

		struct RIterator : IteratorBase
		{
			RIterator  operator++() { NodeLink* tempLink = link; link = link->prev; return RIterator{ tempLink }; }
			RIterator  operator--() { NodeLink* tempLink = link; link = link->next; return RIterator{ tempLink }; }
			RIterator& operator++(int) { link = link->prev; return *this; }
			RIterator& operator--(int) { link = link->next; return *this; }
		};

		Iterator begin() { return Iterator{ next }; }
		Iterator end() { return Iterator{ this }; }


		RIterator rbegin() { return RIterator{ prev }; }
		RIterator rend() { return RIterator{ this }; }
	};

	struct MatrixColumn
	{
		NodeLink colLink;
		NodeLink rowLink;

		int count;
		int col;
#if _DEBUG
		bool bCovered = false;
#endif
		static MatrixColumn& GetFromCol(NodeLink& nodeLink)
		{
			return *FTypeCast::MemberToClass(&nodeLink, &MatrixColumn::colLink);
		}
	};

	struct Node
	{
		NodeLink colLink;
		NodeLink rowLink;

		int col;
		int row;

		static Node& GetFromRow(NodeLink& nodeLink)
		{
			return *FTypeCast::MemberToClass(&nodeLink, &Node::rowLink);
		}

		static Node& GetFromCol(NodeLink& nodeLink)
		{
			return *FTypeCast::MemberToClass(&nodeLink, &Node::colLink);
		}
	};

	struct Matrix
	{
		void build(int nRow, int nCol, uint8 data[]);

		void cover(MatrixColumn& matCol);
		void uncover(MatrixColumn& matCol);
		void cover(Node& selectedNode);
		void uncover(Node& selectedNode);

		template< typename TFunc >
		void visitNode(TFunc&& func)
		{
			for (NodeLink& colLink : header)
			{
				MatrixColumn& matCol = MatrixColumn::GetFromCol(colLink);
				if constexpr (TCheckConcept< CFunctionCallable, TFunc, MatrixColumn >::Value)
				{
					func(matCol);
				}
				for (NodeLink& rowLink : matCol.rowLink)
				{
					Node& node = Node::GetFromRow(rowLink);
					func(node);
				}
			}
		}

		void removeUnsedColumns();

		void copyFrom(Matrix const& rhs)
		{



		}
		bool isEmpty() const
		{
			return header.next == &header;
		}

		int getColSize() const { return mCols.size(); }
		int getRowSize() const { return mRowCount; }
		NodeLink header;
		TArray< MatrixColumn > mCols;
		TArray< Node, FixedSizeAllocator > mNodes;
		int mRowCount;
	};


	class Solver
	{
	public:
		Solver()
			:mMat(nullptr)
		{

		}

		Solver(Matrix& mat)
			:mMat(&mat)
		{

		}

		bool bRecursive = true;

		uint64 timeStart;
		MatrixColumn* selectColumn();

		void solveAll();

		template< bool bRecursive>
		void solveInternal(MatrixColumn* startCol);

		struct LevelInfo
		{
			MatrixColumn* matCol;
			NodeLink*     link;
		};

		void solveRec(MatrixColumn* curCol);
		void solveStack(MatrixColumn* curCol);

		void handleSolutionFound()
		{
			++mSolutionCount;
		}

		TArray< LevelInfo, FixedSizeAllocator > mLevelStack;

		int mSolutionCount = 0;
		TArray< int , FixedSizeAllocator > mSolution;
		Matrix* mMat;
	};

}//namespace DLX

#endif //DLX_H_744134D9_7239_4D7F_BB2C_587E565EF220