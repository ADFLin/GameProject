#include "EditorInternal.h"

EditorInternal* GEditor = nullptr;


IDetailCustomization* EditorInternal::findCustomization(Reflection::StructType* type)
{
	auto iter = mCustomizations.find(type);
	if (iter != mCustomizations.end())
		return iter->second.get();
	return nullptr;
}
