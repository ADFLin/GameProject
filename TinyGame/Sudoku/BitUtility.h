#ifndef BitUtility_h__
#define BitUtility_h__


typedef unsigned __int32 uint32;
class BitUtility
{
public:
	static int toNumber( uint32 n )
	{
		int result = 0;
		if ( n & 0xffff0000 ){ result += 16 ; n >>= 16; }
		if ( n & 0x0000ff00 ){ result +=  8 ; n >>=  8; }
		if ( n & 0x000000f0 ){ result +=  4 ; n >>=  4; }
		if ( n & 0x0000000c ){ result +=  2 ; n >>=  2; }
		if ( n & 0x00000002 ){ result +=  1 ; n >>=  1; }
		return result + (n & 0x00000001);
	}
	static int count( uint32 x )
	{
		int count = 0;
		for( ; x ; ++count )
			x &= x - 1; 
		return count;
	}
};

#endif // BitUtility_h__
