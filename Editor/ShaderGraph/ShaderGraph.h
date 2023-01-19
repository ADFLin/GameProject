#ifndef ShaderGraph_H_8C052C61_D77D_4EE8_AE31_1FEC9A95389E
#define ShaderGraph_H_8C052C61_D77D_4EE8_AE31_1FEC9A95389E

#pragma once

#include "Math/Vector4.h"
#include "StdUtility.h"
#include "InlineString.h"

#include <memory>

using Math::Vector4;

enum class ESGValueType
{
	Invalid,

	FloatType,
	Float1,
	Float2,
	Float3,
	Float4,
};

enum class ESGValueOperator
{
	Add,
	Sub,
	Mul,
	Div,
};

enum class ESGIntrinsicFunc
{
	Sin,
	Cos,
	Tan,
	Abs,
};

class SGNode;
using SGNodePtr = std::shared_ptr<SGNode>;

class SGCompiler
{
public:
	virtual int32 emitConst(ESGValueType type, Vector4 const& value) = 0;
	virtual int32 emitAdd(int32 lhs, int32 rhs) = 0;
	virtual int32 emitSub(int32 lhs, int32 rhs) = 0;
	virtual int32 emitMul(int32 lhs, int32 rhs) = 0;
	virtual int32 emitDiv(int32 lhs, int32 rhs) = 0;

	virtual int32 emitTexcoord(int index) = 0;
	virtual int32 emitIntrinsicFunc(ESGIntrinsicFunc func, int32 index) = 0;

	virtual int32 emitNode(SGNode& node) = 0;
};


struct SGNodeInput
{
	ESGValueType type;
	std::weak_ptr<SGNode> link;

	int32 compile(SGCompiler& compiler);
};


class SGNode : public std::enable_shared_from_this<SGNode>
{
public:

	int32 id;

	std::vector<SGNodeInput>  inputs;
	std::vector< std::weak_ptr<SGNode> > linkedNodes;

	void linkOutput(SGNode& node, int index)
	{
		CHECK(IsValidIndex(node.inputs, index));

		SGNodeInput* input = &node.inputs[index];
		linkedNodes.push_back(node.weak_from_this());
		input->link = weak_from_this();
	}

	void unlinkOutput()
	{
		for (auto weakNode : linkedNodes)
		{
			if ( weakNode.expired())
				continue;

			weakNode.lock()->removeInputsLink(*this);
		}
		linkedNodes.clear();
	}

	void unlinkInput(int index)
	{


	}

	virtual ESGValueType getOutputType() = 0;
	virtual int32 compile(SGCompiler& compiler) = 0;

private:

	void removeInputsLink(SGNode& node)
	{
		for (auto& input : inputs)
		{
			if (input.link.expired())
				continue;

			if (input.link.lock() == node.shared_from_this())
			{
				input.link.reset();
			}
		}
	}

};



class SGNodeConst : public SGNode
{
public:
	SGNodeConst(float value)
		:type(ESGValueType::Float1)
		, value(Vector4(value, 0, 0, 0))
	{
	}

	SGNodeConst(float vx, float vy)
		:type(ESGValueType::Float2)
		, value(Vector4(vx, vy, 0, 0))
	{
	}

	SGNodeConst(float vx, float vy, float vz)
		:type(ESGValueType::Float3)
		, value(Vector4(vx, vy, vz, 0))
	{
	}

	SGNodeConst(float vx, float vy, float vz, float vw)
		:type(ESGValueType::Float4)
		, value(Vector4(vx, vy, vz, vw))
	{
	}
	ESGValueType getOutputType() { return type; }
	virtual int32 compile(SGCompiler& compiler)
	{
		return compiler.emitConst(type, value);
	}

	ESGValueType type;
	Vector4 value;
};


class SGNodeOperator : public SGNode
{
public:
	SGNodeOperator(ESGValueOperator op)
		:op(op)
	{
		SGNodeInput pin;
		pin.type = ESGValueType::FloatType;
		inputs.push_back(pin);
		inputs.push_back(pin);
	}
	ESGValueType getOutputType()
	{
		if (inputs[0].link.expired() && inputs[1].link.expired())
			return ESGValueType::Invalid;

		return ESGValueType::FloatType;
	}


