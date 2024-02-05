#pragma once
#ifndef ImageMergeHelper_H_3A8549D5_BFD9_4866_A2D4_AED1F1AF7AA8
#define ImageMergeHelper_H_3A8549D5_BFD9_4866_A2D4_AED1F1AF7AA8

#include "DataStructure/Array.h"
#include "DataStructure/IntrList.h"
#include "Math/TVector2.h"

#define USE_IMAGE_MERGE_NEW 1

template< typename T >
struct TIntrLinkNode
{
	T*  mNextLink = nullptr;
	T** mPrevLink = nullptr;

	void linkAfter(T* node)
	{
		mPrevLink = &node->mNextLink;
		mNextLink = *mPrevLink;

		*mPrevLink = (T*)this;
		if (mNextLink)
		{
			mNextLink->mPrevLink = &mNextLink;
		}
	}
	void linkReplace(T* node)
	{
		T**& replacePrev = node->mPrevLink;
		T*& replaceNext = node->mNextLink;

		mPrevLink = replacePrev;
		mNextLink = replaceNext;

		if (mPrevLink)
		{
			*mPrevLink = (T*)this;
		}

		if (mNextLink)
		{
			mNextLink->mPrevLink = &mNextLink;
		}

		replacePrev = nullptr;
		replaceNext = nullptr;
	}

	void linkHead(T*& node)
	{
		if (node)
		{
			node->mPrevLink = &mNextLink;
		}

		mNextLink = node;
		mPrevLink = &node;
		node = (T*)this;
	}

};


class ImageMergeHelper
{
public:

	static int const ErrorImageID = -1;

	ImageMergeHelper(int w, int h);
	ImageMergeHelper();

	~ImageMergeHelper();

	void init(int w, int h);
	void clear();

	using Vec2i = TVector2<int>;
	struct Rect
	{
		int x, y;
		int w, h;
	};

	struct Node : public TIntrLinkNode<Node>
	{
		Rect  rect;
		int   imageID;
	};

	bool  isValid(int id) const { return mImageNodeMap.isValidIndex(id); }
	bool  addImage(int id, int w, int h);
	Node const* getNode(int id) const 
	{
		if (!mImageNodeMap.isValidIndex(id))
			return nullptr;
		return mImageNodeMap[id]; 
	}
	Node const* getNodeChecked(int id) const { return mImageNodeMap[id]; }
	int   getTotalArea() const { return mSize.x * mSize.y; }
	int   calcUsageArea() const;

	Vec2i const& getSize() const
	{
		return mSize;
	}
protected:

	typedef TArray< Node* > NodeMap;
	NodeMap mImageNodeMap;
	Vec2i   mSize;
	Rect    mRootRect;

private:

	Node* insertNode(int w, int h);
	void destroyList(Node*& head);
	Node* mUsedList = nullptr;
	Node* mFreeList = nullptr;

	void  destoryNode(Node* node);
	Node* createNode(int x, int y, int w, int h);
};

#endif // ImageMergeHelper_H_3A8549D5_BFD9_4866_A2D4_AED1F1AF7AA8
