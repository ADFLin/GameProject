#pragma once
#ifndef CRC_H_9A2FE690_0438_484A_8C62_53CF8C6287B9
#define CRC_H_9A2FE690_0438_484A_8C62_53CF8C6287B9


#include "Core/IntegerType.h"


class FCRC
{
public:
	static uint32 Value32( uint8 const* pData , uint32 numData );
};

#endif // CRC_H_9A2FE690_0438_484A_8C62_53CF8C6287B9
