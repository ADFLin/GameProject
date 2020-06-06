#include "Stage/TestStageHeader.h"

#include "MarcoCommon.h"
#include "FileSystem.h"
#include "StringParse.h"
#include <sstream>
#include "CPreprocessor.h"

#define AA  1
#define BB  1
#define AABB 1
#define CC CC
#define DD BB

void PreprocessorTest()
{
	using namespace CPP;
	Preprocessor preprocessor;
	preprocessor.addSreachDir("Test");
	CodeSource source;
	char const* filePath = "Test/PTest.h";
	source.loadFile(filePath);
	source.filePath = filePath;

	std::stringstream oss;
	CPP::CodeOutput codeOutput(oss);

	preprocessor.setOutput(codeOutput);

	try
	{
		preprocessor.translate(source);
	}
	catch (std::exception& e)
	{
		LogWarning(0, "PreprocessorTest fial :%s", e.what());
		throw;
	}
	std::string code;
	code += "\n";
	code.append(std::istreambuf_iterator< char >(oss), std::istreambuf_iterator< char >());
	LogMsg(code.data());

}

REGISTER_MISC_TEST_INNER("PreprocessorTest", PreprocessorTest);