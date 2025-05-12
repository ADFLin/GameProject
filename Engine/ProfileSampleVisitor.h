#pragma once
#ifndef ProfileSampleVisitor_H_36B36769_BE47_4E96_B0F8_62F7B221D4CF
#define ProfileSampleVisitor_H_36B36769_BE47_4E96_B0F8_62F7B221D4CF
#include "ProfileSystem.h"


struct EmptyData{};

template < typename T , typename ContextExtraData = EmptyData >
class ProfileNodeVisitorT
{
	T* _this() { return static_cast<T*>(this); }
public:

	using SampleNode = ProfileSampleNode;

	struct VisitContext : ContextExtraData
	{
		SampleNode* node;
		double      timeTotalParent;
		double      displayTimeParent;
		double      displayTime;

		double      totalTimeAcc = 0.0;
		double      displayTimeAcc = 0.0;
		int         indexNode = 0;
	};

	void onRoot(VisitContext& context) {}
	void onNode(VisitContext& context) {}
	void onLeaveNode(VisitContext& context) {}
	bool onEnterChild(VisitContext const& context) { return true; }
	void onReturnParent(VisitContext const& context, VisitContext const& childContext) {}
	bool filterNode(SampleNode* node)
	{
		return node->getLastFrame() + 1 == ProfileSystem::Get().getFrameCountSinceReset();
	}

	void visitNodes(uint32 threadId = 0)
	{
		ProfileSystem::Get().readLock();
		SampleNode* root = ProfileSystem::Get().getRootSample(threadId);

		VisitContext context;
		context.node = root;
		context.displayTime = root->getFrameExecTime();
		_this()->onRoot(context);

		visitChildren(context);
		ProfileSystem::Get().readUnlock();
	}

	void visitChildren(VisitContext const& context)
	{
		VisitContext childContext;
		childContext.timeTotalParent = context.node->getTotalTime();
		childContext.displayTimeParent = context.displayTime;

		SampleNode* node = context.node->getChild();
		for (; node; node = node->getSibling())
		{
			if ( !_this()->filterNode(node) )
				continue;

			childContext.node = node;
			childContext.displayTime = node->getFrameExecTime();
			visitRecursive(childContext);

			childContext.totalTimeAcc += node->getTotalTime();
			childContext.displayTimeAcc += childContext.displayTime;

			++childContext.indexNode;
		}

		_this()->onReturnParent(context, childContext);
	}

	void visitRecursive(VisitContext& context)
	{
		_this()->onNode(context);

		if (_this()->onEnterChild(context))
			visitChildren(context);

		_this()->onLeaveNode(context);
	}

};

#endif // ProfileSampleVisitor_H_36B36769_BE47_4E96_B0F8_62F7B221D4CF
