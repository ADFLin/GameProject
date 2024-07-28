#include "Stage/TestStageHeader.h"

#include "MarcoCommon.h"
#include "FileSystem.h"
#include "StringParse.h"
#include <sstream>
#include "CPreprocessor.h"

#define ENTRY_AA 1
#define CONCAT(A,B) A##B
#define ENTRY(NAME) CONCAT(ENTRY_,NAME)

#define AAA 0
#define BB AAA
#define AAA 1
#if BB
//#error
#endif

#if ENTRY( AA )

#endif

#define MM(A,B)

MM(1 2,3)

void PreprocessorTest()
{
	char d[] = "aaa";
	InlineString<4> s(d);

	LogMsg("%d", ENTRY(AA));
	using namespace CPP;
	Preprocessor preprocessor;
	preprocessor.bReplaceMarcoText = true;
	preprocessor.addSreachDir("Test");
	CodeBufferSource source;
	char const* filePath = "Test/PTest.h";
	source.appendFile(filePath);
	source.filePath = filePath;

	std::stringstream oss;
	CPP::CodeOutput codeOutput(oss);

	preprocessor.setOutput(codeOutput);

	try
	{
		preprocessor.translate(source);

		std::string code;
		code += "\n";
		code.append(std::istreambuf_iterator< char >(oss), std::istreambuf_iterator< char >());
		LogMsg(code.data());
	}
	catch (std::exception& e)
	{
		LogWarning(0, "PreprocessorTest fial :%s", e.what());
	}
}

REGISTER_MISC_TEST_ENTRY("PreprocessorTest", PreprocessorTest);