#pragma once
#ifndef Guid_H_B04E0FF3_17E5_42B1_B883_3A7949ECB4EA
#define Guid_H_B04E0FF3_17E5_42B1_B883_3A7949ECB4EA

#include "Core/IntegerType.h"

class Guid
{
public:
	static Guid New();

	Guid& operator = (Guid const& rhs) = default;

	bool operator == (Guid const& rhs) const
	{
		return ((mA ^ rhs.mA) | (mB ^ rhs.mB) | (mC ^ rhs.mC) | (mD ^ rhs.mD)) == 0;
	}

	bool operator != (Guid const& rhs) const
	{
		return !operator == (rhs);
	}

	uint32 operator [] (int idx) const { return mData[idx]; }

private:
	union 
	{
		struct
		{
			uint32 mA;
			uint32 mB;
			uint32 mC;
			uint32 mD;
		};

		uint32 mData[4];
	};

};

#endif // Guid_H_B04E0FF3_17E5_42B1_B883_3A7949ECB4EA
