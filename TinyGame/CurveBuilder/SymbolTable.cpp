#include "SymbolTable.h"
#include "LogSystem.h"

char const* SymbolTable::getFuncName(FuncInfo const& info) const
{
	for (auto const& pair : mNameToEntryMap)
	{
		if (pair.second.type == SymbolEntry::eFunction && pair.second.func == info)
			return pair.first.c_str();
	}
	return nullptr;
}

char const* SymbolTable::getFuncName(FuncSymbolInfo const& info) const
{
	for (auto const& pair : mNameToEntryMap)
	{
		if (pair.second.type == SymbolEntry::eFunctionSymbol && pair.second.funcSymbol.id == info.id)
			return pair.first.c_str();
	}
	return nullptr;
}

char const* SymbolTable::getVarName(void* varPtr) const
{
	for (auto const& pair : mNameToEntryMap)
	{
		if (pair.second.type == SymbolEntry::eVariable && pair.second.varValue.ptr == varPtr)
			return pair.first.c_str();
	}
	return nullptr;
}

char const* SymbolTable::getInputName(int index) const
{
	for (auto const& pair : mNameToEntryMap)
	{
		if (pair.second.type == SymbolEntry::eInputVar && pair.second.input.index == index)
			return pair.first.c_str();
	}
	return nullptr;
}

VariableInfo const* SymbolTable::findVar(char const* name) const
{
	auto entry = findSymbol(name, SymbolEntry::eVariable);
	return entry ? &entry->varValue : nullptr;
}

InputInfo const* SymbolTable::findInput(char const*  name) const
{
	auto entry = findSymbol(name, SymbolEntry::eInputVar);
	return entry ? &entry->input : nullptr;
}

FuncInfo const* SymbolTable::findFunc(char const* name) const
{
	auto entry = findSymbol(name, SymbolEntry::eFunction);
	return entry ? &entry->func : nullptr;
}

ConstValueInfo const* SymbolTable::findConst(char const* name) const
{
	auto entry = findSymbol(name, SymbolEntry::eConstValue);
	return entry ? &entry->constValue : nullptr;
}

int SymbolTable::getVarTable(char const* varStr[], double varVal[]) const
{
	int index = 0;
	for (auto const& pair : mNameToEntryMap)
	{
		if (pair.second.type != SymbolEntry::eVariable)
			continue;

		if (varStr) varStr[index] = pair.first.c_str();
		if (varVal)
		{
			switch (pair.second.varValue.layout)
			{
			case ValueLayout::Double: varVal[index] = *(double*)pair.second.varValue.ptr; break;
			case ValueLayout::Float:  varVal[index] = *(float*)pair.second.varValue.ptr; break;
			case ValueLayout::Int32:  varVal[index] = *(int32*)pair.second.varValue.ptr; break;
			default: NEVER_REACH("SymbolTable::getVarTable");
			}
		}
		++index;
	}
	return index;
}
