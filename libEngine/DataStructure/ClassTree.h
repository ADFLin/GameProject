#pragma once
#ifndef ClassTree_H_4D9C2491_82E5_426E_8302_6C75E7700E89
#define ClassTree_H_4D9C2491_82E5_426E_8302_6C75E7700E89

#include "CoreShare.h"

#define CLASS_TREE_USE_INTRLIST 1

#if CLASS_TREE_USE_INTRLIST
#include "DataStructure/IntrList.h"
class ClassTreeLink
{
protected:
	LinkHook childHook;
};
#else
#include <vector>
#endif

class ClassTreeNode
#if CLASS_TREE_USE_INTRLIST
	: public ClassTreeLink
#endif
{
public:
	ClassTreeNode(ClassTreeNode* parent);
	~ClassTreeNode();

	bool isChildOf(ClassTreeNode* testParent);
	void changeParent(ClassTreeNode* newParent);

private:
	void offsetQueryIndex(int offset);

	ClassTreeNode* parent;
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
	CORE_API static ClassTree& Get();
	CORE_API void registerClass(ClassTreeNode* node);
	CORE_API void unregisterClass(ClassTreeNode* node, bool bReregister);
	CORE_API void unregisterAllClass();

private:
	static void UnregisterAllNode_R(ClassTreeNode* node);

	static void UpdateChildNumChanged(ClassTreeNode* parent, int numChildrenChanged);

	bool checkValid();
	static bool CheckChildValid_R(ClassTreeNode* parent, int& numTotalChildren);

	ClassTree();
	ClassTree(ClassTree const&);
	~ClassTree();
	ClassTreeNode mRoot;
};

#endif // ClassTree_H_4D9C2491_82E5_426E_8302_6C75E7700E89