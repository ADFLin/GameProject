#pragma once
#ifndef DetailPanel_H_B3999968_7BC2_4C9B_8C42_E43EF34171CA
#define DetailPanel_H_B3999968_7BC2_4C9B_8C42_E43EF34171CA

#include "EditorPanel.h"
#include "ReflectionCollect.h"
#include "Reflection.h"
#include "Editor.h"

class DetailViewPanel : public IEditorPanel
{
public:
	static constexpr char const* ClassName = "DetailView";

	void render();
	void clearProperties();

	struct PropertyViewInfo
	{
		Reflection::EPropertyType type;
		void* ptr;

		union 
		{
			Reflection::StructType* structData;
			Reflection::EnumType*   enumData;
		};
	};

	TArray< PropertyViewInfo > mPropertyViews;

	void addPrimitive(Reflection::EPropertyType type, void* ptr);
	void addStruct(Reflection::StructType* structData, void* ptr);
	void addEnum(Reflection::EnumType* enumData, void* ptr);
};

#endif // DetailPanel_H_B3999968_7BC2_4C9B_8C42_E43EF34171CA