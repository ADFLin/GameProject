#include "ShaderGraph.h"

#include "FileSystem.h"
#include "StringParse.h"
#include "LogSystem.h"
#include "Misc/Format.h"

#define DEFINE_VISIT_NODE(NODE)\
	void SGNodeVisitor::visit(NODE& node){ visitDefault(node); }

DEFINE_VISIT_NODE(SGNodeConst);
DEFINE_VISIT_NODE(SGNodeCustom);

#undef DEFINE_VISIT_NODE

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

	reset();


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

			mCodeSegments.push_back(std::move(mCode));
		}
	}

	int count = Text::Format((char const*)codeTemplate.data(), mCodeSegments, outCode);
	if (count != mCodeSegments.size())
	{
		LogWarning(0, "%s file args is not match with domainInput %d", templateFile, mCodeSegments.size());
	}
	return true;
}

int32 SGCompilerCodeGen::emitDot(int32 lhs, int32 rhs)
{
	ESGValueType type = ESGValueType::Float1;

	int32 result = addLocal(type);
	codeLocalAssign(result, type);
	codeFuncCall("dot", lhs, rhs);
	codeNextline();
	return result;
}

int32 SGCompilerCodeGen::emitOp(int32 lhs, int32 rhs, char const* op)
{
	ESGValueType typeL = getValueType(lhs);
	ESGValueType typeR = getValueType(rhs);
	ESGValueType type = typeL;
	if (typeL != typeR)
	{
		if (type == ESGValueType::Float1)
		{
			type = typeR;
		}
		else if(typeR != ESGValueType::Float1)
		{
			type = ESGValueType::Invalid;
		}
	}

	int32 result = addLocal(type);
	codeLocalAssign(result, type);
	codeVarName(lhs);
	codeString(op);
	codeVarName(rhs);
	codeString(";");
	codeNextline();
	return result;
}

void SGCompilerCodeGen::codeFuncCall(char const* name, int index1)
{
	codeString(name);
	codeChar('(');
	codeVarName(index1);
	codeString(");");

}

void SGCompilerCodeGen::codeFuncCall(char const* name, int index1, int index2)
{
	codeString(name);
	codeVarName(index1);
	codeString(",");
	codeVarName(index2);
	codeString(");");
}

void SGCompilerCodeGen::codeFuncCall(char const* name, TArray<int> indices)
{
	codeString(name);
	codeChar('(');
	for (int i = 0; i < indices.size(); ++i)
	{
		if (i != 0)
		{
			codeChar(',');
		}
		codeVarName(indices[i]);
	}
	codeString(");");
}

int32 SGCompilerCodeGen::findSymbol(char const* name)
{
	int index = 0;
	for (auto const& s : mSymbols)
	{
		if (s.name == name)
			return (index << 1) | 0x1;
		++index;
	}
	return INDEX_NONE;
}

int32 SGCompilerCodeGen::addSymbol(ESGValueType type, char const* name, bool bVariable)
{
	int result = findSymbol(name);
	if (result != INDEX_NONE)
	{
		Symbol const& s = getSymbol(result);
		if (s.bVariable != bVariable || s.type != type)
		{
			LogWarning(0, "Symbol %s definition is not match", name);
		}
		return result;
	}
	result = (mSymbols.size() << 1) | 0x1;
	Symbol symbol;
	symbol.name = name;
	symbol.type = type;
	symbol.bVariable = bVariable;
	mSymbols.push_back(std::move(symbol));
	return result;
}

int32 SGCompilerCodeGen::emitIntrinsicFunc(ESGIntrinsicFunc func, int32 index)
{
	ESGValueType type = getValueType(index);
	int32 result = addLocal(type);

	codeLocalAssign(result, type);
	codeFuncCall(GetString(func), index);
	codeNextline();

	return result;
}

int32 SGCompilerCodeGen::emitTime(bool bReal)
{
	return addSymbol(ESGValueType::Float1, bReal ? "View.realTime" : "View.gameTime", false);
}

int32 SGCompilerCodeGen::emitCallFunc(int32 indexFunc, TArray<int32> const& inputs)
{
	ESGValueType type = getValueType(indexFunc);
	int32 result = addLocal(type);
	codeLocalAssign(result, type);
	codeFuncCall(getSymbol(indexFunc).name.c_str(), inputs);
	codeNextline();
	return result;
}

int32 SGCompilerCodeGen::emitDefineFunc(char const* name, char const* code, ESGValueType returnType, TArray<SGFuncArgInfo> const& args)
{
	InlineString<> defaultName;
	if (name == nullptr)
	{
		defaultName.format("FuncCustom%d", mIndexCustomFunc);
		++mIndexCustomFunc;
		name = defaultName;
	}

	int indexFunc = addSymbol(returnType, name, false);
	CodeScope scoped(*this, mCodeSegments[0]);

	codeValueType(returnType);
	codeSpace();
	codeString(name);
	codeChar('(');
	for (int index = 0; index < args.size(); ++index)
	{
		if (index != 0)
		{
			codeChar(',');
		}
		codeValueType(args[index].type);
		codeSpace();
		codeString(args[index].name);
	}
	codeChar(')');
	codeNextline();
	codeChar('{');
	codeNextline();
	codeString(code);
	codeNextline();
	codeChar('}');
	codeNextline();
	return indexFunc;
}

