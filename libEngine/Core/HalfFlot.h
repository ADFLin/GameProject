#pragma once
#ifndef HalfFlot_H_741C5060_F3B6_4780_9AB7_D487318E993F
#define HalfFlot_H_741C5060_F3B6_4780_9AB7_D487318E993F

#include "Core/IntegerType.h"

struct HalfFloat
{
	HalfFloat() = default;

	HalfFloat(float value)
	{
		union IEEESingle
		{
			float Float;
			struct
			{
				uint32 Frac : 23;
				uint32 Exp : 8;
				uint32 Sign : 1;
			} IEEE;
		};

		IEEESingle f;
		f.Float = value;

		IEEE.Sign = f.IEEE.Sign;

		if (!f.IEEE.Exp)
		{
			IEEE.Frac = 0;
			IEEE.Exp = 0;
		}
		else if (f.IEEE.Exp == 0xff)
		{
			// NaN or INF
			IEEE.Frac = (f.IEEE.Frac != 0) ? 1 : 0;
			IEEE.Exp = 31;
		}
		else
		{
			// regular number
			int new_exp = f.IEEE.Exp - 127;

			if (new_exp < -24)
			{ // this maps to 0
				IEEE.Frac = 0;
				IEEE.Exp = 0;
			}

			else if (new_exp < -14)
			{
				// this maps to a denorm
				IEEE.Exp = 0;
				unsigned int exp_val = (unsigned int)(-14 - new_exp);  // 2^-exp_val
				switch (exp_val)
				{
				case 0:
					IEEE.Frac = 0;
					break;
				case 1: IEEE.Frac = 512 + (f.IEEE.Frac >> 14); break;
				case 2: IEEE.Frac = 256 + (f.IEEE.Frac >> 15); break;
				case 3: IEEE.Frac = 128 + (f.IEEE.Frac >> 16); break;
				case 4: IEEE.Frac = 64 + (f.IEEE.Frac >> 17); break;
				case 5: IEEE.Frac = 32 + (f.IEEE.Frac >> 18); break;
				case 6: IEEE.Frac = 16 + (f.IEEE.Frac >> 19); break;
				case 7: IEEE.Frac = 8 + (f.IEEE.Frac >> 20); break;
				case 8: IEEE.Frac = 4 + (f.IEEE.Frac >> 21); break;
				case 9: IEEE.Frac = 2 + (f.IEEE.Frac >> 22); break;
				case 10: IEEE.Frac = 1; break;
				}
			}
			else if (new_exp > 15)
			{ // map this value to infinity
				IEEE.Frac = 0;
				IEEE.Exp = 31;
			}
			else
			{
				IEEE.Exp = new_exp + 15;
				IEEE.Frac = (f.IEEE.Frac >> 13);
			}
		}
	}


	union
	{
		uint16 bits;			// All bits
		struct
		{
			uint16 Frac : 10;	// mantissa
			uint16 Exp : 5;		// exponent
			uint16 Sign : 1;		// sign
		} IEEE;
	};


};
#endif // HalfFlot_H_741C5060_F3B6_4780_9AB7_D487318E993F
