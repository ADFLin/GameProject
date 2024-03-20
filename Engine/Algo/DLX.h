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

		template< class T, NodeLink T::*Member >
		static T* Cast(NodeLink& link) { return FTypeCast::MemberToClass(&link, Member); }

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
		int col;
		int count;
		NodeLink colLink;
		NodeLink rowLink;
#if _DEBUG
		bool bCovered = false;
#endif
		static MatrixColumn& GetFromCol(NodeLink& nodeLink)
		{
			return *NodeLink::Cast<MatrixColumn, &MatrixColumn::colLink>(nodeLink);
		}
	};

	struct Node
	{
		int col;
		int row;
		NodeLink rowLink;
		NodeLink colLink;

		static Node& GetFromRow(NodeLink& nodeLink)
		{
			return *NodeLink::Cast<Node, &Node::rowLink>(nodeLink);
		}

		static Node& GetFromCol(NodeLink& nodeLink)
		{
			return *NodeLink::Cast<Node, &Node::colLink>(nodeLink);
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
				for (NodeLink& rowLink : matCol.rowLink)
				{
					Node& node = Node::GetFromRow(rowLink);
					func(matCol, node);
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

		int getColSize() { return mCols.size(); }
		NodeLink header;
		TArray< MatrixColumn > mCols;
		TArray< Node, FixedSizeAllocator > mNodes;
	};


	class Solver
	{
	public:
		Solver(Matrix& mat)
			:mMat(mat)
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
		void solveStack(MatrixColumn* startCol);

		void handleSolutionFound()
		{
			++mSolutionCount;
		}

		TArray< LevelInfo, FixedSizeAllocator > mLevelStack;

		int mSolutionCount = 0;
		TArray< int , FixedSizeAllocator > mSolution;
		Matrix& mMat;
	};

}//namespace DLX

#endif 
//DLX_H_744134D9_7239_4D7F_BB2C_587E565EF220