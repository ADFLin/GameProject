#pragma once

#ifndef CommandlLine_H_574B8289_5DB5_4EC2_A1B3_BFDE2E563AF9
#define CommandlLine_H_574B8289_5DB5_4EC2_A1B3_BFDE2E563AF9

#include "CoreShare.h"
#include "CString.h"

class FCommandLine
{
public:
	CORE_API static void Initialize(TChar const* initStr);
	CORE_API static TChar const*  Get();
	CORE_API static TChar const** GetArgs(int& outNumArg);

	static int Parse(TChar const* content, TChar buffer[], int bufferLen, TChar const* outArgs[], int maxNumArgs);
};


#endif // CommandlLine_H_574B8289_5DB5_4EC2_A1B3_BFDE2E563AF9
