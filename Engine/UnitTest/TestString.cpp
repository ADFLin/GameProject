#include "UnitTest.h"
#include "Core/String.h"
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <sstream>

// Forward declaration if not available strictly via headers (assuming LogMsg exists as per TestAssert.h assumption)
// In a real scenario, we might include "LogSystem.h" or similar.

class StringTest
{
public:
	static RunResult run()
	{
		// 1. Small String
		{
			FbString str = "Small";
			UT_ASSERT_EQ(str.length(), 5);
			UT_ASSERT_EQ(str.category(), FbString::Small);
			UT_ASSERT(FCString::Compare(str.c_str(), "Small") == 0);
			
			str = "Change";
			UT_ASSERT_EQ(str.length(), 6);
			UT_ASSERT(FCString::Compare(str.c_str(), "Change") == 0);
		}

		// 2. Medium String
		{
			const char* mediumText = "This is a string that is definitely longer than 23 characters but shorter than 255 characters to test Medium category.";
			FbString str = mediumText;
			UT_ASSERT_EQ(str.category(), FbString::Middle);
			UT_ASSERT_EQ(str.length(), FCString::Strlen(mediumText));
			UT_ASSERT(FCString::Compare(str.c_str(), mediumText) == 0);

			FbString copyStr = str;
			UT_ASSERT_EQ(copyStr.category(), FbString::Middle);
			UT_ASSERT_EQ(copyStr.length(), str.length());
			// Deep Copy checking
			UT_ASSERT_NE(copyStr.data(), str.data());
		}

		// 3. Large String (> 255) with CoW
		{
			FbString str;
			str.reserve(300);
			for(int i=0; i<260; ++i) str += 'A';
			
			UT_ASSERT_EQ(str.category(), FbString::Large);
			UT_ASSERT_EQ(str.length(), 260);
			
			// Test CoW
			FbString copyStr = str;
			UT_ASSERT_EQ(copyStr.category(), FbString::Large);
			UT_ASSERT_EQ((void*)copyStr.c_str(), (void*)str.c_str()); // Shared buffer check
			
			// Modify copy - should trigger Unshare
			copyStr += 'B';
			UT_ASSERT_EQ(copyStr.length(), 261);
			UT_ASSERT_EQ(str.length(), 260); 
			UT_ASSERT_NE((void*)copyStr.c_str(), (void*)str.c_str()); 
		}

		// 4. Operators & Self-Append
		{
			FbString s1 = "Hello";
			FbString s2 = " World";
			FbString s3 = s1 + s2;
			UT_ASSERT(FCString::Compare(s3.c_str(), "Hello World") == 0);
			
			s1 += s2;
			UT_ASSERT(FCString::Compare(s1.c_str(), "Hello World") == 0);
			
			FbString s4 = s1 + "!";
			UT_ASSERT(FCString::Compare(s4.c_str(), "Hello World!") == 0);

			// Self-append
			s1 += s1;
			UT_ASSERT(FCString::Compare(s1.c_str(), "Hello WorldHello World") == 0);
		}

		// 5. Edge Cases & Move
		{
			// Empty
			FbString empty;
			UT_ASSERT_EQ(empty.length(), 0);
			UT_ASSERT(empty.empty());
			UT_ASSERT_EQ(empty.category(), FbString::Small);
			
			// Clear
			FbString s = "Content";
			UT_ASSERT(!s.empty());
			s.clear();
			UT_ASSERT_EQ(s.length(), 0);
			UT_ASSERT(s.empty());
			
			// Move
			FbString source = "MoveMe";
			FbString dest = std::move(source);
			UT_ASSERT(FCString::Compare(dest.c_str(), "MoveMe") == 0);
			UT_ASSERT_EQ(source.length(), 0);
			
			// Self Assignment
			dest = dest;
			UT_ASSERT(FCString::Compare(dest.c_str(), "MoveMe") == 0);
		}


		// 6. STL Compatibility
		{
			// std::set
			std::set<FbString> stringSet;
			stringSet.insert("Alpha");
			stringSet.insert("Beta");
			stringSet.insert("Alpha"); // Duplicate
			
			UT_ASSERT_EQ(stringSet.size(), 2);
			UT_ASSERT(stringSet.find("Alpha") != stringSet.end());
			UT_ASSERT(stringSet.find("Gamma") == stringSet.end());
			
			// std::map
			std::map<FbString, int> stringMap;
			stringMap["One"] = 1;
			stringMap["Two"] = 2;
			
			UT_ASSERT_EQ(stringMap["One"], 1);
			UT_ASSERT_EQ(stringMap["Two"], 2);
			
			// std::unordered_set
			std::unordered_set<FbString> hashSet;
			hashSet.insert("Hash");
			hashSet.insert("Table");
			hashSet.insert("Hash");
			
			UT_ASSERT_EQ(hashSet.size(), 2);
			UT_ASSERT(hashSet.find("Table") != hashSet.end());
			
			// std::unordered_map
			std::unordered_map<FbString, FbString> hashMap;
			hashMap["Key"] = "Value";
			hashMap["LongKeyToCheckHash"] = "LongValue"; // Medium/Large string potentially
			
			UT_ASSERT(FCString::Compare(hashMap["Key"].c_str(), "Value") == 0);
			UT_ASSERT(FCString::Compare(hashMap["LongKeyToCheckHash"].c_str(), "LongValue") == 0);
		}


		// 7. IO Stream Support
		{
			// Output
			FbString str = "StreamTest";
			std::stringstream ss;
			ss << str;
			UT_ASSERT_EQ(str.length(), 10);
			UT_ASSERT(ss.str() == "StreamTest");

			// Input
			FbString inputStr;
			ss >> inputStr;
			UT_ASSERT_EQ(inputStr.length(), 10);
			UT_ASSERT(FCString::Compare(inputStr.c_str(), "StreamTest") == 0);
			
			// Input multiple words
			std::stringstream ss2("Hello World");
			FbString s1, s2;
			ss2 >> s1 >> s2;
			UT_ASSERT(FCString::Compare(s1.c_str(), "Hello") == 0);
			UT_ASSERT(FCString::Compare(s2.c_str(), "World") == 0);
		}


		// 8. Interoperability (std::string, TStringView)
		{
			std::string stdStr = "StdString";
			
			// Constructor from std::string
			FbString s1 = stdStr;
			UT_ASSERT_EQ(s1.length(), 9);
			UT_ASSERT(FCString::Compare(s1.c_str(), "StdString") == 0);
			
			// Assignment from std::string
			FbString s2;
			s2 = stdStr;
			UT_ASSERT_EQ(s2.length(), 9);
			UT_ASSERT(FCString::Compare(s2.c_str(), "StdString") == 0);
			
			// Constructor from StringView
			StringView view = "StringView";
			FbString s3 = view;
			UT_ASSERT_EQ(s3.length(), 10);
			UT_ASSERT(FCString::Compare(s3.c_str(), "StringView") == 0);
			
			// Assignment from StringView
			FbString s4;
			s4 = view;
			UT_ASSERT_EQ(s4.length(), 10);
			UT_ASSERT(FCString::Compare(s4.c_str(), "StringView") == 0);
			
			// Conversion to StringView
			FbString s5 = "ConvertToView";
			StringView view2 = s5;
			UT_ASSERT_EQ(view2.length(), 13);
			// Manual compare characters
			UT_ASSERT(view2 == "ConvertToView");
		}

		return RR_SUCCESS;
	}
};

UT_REG_TEST(StringTest, "Core.String")
