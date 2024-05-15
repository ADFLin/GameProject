#pragma once
#ifndef Format_H_4D4CD3DB_1D41_49BD_892F_9E58D55516C9
#define Format_H_4D4CD3DB_1D41_49BD_892F_9E58D55516C9

#include "Template/ArrayView.h"
#include "CString.h"
#include "StringParse.h"

namespace Text
{
	template< typename CharT >
	using TStdString = typename TStringTraits< CharT >::StdString;

	template< typename CharT >
	static int Format(CharT const* format, TArrayView< TStdString< CharT > const > const& strList, TStdString< CharT >& outString)
	{
		int index = 0;
		char const* cur = format;
		if (strList.size() > 0)
		{
			while (*cur != 0)
			{
				char const* next = FStringParse::FindChar(cur, '%');
				if (next[0] == '%' && next[1] == 's')
				{
					CHECK(cur != next);

					outString.append(cur, next - cur);
					cur = next + 2;

					outString += strList[index];
					++index;

					if (cur[0] == '\n')
					{
						cur += 1;
					}
					else if (cur[0] == '\r' && cur[1] == '\n')
					{
						cur += 2;
					}

					if (!strList.isValidIndex(index))
					{
						break;
					}
				}
				else
				{
					break;
				}
			}
		}

		if (*cur != 0)
		{
			outString.append(cur);
		}
		return index;
	}
}

#endif // Format_H_4D4CD3DB_1D41_49BD_892F_9E58D55516C9
