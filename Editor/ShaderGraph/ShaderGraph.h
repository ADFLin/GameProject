#ifndef ShaderGraph_H_8C052C61_D77D_4EE8_AE31_1FEC9A95389E
#define ShaderGraph_H_8C052C61_D77D_4EE8_AE31_1FEC9A95389E

#pragma once

#include "Math/Vector4.h"
#include "StdUtility.h"
#include "InlineString.h"
#include "DataStructure/Array.h"

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
	Exp,
	Log,
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

	virtual int32 emitTime(bool bReal) = 0;
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

	TArray<SGNodeInput>  inputs;

	struct LinkInfo
	{
		int index;
		std::weak_ptr<SGNode> node;

		bool isValid() const { return node.expired() == false; }

		SGNodeInput& getIntput()
		{
			CHECK(node.expired() == false);
			return node.lock()->inputs[index];
		}
	};
	TArray< LinkInfo > outputLinks;

	void* getOutputId() { return &outputLinks; }
	static SGNode* GetFromOuputId(void* ptr)
	{
		return reinterpret_cast<SGNode*>((uint8*)(ptr)-offsetof(SGNode, outputLinks));
	}

	void linkTo(SGNode& node, int index)
	{
		CHECK(node.inputs.isValidIndex(index));

		SGNodeInput* input = &node.inputs[index];
		if (!input->link.expired())
		{
			node.unlinkInput(index);
		}
		LinkInfo link;
		link.index = index;
		link.node = node.weak_from_this();
		outputLinks.push_back(link);
		input->link = weak_from_this();
	}

	void unlinkAllOutputs()
	{
		for (auto& link : outputLinks)
		{
			if (!link.node.expired())
			{
				link.node.lock()->unlinkInput(link.index);
			}
		}
		outputLinks.clear();
	}

	void unlinkInput(int index)
	{
		SGNodeInput& input = inputs[index];
		CHECK(input.link.expired() == false);
		input.link.lock()->removeOutputLink(*this, index);
		input.link.reset();
	}

	void removeOutputLink(SGNode& node, int index)
	{
		for (auto iter = outputLinks.begin(), itEnd = outputLinks.end();
			iter != itEnd; ++iter)
		{
			auto& link = *iter;
			if (link.index == index && link.node.lock().get() == &node)
			{
				outputLinks.erase(iter);
				return;
			}
		}
	}

	virtual ESGValueType getOutputType() = 0;
	virtual int32 compile(SGCompiler& compiler) = 0;


	virtual std::string getTitle() = 0;

	virtual SGNode* copy() = 0;

private:



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

	std::string getTitle() override
	{
		switch (type)
		{
		case ESGValueType::Float1:
			return InlineString<>::Make("%g", value.x);
		case ESGValueType::Float2:
			return InlineString<>::Make("float2(%g, %g)", value.x, value.y);
		case ESGValueType::Float3:
			return InlineString<>::Make("float3(%g, %g, %g)", value.x, value.y, value.z);
		case ESGValueType::Float4:
			return InlineString<>::Make("float4(%g, %g, %g, %g)", value.x, value.y, value.z, value.w);
		}
		return "Unknown Const Value";
	}

	ESGValueType type;
	Vector4 value;

	virtual SGNode* copy() override
	{
		return new SGNodeConst(*this);
	}

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

	std::string getTitle() override
	{
		switch (op)
		{
		case ESGValueOperator::Add:
			return "+";
		case ESGValueOperator::Sub:
			return "-";
		case ESGValueOperator::Mul:
			return "*";
		case ESGValueOperator::Div:
			return "/";
		}
		return "Unknown Operator";
	}


	ESGValueOperator op;

	virtual SGNode* copy() override
	{
		return new SGNodeOperator(op);
	}

};

class SGNodeTexcooord : public SGNode
{
public:
	SGNodeTexcooord(int index = 0)
		:index(index)
	{

	}
	ESGValueType getOutputType()
	{
		return ESGValueType::Float2;
	}

	int32 compile(SGCompiler& compiler)
	{
		return compiler.emitTexcoord(index);
	}

	std::string getTitle() override
	{
		return InlineString<>::Make("Texcoord[%d]", index);
	}

	virtual SGNode* copy() override
	{
		return new SGNodeTexcooord(index);
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

	std::string getTitle() override
	{
		switch (func)
		{
		case ESGIntrinsicFunc::Sin:
			return "Sin";
		case ESGIntrinsicFunc::Cos:
			return "Cos";
		case ESGIntrinsicFunc::Tan:
			return "Tan";
		case ESGIntrinsicFunc::Abs:
			return "Abs";
		case ESGIntrinsicFunc::Exp:
			return "Exp";
		case ESGIntrinsicFunc::Log:
			return "Log";
		}

		return "Unknown Func";
	}

	virtual SGNode* copy() override
	{
		return new SGNodeIntrinsicFunc(func);
	}
};

class SGNodeTime : public SGNode
{
public:
	SGNodeTime(bool bReal)
		:bReal(bReal)
	{

	}
	virtual ESGValueType getOutputType()
	{
		return ESGValueType::Float1;
	}
	int32 compile(SGCompiler& compiler)
	{
		return compiler.emitTime(bReal);
	}

	std::string getTitle() override
	{
		return "GameTime";
	}

	virtual SGNode* copy() override
	{
		return new SGNodeTime(bReal);
	}

	bool bReal;
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
	TArray<SGNodePtr> nodes;

	void linkDomainInput(SGNode& node, int index)
	{
		CHECK(domainInputs.isValidIndex(index));
		DomainInput& input = domainInputs[index];
		input.link = node.weak_from_this();
	}

	void setDoamin(ESGDomainType doamin)
	{
		if (mDomain == doamin)
			return;

		mDomain = doamin;
		TArray<DomainInput> domainInputsOld = domainInputs;

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

	bool isDomainInput(SGNodeInput* input, int& outIndex)
	{
		int index = 0;
		for (auto& domainInput : domainInputs)
		{
			if (&domainInput == input)
			{
				outIndex = index;
				return true;
			}
		}
		return false;
	}

	SGNode* findNode(SGNodeInput* input, int& outIndex)
	{
		for (auto& node : nodes)
		{
			int index = 0;
			for (auto& curInput : node->inputs)
			{
				if (input == &curInput)
				{
					outIndex = index;
					return node.get();
				}
				++index;
			}
		}
		return nullptr;
	}

	bool isNodeOutputId(void* ptr)
	{
		for (auto& node : nodes)
		{
			if (ptr == node->getOutputId())
				return true;
		}
		return false;
	}
	ESGDomainType mDomain;
	struct DomainInput : SGNodeInput
	{
		Vector4 defaultValue;
	};
	TArray<DomainInput> domainInputs;
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
		codeVarName(result);
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
		ESGValueType typeR = getValueType(rhs);
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
		codeVarName(result);
		codeString("=");
		codeVarName(lhs);
		codeString(op);
		codeVarName(rhs);
		codeString(";");
		codeNextline();
		return result;
	}

	int32 emitTexcoord(int index)
	{
		InlineString<> text;
		text.format("Parameters.texCoords[%d]", index);
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

	void codeVarName(int32 index)
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

	TArray< std::string > mDomionInputCodes;
	std::string mCode;
	TArray<Symbol> mSymbols;

	TArray<LocalVar> mLocalVars;



	int32 emitIntrinsicFunc(ESGIntrinsicFunc func, int32 index) override;
	int32 emitTime(bool bReal) override;
};

#endif // ShaderGraph_H_8C052C61_D77D_4EE8_AE31_1FEC9A95389E
