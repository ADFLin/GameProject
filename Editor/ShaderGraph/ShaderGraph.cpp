#include "ShaderGraph.h"

#include "FileSystem.h"
#include "StringParse.h"

static int Printf(char const* format, std::vector< std::string > const& strList, std::string& outString)
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
				if (!IsValidIndex(strList, index))
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
	switch (graph.mDomain)
	{
	case ESGDomainType::Test:
		if (!FFileUtility::LoadToBuffer("Shader/Template/Test.sgc", codeTemplate, true))
			return false;
		break;
	default:
		return false;
	}

	mNodeOutIndexMap.clear();
	mCode.clear();
	mDomionInputCodes.clear();
	for (auto& input : graph.domainInputs)
	{
		if (input.link.expired())
		{

		}
		else
		{
			int32 indexResult = input.compile(*this);
			codeString("return");
			codeSpace();
			ESGValueType returnType = getValueType(indexResult);

			if (returnType != ESGValueType::Float4 || returnType != ESGValueType::Float1) 
			{
				codeString("float4(");
				codeVarString(indexResult);
				switch (returnType)
				{
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
				codeVarString(indexResult);
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
	codeVarString(result);
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
	}
	codeVarString(index);
	codeString(");");

	return result;
}