	virtual int32 compile(SGCompiler& compiler)
	{
		int32 lhs = inputs[0].compile(compiler);

		for (int i = 1; i < inputs.size(); ++i)
		{
			int32 rhs = inputs[i].compile(compiler);
			switch (op)
			{
			case ESGValueOperator::Add:
				lhs = compiler.emitAdd(lhs, rhs);
				break;
			case ESGValueOperator::Sub:
				lhs = compiler.emitSub(lhs, rhs);
				break;
			case ESGValueOperator::Mul:
				lhs = compiler.emitMul(lhs, rhs);
				break;
			case ESGValueOperator::Div:
				lhs = compiler.emitDiv(lhs, rhs);
				break;
			}
		}

		return lhs;
	}



	ESGValueOperator op;

};

class SGNodeTexcooord : public SGNode
{
public:
	ESGValueType getOutputType()
	{
		return ESGValueType::Float2;
	}

	int32 compile(SGCompiler& compiler)
	{
		return compiler.emitTexcoord(index);
	}
	int index = 0;
};

class SGNodeIntrinsicFunc : public SGNode
{
public:
	SGNodeIntrinsicFunc(ESGIntrinsicFunc func)
		:func(func)
	{
		SGNodeInput pin;
		pin.type = ESGValueType::FloatType;
		inputs.push_back(pin);
		inputs.push_back(pin);
	}
	virtual ESGValueType getOutputType()
	{
		return ESGValueType::FloatType;
	}
	ESGIntrinsicFunc func;
	int32 compile(SGCompiler& compiler)
	{
		int32 index = inputs[0].compile(compiler);
		return compiler.emitIntrinsicFunc(func, index);
	}

};

enum class ESGDomainType
{
	Test,

	None,
};

class ShaderGraph
{
public:

	template< typename TNode, typename ...TArgs>
	std::shared_ptr<TNode> createNode(TArgs&& ...args)
	{
		std::shared_ptr<TNode> result = std::make_shared<TNode>(std::forward<TArgs>(args)...);
		nodes.push_back(result);
		return result;
	}

	void removeAllNodes()
	{
		nodes.clear();
	}
	std::vector<SGNodePtr> nodes;

	void linkDomainInput(SGNode& node, int index)
	{
		CHECK(IsValidIndex(domainInputs, index));
		domainInputs[index].link = node.weak_from_this();
	}

	void setDoamin(ESGDomainType doamin)
	{
		if (mDomain == doamin)
			return;

		mDomain = doamin;
		std::vector<SGNodeInput> domainInputsOld = domainInputs;

		domainInputs.clear();
		switch (doamin)
		{
		case ESGDomainType::Test:
			{
				domainInputs.resize(1);
				domainInputs[0].type = ESGValueType::Float4;
			}
			break;
		case ESGDomainType::None:
			break;
		}
	}
	ESGDomainType mDomain;
	std::vector<SGNodeInput> domainInputs;
};

class SGCompilerCodeGen : public SGCompiler
{

public:
	bool generate(ShaderGraph& graph, std::string& outCode);



	int32 emitConst(ESGValueType type, Vector4 const& value) override
	{
		int32 result = addLocal(type);

		codeValueType(type);
		codeSpace();
		codeVarString(result);
		codeString("=");
		codeValue(type, value);
		codeString(";");
		codeNextline();
		return result;
	}

	int32 emitAdd(int32 lhs, int32 rhs) override
	{
		return emitOp(lhs, rhs, "+");
	}
	int32 emitSub(int32 lhs, int32 rhs) override
	{
		return emitOp(lhs, rhs, "-");
	}
	int32 emitMul(int32 lhs, int32 rhs) override
	{
		return emitOp(lhs, rhs, "*");
	}
	int32 emitDiv(int32 lhs, int32 rhs) override
	{
		return emitOp(lhs, rhs, "/");
	}

