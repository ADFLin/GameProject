#pragma once
#ifndef ImageMergeHelper_H_3A8549D5_BFD9_4866_A2D4_AED1F1AF7AA8
#define ImageMergeHelper_H_3A8549D5_BFD9_4866_A2D4_AED1F1AF7AA8

#include <vector>

class ImageMergeHelper
{
public:

	static int const ErrorImageID = -1;

	ImageMergeHelper(int w, int h);
	ImageMergeHelper();

	~ImageMergeHelper();

	void init(int w, int h);
	void clear();


	struct Rect
	{
		int x, y;
		int w, h;
	};

	struct Node
	{
		Node* children[2];
		Rect  rect;
		int   imageID;
		bool isLeaf() { return children[0] == nullptr; }
	};

	bool  addImage(int id, int w, int h);
	Node const* getNode(int id) const { return mImageNodeMap[id]; }

protected:

	typedef std::vector< Node* > NodeMap;
	NodeMap mImageNodeMap;
	Node*   mRoot;

private:
	Rect  mRect;
	Node* insertNode(Node* curNode);
	void  destoryNode(Node* node);
	Node* createNode(int x, int y, int w, int h);
};

#endif // ImageMergeHelper_H_3A8549D5_BFD9_4866_A2D4_AED1F1AF7AA8
