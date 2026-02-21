#ifndef SurvivorsItem_h__
#define SurvivorsItem_h__

#include "SurvivorsCommon.h"
#include <string>

namespace Survivors
{
	class Hero;

	class ItemBase
	{
	public:
		virtual ~ItemBase() {}
		virtual void applyEffect(Hero& hero) = 0;
		virtual std::string getName() const = 0;
		virtual std::string getDescription() const = 0;

		int mLevel = 1;
		int mMaxLevel = 5;
	};

}//namespace Survivors

#endif // SurvivorsItem_h__
