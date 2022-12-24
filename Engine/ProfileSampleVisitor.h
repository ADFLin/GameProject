#pragma once
#ifndef ProfileSampleVisitor_H_36B36769_BE47_4E96_B0F8_62F7B221D4CF
#define ProfileSampleVisitor_H_36B36769_BE47_4E96_B0F8_62F7B221D4CF
#include "ProfileSystem.h"

template < typename T >
class ProfileNodeVisitorT
{
	T* _this() { return static_cast<T*>(this); }
public:

	using SampleNode = ProfileSampleNode;

	struct VisitContext
	{
		SampleNode* node;
		double      parentTime;
		double      parentFrameTime;
		double      accTime = 0.0;
		double      accFrameTime = 0.0;

		int         indexNode = 0;
	};

	void onRoot(SampleNode* node) {}
	void onNode(VisitContext const& context) {}
	bool onEnterChild(VisitContext const& context) { return true; }
	void onReturnParent(VisitContext const& context, VisitContext const& childContext) {}

	void visitNodes(uint32 threadId = 0)
	{
		SampleNode* root = ProfileSystem::Get().getRootSample(threadId);
		_this()->onRoot(root);

		VisitContext context;
		context.node = root;
		VisitContext childContext;
		childContext.parentTime = ProfileSystem::Get().getDurationSinceReset();
		childContext.parentFrameTime = ProfileSystem::Get().getLastFrameDuration();

		SampleNode* node = root->getChild();
		for (; node; node = node->getSibling())
		{
			childContext.node = node;
			childContext.accTime += node->getTotalTime();
			childContext.accFrameTime += node->getLastFrameTime();
			visitRecursive(childContext);

			++childContext.indexNode;
		}

		_this()->onReturnParent(context, childContext);
	}

	void visitRecursive(VisitContext const& context)
	{
		_this()->onNode(context);

		if (!_this()->onEnterChild(context))
			return;

		VisitContext childContext;
		childContext.indexNode = 0;
		childContext.parentTime = context.node->getTotalTime();
		childContext.parentFrameTime = context.node->getLastFrameTime();

		SampleNode* node = context.node->getChild();
		for (; node; node = node->getSibling())
		{
			childContext.node = node;

			childContext.accTime += node->getTotalTime();
			childContext.accFrameTime += node->getLastFrameTime();

			visitRecursive(childContext);
			++childContext.indexNode;
		}

		_this()->onReturnParent(context, childContext);
	}
};

#endif // ProfileSampleVisitor_H_36B36769_BE47_4E96_B0F8_62F7B221D4CF
