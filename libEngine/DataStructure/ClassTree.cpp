#include "ClassTree.h"

#include "MarcoCommon.h"

void ClassTreeNode::offsetQueryIndex(int offset)
{
	indexQuery += offset;

	for( ClassTreeNode* child : children )
	{
		child->offsetQueryIndex(offset);
	}
}

ClassTreeNode::ClassTreeNode(ClassTreeNode* parent)
	:parent(parent)
	,numTotalChildren(0)
	,indexQuery(INDEX_NONE)
#if CLASS_TREE_USE_INTRLIST
#else
	, indexParentSlot(INDEX_NONE)
#endif

{

#if _DEBUG
	static int gIdDbg = 0;
	idDbg = gIdDbg++;
#endif
	ClassTree::Get().registerClass(this);
}

ClassTreeNode::ClassTreeNode(ERootConstruct)
	:parent(nullptr)
	,numTotalChildren(0)
	,indexQuery(INDEX_NONE)
#if CLASS_TREE_USE_INTRLIST
#else
	, indexParentSlot(INDEX_NONE)
#endif

{
#if _DEBUG
	idDbg = -1;
#endif
}

ClassTreeNode::~ClassTreeNode()
{
#if CLASS_TREE_USE_INTRLIST
	if( childHook.isLinked() )
#else
	if (indexParentSlot != -1)
#endif
	{
		ClassTree::Get().unregisterClass(this, false);
	}
}

bool ClassTreeNode::isChildOf(ClassTreeNode* testParent)
{
#if CLASS_TREE_USE_INTRLIST
	assert(childHook.isLinked() && testParent->childHook.isLinked());
#else
	assert(indexParentSlot != INDEX_NONE && testParent->indexParentSlot != INDEX_NONE);
#endif
	return unsigned(indexQuery - testParent->indexQuery) <= testParent->numTotalChildren;
}

void ClassTreeNode::changeParent(ClassTreeNode* newParent)
{
	if( newParent == parent )
		return;

	ClassTree::Get().unregisterClass(this, true);
	parent = newParent;
	ClassTree::Get().registerClass(this);
}

void ClassTree::UnregisterAllNode_R(ClassTreeNode* node)
{
	for( ClassTreeNode* child : node->children )
	{
		UnregisterAllNode_R(child);
	}

#if CLASS_TREE_USE_INTRLIST
	assert(node->children.empty());
	node->childHook.unlink();
#else
	node->children.clear();
	node->indexParentSlot = INDEX_NONE;
#endif
	node->indexQuery = INDEX_NONE;
	node->numTotalChildren = 0;
}

void ClassTree::UpdateChildNumChanged(ClassTreeNode* parent, int numChildrenChanged)
{
	ClassTreeNode* super = parent;

	for( ;;)
	{
		super->numTotalChildren += numChildrenChanged;

		if( super->parent == nullptr )
			break;

		ClassTreeNode* superParent = super->parent;
#if CLASS_TREE_USE_INTRLIST
		auto iter = superParent->children.beginNode(super);
		for( ++iter; iter != superParent->children.end(); ++iter )
		{
			ClassTreeNode* child = *iter;
			child->offsetQueryIndex(numChildrenChanged);
		}
#else
		int indexSlot = super->indexParentSlot + 1;
		for( int i = indexSlot; i < superParent->children.size(); ++i )
		{
			super->children[i]->offsetQueryIndex(numChildrenChanged);
		}
#endif
		super = superParent;
	}
}

bool ClassTree::checkValid()
{
	int numTotalChildren;
	return CheckChildValid_R(&mRoot, numTotalChildren);
}

