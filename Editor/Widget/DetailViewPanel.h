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
		int         categoryIndex = INDEX_NONE;
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

	struct CategoryInfo
	{
		std::string name;
		std::function<void(char const*)> callback;
		bool bDefaultOpen = true;
	};

	class RenderContext;
	TArray< PropertyViewInfo > mPropertyViews;
	TArray< CategoryInfo >     mCategories;
	uint32 mNextHandle = 0;

	PropertyViewInfo* getViewInfo(PropertyViewHandle handle);

	PropertyViewHandle addView(Reflection::EPropertyType type, void* ptr, char const* name, char const* category = nullptr);
	PropertyViewHandle addView(Reflection::StructType* structData, void* ptr, char const* name, char const* category = nullptr);
	PropertyViewHandle addView(Reflection::EnumType* enumData, void* ptr, char const* name, char const* category = nullptr);
	PropertyViewHandle addView(Reflection::PropertyBase* property, void* ptr, char const* name, char const* category = nullptr);
	void addCallback(PropertyViewHandle handle, std::function<void(char const*)> const& callback);
	void addCategoryCallback(char const* category, std::function<void(char const*)> const& callback);
	void removeView(PropertyViewHandle handle);
	void clearCategoryViews(char const* name);

	void addCategory(char const* name, bool bDefaultOpen = true);


	int getOrAddCategoryIndex(char const* name);
	int getCategoryIndex(char const* name) const;
	void clearAllViews();
};

BITWISE_RELLOCATABLE_FAIL(DetailViewPanel::CategoryInfo);
BITWISE_RELLOCATABLE_FAIL(DetailViewPanel::PropertyViewInfo);

#endif // DetailPanel_H_B3999968_7BC2_4C9B_8C42_E43EF34171CA