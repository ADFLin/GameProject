#pragma once
#ifndef TFWRUnlock_H_178F6A31_0116_4293_B302_EF68C1305FC7
#define TFWRUnlock_H_178F6A31_0116_4293_B302_EF68C1305FC7

#include "DataStructure/Array.h"

namespace TFWR
{
	struct FeatureNode
	{
		Vec2i pos;
		FeatureNode* prerequisite;
		TArray<FeatureNode*> subsequents;
	};

}//namespace TFWR

#endif // TFWRUnlock_H_178F6A31_0116_4293_B302_EF68C1305FC7