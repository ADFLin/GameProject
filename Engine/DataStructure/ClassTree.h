#pragma once
#ifndef ClassTree_H_4D9C2491_82E5_426E_8302_6C75E7700E89
#define ClassTree_H_4D9C2491_82E5_426E_8302_6C75E7700E89

#include "CoreShare.h"

#define CLASS_TREE_USE_INTRLIST 1

#if CLASS_TREE_USE_INTRLIST
#include "DataStructure/IntrList.h"
class ClassTreeLink
{
public:
	LinkHook childHook;
};
#else
#include <vector>
#endif

class ClassTree;
class ClassTreeNode
#if CLASS_TREE_USE_INTRLIST
	: public ClassTreeLink
#endif
{
public:
	ClassTreeNode(ClassTree& tree, ClassTreeNode* parent);
	~ClassTreeNode();

	bool isChildOf(ClassTreeNode* testParent) const;
	void changeParent(ClassTreeNode* newParent);

private:
	void offsetQueryIndex(int offset);

	ClassTreeNode* parent;
	ClassTree*  mTree;
	int      indexQuery;
	unsigned numTotalChildren;
	
#if CLASS_TREE_USE_INTRLIST
	typedef TIntrList< ClassTreeNode, MemberHook< ClassTreeLink, &ClassTreeLink::childHook >, PointerType > NodeList;
	NodeList children;
#else
	int  indexParentSlot;
	std::vector< ClassTreeNode* > children;
#endif

#if _DEBUG
	int  idDbg;
#endif
	enum ERootConstruct
	{
		RootConstruct,
	};
	ClassTreeNode(ERootConstruct);
	friend class ClassTree;
};

class ClassTree
{
public:
	ClassTree();
	~ClassTree();

	void registerClass(ClassTreeNode* node);
	void unregisterClass(ClassTreeNode* node, bool bReregister);
	void unregisterAllClass();

private:
	static void UnregisterAllNode_R(ClassTreeNode* node);

	static void UpdateChildNumChanged(ClassTreeNode* parent, int numChildrenChanged);

	bool checkValid();
	static bool CheckChildValid_R(ClassTreeNode* parent, int& numTotalChildren);

	ClassTreeNode mRoot;
};

#endif // ClassTree_H_4D9C2491_82E5_426E_8302_6C75E7700E89