	int32 emitOp(int32 lhs, int32 rhs, char const* op)
	{
		ESGValueType typeL = getValueType(lhs);
		ESGValueType typeR = getValueType(lhs);


		ESGValueType type = typeL;
		if (typeL != typeR)
		{
			if (type == ESGValueType::Float1)
				type = typeR;
			else
				type = ESGValueType::Invalid;
		}

		int32 result = addLocal(type);
		codeValueType(type);
		codeSpace();
		codeVarString(result);
		codeString("=");
		codeVarString(lhs);
		codeString(op);
		codeVarString(rhs);
		codeString(";");
		codeNextline();
		return result;
	}

	int32 emitTexcoord(int index)
	{
		InlineString<> text;
		text.format("parameter.texCoords[%d]", index);
		return addSymbol(ESGValueType::Float2, text, false);
	}

	std::unordered_map<SGNode*, int32 > mNodeOutIndexMap;
	int32 emitNode(SGNode& node)
	{
		auto iter = mNodeOutIndexMap.find(&node);
		if (iter != mNodeOutIndexMap.end())
			return iter->second;

		int32 result = node.compile(*this);
		mNodeOutIndexMap.emplace(&node, result);
		return result;
	}

	void codeString(char const* str)
	{
		mCode += str;
	}
	void codeChar(char c)
	{
		mCode += c;
	}

	void codeSpace() { codeChar(' '); }
	void codeNextline() { codeChar('\n\t'); }
	template< typename ...TArgs >
	void codeForamt(char const* format, TArgs&& ...args)
	{
		char buffer[256];
		int num = FCString::PrintfT(buffer, format, args...);
		mCode.append(buffer, num);
	}

	void codeValueType(ESGValueType type)
	{
		switch (type)
		{
		case ESGValueType::Float1:
			codeString("float");
			break;
		case ESGValueType::Float2:
			codeString("float2");
			break;
		case ESGValueType::Float3:
			codeString("float3");
			break;
		case ESGValueType::Float4:
			codeString("float4");
			break;
		default:
			break;
		}
	}

	void codeValue(ESGValueType type, Vector4 const& value)
	{
		switch (type)
		{
		case ESGValueType::Float1:
			codeForamt("%g", value.x);
			break;
		case ESGValueType::Float2:
			codeForamt("float2(%g, %g)", value.x, value.y);
			break;
		case ESGValueType::Float3:
			codeForamt("float3(%g, %g, %g)", value.x, value.y, value.z);
			break;
		case ESGValueType::Float4:
			codeForamt("float4(%g, %g, %g, %g)", value.x, value.y, value.z, value.w);
			break;
		default:
			break;
		}
	}

	void codeVarString(int32 index)
	{
		if (IsLocal(index))
		{
			codeForamt("local_%d", int(index >> 1));
		}
		else
		{
			mCode += mSymbols[index >> 1].name;
		}
	}

	int32 addSymbol(ESGValueType type, char const* name, bool bVariable = false)
	{
		int result = ( mSymbols.size() << 1 ) | 0x1;
		Symbol symbol;
		symbol.name = name;
		symbol.type = type;
		symbol.bVariable = bVariable;
		mSymbols.push_back(symbol);
		return result;
	}

	int32 addLocal(ESGValueType type)
	{
		int result = mLocalVars.size() << 1;
		LocalVar local;
		local.index = result;
		local.type = type;
		mLocalVars.push_back(local);
		return result;
	}

	struct Symbol
	{
		ESGValueType type;
		bool bVariable;
		std::string name;
	};

	struct LocalVar
	{
		ESGValueType type;
		int index;
	};

	static bool IsLocal(int32 index)
	{
		return !(index & 0x1);
	}

	Symbol& getSymbol(int32 index)
	{
		CHECK(index & 0x1);
		return mSymbols[index >> 1];
	}

	ESGValueType getValueType(int32 index)
	{
		if (IsLocal(index))
		{
			return mLocalVars[index >> 0x1].type;
		}
		else
		{
			return mSymbols[index >> 0x1].type;
		}
	}

	std::vector< std::string > mDomionInputCodes;
	std::string mCode;
	std::vector<Symbol> mSymbols;

	std::vector<LocalVar> mLocalVars;



	int32 emitIntrinsicFunc(ESGIntrinsicFunc func, int32 index) override;

};

#endif // ShaderGraph_H_8C052C61_D77D_4EE8_AE31_1FEC9A95389E
