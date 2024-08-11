#pragma once
#ifndef SkipList_H_CBDB5698_BC43_432C_8EF9_C4C1CF2F8ED8
#define SkipList_H_CBDB5698_BC43_432C_8EF9_C4C1CF2F8ED8

#include "Array.h"
#include "Meta/MetaBase.h"

#include <functional>
#include <algorithm>
#include <stdlib.h>

template< typename T, typename CmpFunc = std::less<> >
class TSkipList
{
public:

	TSkipList()
	{
		init();
	}

	TSkipList(std::initializer_list<T> list)
	{
		init();
		addRange(list.begin(), list.end());
	}

	template< typename Iter, TEnableIf_Type< TIsIterator<Iter>::Value, bool > = true >
	TSkipList(Iter itBegin, Iter itEnd)
	{
		init();
		addRange(itBegin, itEnd);
	}

	template< typename Iter, TEnableIf_Type< TIsIterator<Iter>::Value, bool > = true >
	void addRange(Iter itBegin, Iter itEnd)
	{
		for (Iter it = itBegin; it != itEnd; ++it)
		{
			insert(*it);
		}
	}

	~TSkipList()
	{
		cleanupNodes();
	}

	int getLevel() const { return mLevel; }
	int size() const { return mSize; }

	void clear()
	{
		cleanupNodes();
		init();
	}

	bool empty() const { return !mHeads[0]; }

	T const& front() const { CHECK(!empty()) return mHeads[0]->value; }

	bool find(T const& value) const
	{
		Node* node = walkFindingPath(value, [](int, Node**) {});
		return node && !CmpFunc()(value, node->value);
	}

	void removeIndex(int index, T& outValue)
	{
		if (index < 0 || index >= mSize)
			return;

		auto node = mHeads[0];
		while (index)
		{
			node = node->links[0];
			--index;
		}
		outValue = node->value;
		remove(node);
	}

	bool insert(T const& value)
	{
		Node** levelLinks[MaxLevel + 1];
		Node* node = walkFindingPath(value, [&](int level, Node** levelLink)
		{
			levelLinks[level] = levelLink + level;
		});
		if (node && !CmpFunc()(value, node->value))
			return false;

		int level = genRandLevel();
		if (level > getLevel())
		{
			for (int i = mLevel + 1; i <= level; ++i)
			{
				mHeads[i] = nullptr;
				levelLinks[i] = mHeads + i;
			}
			mLevel = level;
		}

		Node* newNode = new Node(value);
		++mSize;
		newNode->links.resize(level + 1);
		for (int i = 0; i <= level; ++i)
		{
			newNode->links[i] = *levelLinks[i];
			*levelLinks[i] = newNode;
		}
		return true;
	}

	bool remove(T const& value)
	{
		Node** levelLinks[MaxLevel + 1];
		Node* node = walkFindingPath(value, [&](int level, Node** levelLink)
		{
			levelLinks[level] = levelLink + level;
		});

		if ( !(node && !CmpFunc()(value, node->value)) )
			return false;

		for (int i = 0; i < node->links.size(); ++i)
		{
			*levelLinks[i] = node->links[i];
		}
		delete node;
		--mSize;
		return true;
	}

	struct Node
	{
		T value;
		TArray<Node*> links;
		Node(T const& inValue) :value(inValue) {}
	};

	class Iterator
	{
	public:
		typedef T const& reference;
		typedef T const* pointer;

		Iterator(Node* node) :mNode(node) {}

		Iterator  operator++() { Node* node = mNode; mNode = mNode->links[0]; return Iterator(node); }
		Iterator& operator++(int) { mNode = mNode->links[0]; return *this; }

		reference operator*() { return mNode->value; }
		pointer   operator->() { return &mNode->value; }

		bool operator == (Iterator other) const { return mNode == other.mNode; }
		bool operator != (Iterator other) const { return mNode != other.mNode; }
		Node* mNode;
	};

	using iterator = Iterator;
	iterator begin() { return Iterator(mHeads[0]); }
	iterator end()   { return Iterator(nullptr); }

public:

	static int constexpr MaxLevel = 32;
	Node* mHeads[MaxLevel + 1];
	int   mLevel;
	int   mSize;

private:

	void remove(Node* node)
	{
		CHECK(node);
		walkFindingPath(node->value, [&](int level, Node** levelLink)
		{			
			if (level < node->links.size())
			{
				if (level == 0)
				{
					CHECK(levelLink[0] == node);
				}
				levelLink[level] = node->links[level];
			}
		});
		delete node;
		--mSize;
	}

	void init()
	{
		mLevel = 0;
		mHeads[0] = nullptr;
		mSize = 0;
	}
	void cleanupNodes()
	{
		Node* node = mHeads[0];
		while (node)
		{
			Node* next = node->links[0];
			delete node;
			node = next;
		}
	}

	int genRandLevel()
	{
		int level = 0;
#if 0
		while (rand() % 2) { ++level; }
		return Math::Min(MaxLevel, level);
#else
		uint32 value = rand();
		while (value & 0x1) { ++level; value >>= 1; }
		return Math::Min(MaxLevel, level);
#endif
	}

	template< typename TFunc >
	Node* walkFindingPath(T const& value, TFunc& func)
	{
		Node** curLinks = mHeads;
		for (int level = getLevel(); level >= 0; --level)
		{
			Node* curNode;
			while ((curNode = curLinks[level]) && CmpFunc()(curNode->value, value))
			{
				curLinks = curNode->links.data();
			}
			func(level, curLinks);
		}
		return curLinks[0];
	}
};
#endif // SkipList_H_CBDB5698_BC43_432C_8EF9_C4C1CF2F8ED8