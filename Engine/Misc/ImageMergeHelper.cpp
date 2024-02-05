#include "ImageMergeHelper.h"

ImageMergeHelper::ImageMergeHelper(int w, int h)
{
	init(w, h);
}

ImageMergeHelper::ImageMergeHelper()
{

}

ImageMergeHelper::~ImageMergeHelper()
{
	clear();
}

void ImageMergeHelper::init(int w, int h)
{
	mSize = Vec2i(w, h);
	createNode(0, 0, w, h)->linkHead(mFreeList);
}

void ImageMergeHelper::clear()
{
	destroyList(mUsedList);
	destroyList(mFreeList);
	mImageNodeMap.clear();
	mSize = Vec2i::Zero();
}


bool ImageMergeHelper::addImage(int id, int w, int h)
{
	Node* node = insertNode(w, h);
	if( !node )
		return false;

	if( id >= mImageNodeMap.size() )
		mImageNodeMap.resize(id + 1, nullptr);

	CHECK(node->imageID == ErrorImageID);
	node->imageID = id;
	mImageNodeMap[id] = node;
	return true;
}

int ImageMergeHelper::calcUsageArea() const
{
	int result = 0;
#if 0
	for( Node* node : mImageNodeMap )
	{
		if( node && node->imageID != ErrorImageID )
		{
			result += node->rect.w * node->rect.h;
		}
	}
#else
	for (auto node = mUsedList; node; node = node->mNextLink)
	{
		result += node->rect.w * node->rect.h;
	}
#endif
	return result;
}

ImageMergeHelper::ImageMergeHelper::Node* ImageMergeHelper::insertNode(int w, int h)
{
	Node* usedNode = nullptr;
	for (auto node = mFreeList; node; node = node->mNextLink)
	{
		if (w <= node->rect.w && h <= node->rect.h)
		{
			usedNode = node;
			break;
		}
	}

	if (usedNode)
	{
		auto& rect = usedNode->rect;
		int dw = rect.w - w;
		int dh = rect.h - h;
		Node* left = nullptr;
		Node* right = nullptr;
		if (dh <= dw)
		{
			if (dh > 0)
			{
				left = createNode(rect.x, rect.y + h, w, dh);
			}
			right = createNode(rect.x + w, rect.y, dw, rect.h);
		}
		else
		{
			if (dw > 0)
			{
				left = createNode(rect.x + w, rect.y, dw, h);
			}
			right = createNode(rect.x, rect.y + h, rect.w, dh);
		}

		if (left)
		{
			left->linkReplace(usedNode);
			right->linkAfter(left);
		}
		else
		{
			right->linkReplace(usedNode);
		}

		rect.w = w;
		rect.h = h;
		usedNode->linkHead(mUsedList);
	}

	return usedNode;
}

void ImageMergeHelper::destroyList(Node*& head)
{
	Node* node = head;
	while (node)
	{
		Node* next = node->mNextLink;
		destoryNode(node);
		node = next;
	}
	head = nullptr;
}

ImageMergeHelper::Node* ImageMergeHelper::createNode(int x, int y, int w, int h)
{
	Node* node = new Node;
	node->rect.x = x;
	node->rect.y = y;
	node->rect.w = w;
	node->rect.h = h;
	node->imageID = ErrorImageID;
	return node;
}

void ImageMergeHelper::destoryNode(Node* node)
{
	delete node;
}