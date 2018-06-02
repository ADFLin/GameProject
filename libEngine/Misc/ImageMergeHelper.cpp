#include "ImageMergeHelper.h"

#include <cassert>


ImageMergeHelper::Node* ImageMergeHelper::createNode(int x, int y, int w, int h)
{
	Node* node = new Node;
	node->children[0] = node->children[1] = nullptr;
	node->rect.x = x;
	node->rect.y = y;
	node->rect.w = w;
	node->rect.h = h;
	node->imageID = ErrorImageID;
	return node;
}

ImageMergeHelper::ImageMergeHelper(int w, int h)
{
	mRoot = createNode(0, 0, w, h);
}

ImageMergeHelper::ImageMergeHelper()
{
	mRoot = nullptr;
}

ImageMergeHelper::~ImageMergeHelper()
{
	if( mRoot )
	{
		destoryNode(mRoot);
	}
	
}

void ImageMergeHelper::init(int w, int h)
{
	assert(mRoot == nullptr);
	mRoot = createNode(0,0,w, h);
}

void ImageMergeHelper::destoryNode(Node* node)
{
	if( node->children[0] )
		destoryNode(node->children[0]);
	if( node->children[1] )
		destoryNode(node->children[1]);
	delete node;
}

bool ImageMergeHelper::addImage(int id, int w, int h)
{
	mRect.w = w;
	mRect.h = h;
	mRect.x = 0;
	mRect.y = 0;

	Node* node = insertNode(mRoot);
	if( !node )
		return false;

	if( id >= mImageNodeMap.size() )
		mImageNodeMap.resize(id + 1, nullptr);

	assert(node->imageID == ErrorImageID);
	node->imageID = id;
	mImageNodeMap[id] = node;
	return true;
}

ImageMergeHelper::Node* ImageMergeHelper::insertNode(Node* curNode)
{
	assert(curNode);

	if( curNode->imageID != ErrorImageID )
		return nullptr;

	if( !curNode->isLeaf() )
	{
		Node* newNode = insertNode(curNode->children[0]);
		if( !newNode )
			newNode = insertNode(curNode->children[1]);
		return newNode;
	}

	Rect& curRect = curNode->rect;

	if( mRect.w == curRect.w && mRect.h == curRect.h )
		return curNode;

	if( mRect.w > curRect.w || mRect.h > curRect.h )
		return nullptr;

	int dw = curRect.w - mRect.w;
	int dh = curRect.h - mRect.h;

	if( dw > dh )
	{
		curNode->children[0] = createNode(curRect.x, curRect.y, mRect.w, curRect.h);
		curNode->children[1] = createNode(curRect.x + mRect.w, curRect.y, dw, curRect.h);
	}
	else
	{
		curNode->children[0] = createNode(curRect.x, curRect.y, curRect.w, mRect.h);
		curNode->children[1] = createNode(curRect.x, curRect.y + mRect.h, curRect.w, dh);
	}

	return insertNode(curNode->children[0]);
}

