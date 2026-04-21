#pragma once
#ifndef HashLifeAlgo_H_CBB8A4E5_9174_4102_8ED7_532D147346E1
#define HashLifeAlgo_H_CBB8A4E5_9174_4102_8ED7_532D147346E1

#include "LifeCore.h"

#include <unordered_map>

namespace Life
{
	class HashLifeAlgo : public IAlgorithm, public IRenderProxy
	{
	public:
		HashLifeAlgo(int32 x, int32 y);
		~HashLifeAlgo();

		bool setCell(int x, int y, uint8 value) final;
		uint8 getCell(int x, int y) final;
		void clearCell() final;
		BoundBox getBound() final;
		BoundBox getLimitBound() final;
		void getPattern(TArray<Vec2i>& outList) final;
		void getPattern(BoundBox const& bound, TArray<Vec2i>& outList) final;
		void step() final;
		void evolate(int nStep) final;

		void draw(IRenderer& renderer, Viewport const& viewport, BoundBox const& boundRender) final;
		IRenderProxy* getRenderProxy() final { return this; }

		struct Node
		{
			uint32 level = 0;
			uint32 population = 0;
			Node* nw = nullptr;
			Node* ne = nullptr;
			Node* sw = nullptr;
			Node* se = nullptr;
			Node* next = nullptr;
		};

	private:
		struct NodeKey
		{
			Node* nw;
			Node* ne;
			Node* sw;
			Node* se;

			bool operator == (NodeKey const& rhs) const
			{
				return nw == rhs.nw && ne == rhs.ne && sw == rhs.sw && se == rhs.se;
			}
		};

		struct NodeKeyHash
		{
			size_t operator()(NodeKey const& key) const
			{
				return HashValues(key.nw, key.ne, key.sw, key.se);
			}
		};

		struct AdvanceKey
		{
			Node* node;
			uint32 exp;

			bool operator == (AdvanceKey const& rhs) const
			{
				return node == rhs.node && exp == rhs.exp;
			}
		};

		struct AdvanceKeyHash
		{
			size_t operator()(AdvanceKey const& key) const
			{
				return HashValues(key.node, key.exp);
			}
		};

		Node* getLeaf(bool bAlive);
		Node* makeNode(Node* nw, Node* ne, Node* sw, Node* se);
		Node* makeEmpty(uint32 level);
		Node* setCell(Node* node, uint32 level, int x, int y, bool value);
		bool  getCell(Node* node, uint32 level, int x, int y) const;

		Node* centeredSubnode(Node* node);
		Node* centeredHorizontal(Node* left, Node* right);
		Node* centeredVertical(Node* top, Node* bottom);
		Node* centeredSubSubnode(Node* node);
		Node* middleHorizontal(Node* left, Node* right);
		Node* middleVertical(Node* top, Node* bottom);
		Node* calcFastSuccessor(Node* node);
		Node* expandRoot(Node* node);
		Node* calcSuccessor(Node* node);
		Node* calcAdvance(Node* node, uint32 exp);
		Node* calcBaseSuccessor(Node* node);

		template< class Func >
		void visitLiveCells(Node* node, int x, int y, int size, Func&& func) const;

		void updateBoundCache();

		int mWidth = 0;
		int mHeight = 0;
		uint32 mRootLevel = 0;
		Node* mRoot = nullptr;

		Node* mDeadLeaf = nullptr;
		Node* mAliveLeaf = nullptr;
		std::unordered_map<NodeKey, Node*, NodeKeyHash> mNodeMap;
		std::unordered_map<AdvanceKey, Node*, AdvanceKeyHash> mAdvanceMap;
		TArray<Node*> mEmptyCache;
		TArray<Node*> mAllNodes;

		bool mBoundDirty = true;
		BoundBox mBoundCache;
		Rule mRule;
	};
}

#endif
