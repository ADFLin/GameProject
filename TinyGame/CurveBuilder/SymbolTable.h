#ifndef SymbolTable_h__
#define SymbolTable_h__

#include "ExpressionCore.h"
#include "HashString.h"
#include <unordered_map>

struct SymbolEntry
{
	enum Type
	{
		eFunction,
		eFunctionSymbol,
		eConstValue,
		eVariable,
		eInputVar,
	};
	Type type;

	union
	{
		InputInfo      input;
		FuncInfo       func;
		FuncSymbolInfo funcSymbol;
		ConstValueInfo constValue;
		VariableInfo   varValue;
	};

	SymbolEntry() {}
	SymbolEntry(FuncInfo const& funInfo) :type(eFunction), func(funInfo) {}
	SymbolEntry(ConstValueInfo value) :type(eConstValue), constValue(value) {}
	SymbolEntry(VariableInfo value) :type(eVariable), varValue(value) {}
	SymbolEntry(InputInfo value) :type(eInputVar), input(value) {}
	SymbolEntry(FuncSymbolInfo value) :type(eFunctionSymbol), funcSymbol(value) {}
};

class SymbolTable
{
public:
	template< class T >
	void            defineConst(char const* name, T val) { mNameToEntryMap[name] = ConstValueInfo{ val }; }
	template< class T >
	void            defineVar(char const* name, T* varPtr) { mNameToEntryMap[name] = VariableInfo{ varPtr }; }
	template< class T>
	void            defineFunc(char const* name, T func) { mNameToEntryMap[name] = FuncInfo{ func }; }

	void            defineFuncSymbol(char const* name, int32 symbolId, int32 numParams = 1) { mNameToEntryMap[name] = FuncSymbolInfo{ symbolId, numParams }; }
	void            defineVarInput(char const* name, uint8 inputIndex) { mNameToEntryMap[name] = InputInfo{ inputIndex }; }

	ConstValueInfo const* findConst(char const* name) const;
	FuncInfo const*       findFunc(char const* name) const;
	VariableInfo const*   findVar(char const* name) const;
	InputInfo const*      findInput(char const* name) const;

	char const*     getFuncName(FuncInfo const& info) const;
	char const*     getFuncName(FuncSymbolInfo const& info) const;
	char const*     getVarName(void* varPtr) const;
	char const*     getInputName(int index) const;

	bool            isFuncDefined(char const* name) const { return isDefinedInternal(name, SymbolEntry::eFunction); }
	bool            isVarDefined(char const* name) const { return isDefinedInternal(name, SymbolEntry::eVariable); }
	bool            isConstDefined(char const* name) const { return isDefinedInternal(name, SymbolEntry::eConstValue); }

	int             getVarTable(char const* varStr[], double varVal[]) const;

	SymbolEntry const* findSymbol(char const* name) const
	{
		HashString key;
		if (!HashString::Find(name, key)) return nullptr;
		auto iter = mNameToEntryMap.find(key);
		if (iter != mNameToEntryMap.end()) return &iter->second;
		return nullptr;
	}

	SymbolEntry const* findSymbol(char const* name, SymbolEntry::Type type) const
	{
		HashString key;
		if (!HashString::Find(name, key)) return nullptr;
		auto iter = mNameToEntryMap.find(key);
		if (iter != mNameToEntryMap.end() && iter->second.type == type) return &iter->second;
		return nullptr;
	}

protected:
	bool  isDefinedInternal(char const* name, SymbolEntry::Type type) const { return findSymbol(name, type) != nullptr; }
	std::unordered_map< HashString, SymbolEntry > mNameToEntryMap;
};

#endif // SymbolTable_h__