bool ClassTree::CheckChildValid_R(ClassTreeNode* parent, int& numTotalChildren)
{
	numTotalChildren = 0;

#if CLASS_TREE_USE_INTRLIST
	for( ClassTreeNode* node : parent->children )
	{

#else
	for( int i = 0; i < parent->children.size(); ++i )
	{
		ClassTreeNode* node = parent->children[i];
		if( node->indexParentSlot != i )
			return false;
#endif
		if( node->indexQuery != numTotalChildren + parent->indexQuery + 1 )
			return false;
		int num;
		if( !CheckChildValid_R(node, num) )
			return false;
		numTotalChildren += num + 1;
	}

	if( parent->numTotalChildren != numTotalChildren )
		return false;

	return true;
}

ClassTree::ClassTree()
	:mRoot(ClassTreeNode::RootConstruct)
{

}

ClassTree::~ClassTree()
{
	unregisterAllClass();
}

#if CORE_SHARE_CODE

ClassTree& ClassTree::Get()
{
	static ClassTree sClassTree;
	return sClassTree;
}

void ClassTree::registerClass(ClassTreeNode* node)
{
#if CLASS_TREE_USE_INTRLIST
	assert(!node->childHook.isLinked());
#else
	assert(node->indexParentSlot == INDEX_NONE);
#endif
	if( node->parent == nullptr )
	{
		node->parent = &mRoot;
	}
	ClassTreeNode* parent = node->parent;
	assert(parent != node);

#if CLASS_TREE_USE_INTRLIST
#else
	node->indexParentSlot = parent->children.size();
#endif
	if( node->indexQuery != INDEX_NONE)
	{
		int offset = (parent->indexQuery + parent->numTotalChildren + 1) - node->indexQuery;
		if( offset )
			node->offsetQueryIndex(offset);
	}
	else
	{
		node->indexQuery = parent->indexQuery + parent->numTotalChildren + 1;
	}

	parent->children.push_back(node);
	int numChildren = 1 + node->numTotalChildren;
	UpdateChildNumChanged(parent, numChildren);


	assert(checkValid());
}

void ClassTree::unregisterClass(ClassTreeNode* node, bool bReregister)
{
#if CLASS_TREE_USE_INTRLIST
	assert(node->childHook.isLinked());
#else
	assert(node->indexParentSlot != INDEX_NONE);
#endif
	ClassTreeNode* parent = node->parent;
	assert(parent);

	int numChildren = 1 + node->numTotalChildren;

#if CLASS_TREE_USE_INTRLIST
	auto iter = parent->children.beginNode(node);
	for( ++iter; iter != parent->children.end(); ++iter )
	{
		ClassTreeNode* child = *iter;
		child->offsetQueryIndex(-numChildren);
	}
	node->childHook.unlink();
#else
	for( int i = node->indexParentSlot + 1; i < parent->children.size(); ++i )
	{
		ClassTreeNode* child = parent->children[i];
		child->indexParentSlot -= 1;
		child->offsetQueryIndex(-numChildren);
	}
	parent->children.erase(parent->children.begin() + node->indexParentSlot);
	node->indexParentSlot = INDEX_NONE;
#endif
	UpdateChildNumChanged(parent, -numChildren);

	assert(checkValid());

	if( !bReregister )
	{
#if CLASS_TREE_USE_INTRLIST
		while( !node->children.empty() )
		{
			ClassTreeNode* child = node->children.front();
			child->parent = nullptr;
			child->childHook.unlink();
			registerClass(child);
		}
#else
		for( size_t i = 0; i < node->children.size(); ++i )
		{
			ClassTreeNode* child = node->children[i];
			child->indexParentSlot = INDEX_NONE;
			child->parent = nullptr;
			registerClass(child);
		}
		node->children.clear();
#endif
	}
}

void ClassTree::unregisterAllClass()
{
	for( ClassTreeNode* child : mRoot.children )
	{
		UnregisterAllNode_R(child);
	}
#if CLASS_TREE_USE_INTRLIST
	assert(mRoot.children.empty());
#else
	mRoot.children.clear();
#endif
	mRoot.numTotalChildren = 0;
}
#endif //CORE_SHARE_CODE