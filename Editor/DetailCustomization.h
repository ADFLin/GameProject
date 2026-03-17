#pragma once
#ifndef DetailCustomization_H_54F7990F_F10C_4F74_A15A_6823A63F68F4
#define DetailCustomization_H_54F7990F_F10C_4F74_A15A_6823A63F68F4

#include "EditorConfig.h"
#include "Reflection.h"
#include <memory>
#include <unordered_map>

class IDetailLayoutBuilder
{
public:
	virtual ~IDetailLayoutBuilder() = default;
	virtual void* getStructPtr() = 0;
	virtual void  addProperty(char const* propertyName, char const* label = nullptr) = 0;
	virtual void  addProperty(Reflection::PropertyBase* property, void* ptr, char const* label = nullptr) = 0;
	virtual void  hideProperty(char const* propertyName) = 0;
};

class IDetailCustomization
{
public:
	virtual ~IDetailCustomization() = default;
	virtual void customizeLayout(IDetailLayoutBuilder& builder) = 0;
};

#endif // DetailCustomization_H_54F7990F_F10C_4F74_A15A_6823A63F68F4
