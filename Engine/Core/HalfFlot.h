#pragma once
#ifndef HalfFlot_H_741C5060_F3B6_4780_9AB7_D487318E993F
#define HalfFlot_H_741C5060_F3B6_4780_9AB7_D487318E993F

#include "Core/IntegerType.h"

struct HalfFloat
{
	HalfFloat() = default;

	HalfFloat(float value)
	{
		init(value);
	}

	HalfFloat& operator = ( float value )
	{
		init(value);
		return *this;
	}

	void init(float value)
	{
		union IEEESingle
		{
			float Float;
			struct
			{
				uint32 frac : 23;
				uint32 exp : 8;
				uint32 sign : 1;
			};
		};

		IEEESingle f;
		f.Float = value;

		mSign = f.sign;

		if (!f.exp)
		{
			mFrac = 0;
			mExp = 0;
		}
		else if (f.exp == 0xff)
		{
			// NaN or INF
			mFrac = (f.frac != 0) ? 1 : 0;
			mExp = 31;
		}
		else
		{
			// regular number
			int new_exp = f.exp - 127;

			if (new_exp < -24)
			{
				// this maps to 0
				mFrac = 0;
				mExp = 0;
			}
			else if (new_exp < -14)
			{
				// this maps to a denorm
				mExp = 0;
				unsigned int exp_val = (unsigned int)(-14 - new_exp);  // 2^-exp_val
				switch (exp_val)
				{
				case 0:	mFrac = 0; break;
				case 1: mFrac = 512 + (f.frac >> 14); break;
				case 2: mFrac = 256 + (f.frac >> 15); break;
				case 3: mFrac = 128 + (f.frac >> 16); break;
				case 4: mFrac = 64 + (f.frac >> 17); break;
				case 5: mFrac = 32 + (f.frac >> 18); break;
				case 6: mFrac = 16 + (f.frac >> 19); break;
				case 7: mFrac = 8 + (f.frac >> 20); break;
				case 8: mFrac = 4 + (f.frac >> 21); break;
				case 9: mFrac = 2 + (f.frac >> 22); break;
				case 10: mFrac = 1; break;
				}
			}
			else if (new_exp > 15)
			{
				// map this value to infinity
				mFrac = 0;
				mExp = 31;
			}
			else
			{
				mExp = new_exp + 15;
				mFrac = (f.frac >> 13);
			}
		}
	}

	union
	{
		uint16 mValue;			// All bits
		struct
		{
			uint16 mFrac : 10;	// mantissa
			uint16 mExp  : 5;   // exponent
			uint16 mSign : 1;   // sign
		};
	};


};
#endif // HalfFlot_H_741C5060_F3B6_4780_9AB7_D487318E993F
