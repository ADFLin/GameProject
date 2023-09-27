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

	enum class EViewType
	{
		Primitive,
		Struct,
		Enum,
		Property,
	};

	struct PropertyViewInfo
	{
		PropertyViewHandle handle;
		std::string name;
		EViewType   type;
		void*       ptr;
		std::function<void(char const*)> callback;
		union 
		{
			Reflection::EPropertyType primitiveType;
			Reflection::StructType*   structData;
			Reflection::EnumType*     enumData;
			Reflection::PropertyBase* property;
		};
	};

	class RenderContext;
	TArray< PropertyViewInfo > mPropertyViews;
	uint32 mNextHandle = 0;

	PropertyViewInfo* getViewInfo(PropertyViewHandle handle);

	PropertyViewHandle addView(Reflection::EPropertyType type, void* ptr, char const* name);
	PropertyViewHandle addView(Reflection::StructType* structData, void* ptr, char const* name);
	PropertyViewHandle addView(Reflection::EnumType* enumData, void* ptr, char const* name);
	PropertyViewHandle addView(Reflection::PropertyBase* property, void* ptr, char const* name);
	void addCallback(PropertyViewHandle handle, std::function<void(char const*)> const& callback);
	void removeView(PropertyViewHandle handle);

	void clearAllViews();
};

#endif // DetailPanel_H_B3999968_7BC2_4C9B_8C42_E43EF34171CA