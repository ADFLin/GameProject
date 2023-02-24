#include "ShaderGraph.h"

#include "FileSystem.h"
#include "StringParse.h"
#include "LogSystem.h"

static int Printf(char const* format, TArray< std::string > const& strList, std::string& outString)
{
	int index = 0;
	char const* cur = format;
	char const* write = cur;
	if (strList.size() > 0)
	{
		while (*cur != 0)
		{
			char const* next = FStringParse::FindChar(cur, '%');
			if (next[0] == '%' && next[1] == 's')
			{
				if (write != next)
				{
					outString.append(write, next - write);
					write = next + 2;
				}
				outString += strList[index];
				++index;
				if (!strList.isValidIndex(index))
				{
					break;
				}
			}
			cur = next;
		}
	}

	if (*write != 0)
	{
		outString.append(write);
	}
	return index;
}

int32 SGNodeInput::compile(SGCompiler& compiler)
{
	if (link.expired())
	{
		return INDEX_NONE;
	}

	return compiler.emitNode(*link.lock());
}

bool SGCompilerCodeGen::generate(ShaderGraph& graph, std::string& outCode)
{
	std::vector<uint8> codeTemplate;

	char const* templateFile = nullptr;
	switch (graph.mDomain)
	{
	case ESGDomainType::Test:
		templateFile = "Shader/Template/Test.sgc";
		break;
	default:
		return false;
	}
	if (!FFileUtility::LoadToBuffer(templateFile, codeTemplate, true))
	{
		LogWarning(0,"Can't load shader template file : %s", templateFile);
		return false;
	}
	mNodeOutIndexMap.clear();
	mCode.clear();
	mDomionInputCodes.clear();
	for (auto& input : graph.domainInputs)
	{
		if (input.link.expired())
		{
			input.type;
		}
		else
		{
			int32 indexResult = input.compile(*this);
			codeString("return");
			codeSpace();
			ESGValueType returnType = getValueType(indexResult);

			if (returnType != ESGValueType::Float4) 
			{
				codeString("float4(");
				codeVarName(indexResult);
				switch (returnType)
				{
				case ESGValueType::Float1:
					codeString(".xxxx);");
					break;
				case ESGValueType::Float2:
					codeString(",0,1);");
					break;
				case ESGValueType::Float3:
					codeString(",1);");
					break;
				}
			}
			else
			{
				codeVarName(indexResult);
				codeChar(';');
			}

			mDomionInputCodes.push_back(std::move(mCode));
		}
	}
	int count = Printf((char const*)codeTemplate.data(), mDomionInputCodes, outCode);
	return true;
}

int32 SGCompilerCodeGen::emitIntrinsicFunc(ESGIntrinsicFunc func, int32 index)
{
	ESGValueType type = getValueType(index);

	int32 result = addLocal(type);
	
	codeValueType(type);
	codeSpace();
	codeVarName(result);
	codeString("=");
	switch (func)
	{
	case ESGIntrinsicFunc::Sin:
		codeString("sin(");
		break;
	case ESGIntrinsicFunc::Cos:
		codeString("cos(");
		break;
	case ESGIntrinsicFunc::Tan:
		codeString("tan(");
		break;
	case ESGIntrinsicFunc::Abs:
		codeString("abs(");
		break;
	case ESGIntrinsicFunc::Exp:
		codeString("exp(");
		break;
	case ESGIntrinsicFunc::Log:
		codeString("log(");
		break;
	}
	codeVarName(index);
	codeString(");");
	codeNextline();

	return result;
}

int32 SGCompilerCodeGen::emitTime(bool bReal)
{
	return addSymbol(ESGValueType::Float1, bReal ? "View.realTime" : "View.gameTime", false);
}

