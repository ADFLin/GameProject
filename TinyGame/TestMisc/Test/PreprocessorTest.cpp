#include "Stage/TestStageHeader.h"

#include "MarcoCommon.h"
#include "FileSystem.h"
#include "StringParse.h"
#include <sstream>
#include "CPreprocessor.h"

void PreprocessorTest()
{
	using namespace CPP;
	Preprocessor preprocessor;
	preprocessor.addSreachDir("Test");
	CodeSource source;
	source.loadFile("Test/PTest.h");

	std::stringstream oss;
	CPP::CodeOutput codeOutput(oss);
	preprocessor.setOutput(codeOutput);

	preprocessor.translate(source);

	std::vector<char> codeBuffer;
	codeBuffer.assign(std::istreambuf_iterator< char >(oss), std::istreambuf_iterator< char >());
	codeBuffer.push_back(0);
	LogMsg(codeBuffer.data());

}

REGISTER_MISC_TEST_INNER("PreprocessorTest", PreprocessorTest);