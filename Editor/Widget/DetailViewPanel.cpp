#include "DetailViewPanel.h"
#include "InlineString.h"
#include "Math/Quaternion.h"

REGISTER_EDITOR_PANEL(DetailViewPanel, DetailViewPanel::ClassName, false, true);

using namespace Reflection;

enum class EDisplayType
{
	None,

	Bool,
	Primitive,
	Enum,

	Vector2,
	Vector3,
	Vector4,

	Color3f,
	Color4f,

	Struct,
	Array ,
	Quaternion,
	String,

};


static EDisplayType GetDisplayType(PropertyBase* property)
{
	switch (property->getType())
	{
	case EPropertyType::Uint8:
	case EPropertyType::Uint16:
	case EPropertyType::Uint32:
	case EPropertyType::Uint64:
	case EPropertyType::Int8:
	case EPropertyType::Int16:
	case EPropertyType::Int32:
	case EPropertyType::Int64:
	case EPropertyType::Bool:
	case EPropertyType::Float:
	case EPropertyType::Double:
		return EDisplayType::Primitive;

	case EPropertyType::Enum:
		return EDisplayType::Enum;
	case EPropertyType::StdString:
		return EDisplayType::String;
	case EPropertyType::Array:
		return EDisplayType::Array;
	case EPropertyType::Struct:
	case EPropertyType::UnformattedStruct:
		{
			auto structProperty = static_cast<StructProperty*>(property);
			if (property->typeIndex == typeid(Color3f))
			{
				return EDisplayType::Color3f;
			}
			if (property->typeIndex == typeid(Color4f))
			{
				return EDisplayType::Color4f;
			}
			if (property->typeIndex == typeid(Color3f))
			{
				return EDisplayType::Color3f;
			}
			if (property->typeIndex == typeid(Math::Vector3))
			{
				return EDisplayType::Vector3;
			}
			if (property->typeIndex == typeid(Math::Vector2))
			{
				return EDisplayType::Vector2;
			}
			if (property->typeIndex == typeid(Math::Quaternion))
			{
				return EDisplayType::Quaternion;
			}
			else
			{
				return (property->getType() == EPropertyType::Struct) ? EDisplayType::Struct : EDisplayType::None;
			}
		}
		break;
	}

	return EDisplayType::None;
}

ImGuiDataType ToImGuiDataType(EPropertyType type)
{
	switch (type)
	{
	case EPropertyType::Uint8: return ImGuiDataType_U8;
	case EPropertyType::Uint16:return ImGuiDataType_U16;
	case EPropertyType::Uint32:return ImGuiDataType_U32;
	case EPropertyType::Uint64:return ImGuiDataType_U64;
	case EPropertyType::Int8:  return ImGuiDataType_S8;
	case EPropertyType::Int16: return ImGuiDataType_S16;
	case EPropertyType::Int32: return ImGuiDataType_S32;
	case EPropertyType::Int64: return ImGuiDataType_S64;
	}
}

template< typename T >
ReflectionMeta::TSlider<T>* GetSliderMeta(Reflection::MetaData* metaData)
{
	if (metaData)
	{
		return metaData->find<ReflectionMeta::TSlider<T>>();
	}
	return nullptr;
}

struct DetailViewPanel::RenderContext
{
	RenderContext(PropertyViewInfo& propertyView, DetailViewPanel& panel)
		:propertyView(propertyView)
		,panel(panel)
	{
	}

	PropertyViewInfo& propertyView;
	DetailViewPanel& panel;
	char const* propertyName = nullptr;


	void verify(bool bChanged)
	{
		if (bChanged)
		{
			if (propertyView.callback)
			{
				propertyView.callback(propertyName);
			}
			// Also fire the category-level callback if one is registered
			if (propertyView.categoryIndex != INDEX_NONE)
			{
				auto& cat = panel.mCategories[propertyView.categoryIndex];
				if (cat.callback)
				{
					cat.callback(propertyName);
				}
			}
		}
	}


	template< typename T >
	void renderScalarInput(ImGuiDataType ScalarType, void* pData, Reflection::MetaData* metaData)
	{
		auto slider = GetSliderMeta< T>(metaData);
		if (slider)
		{
			ImGuiSliderFlags flags = 0;
			verify(ImGui::SliderScalar("##Property", ScalarType, pData, &slider->min, &slider->max, nullptr, flags));
		}
		else
		{
			ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue;
			ImGui::InputScalar("##Property", ScalarType, pData, NULL, NULL, "%d", flags);
			verify(ImGui::IsItemDeactivatedAfterEdit());
		}
	}

	void renderPrimitivePropertyValue(Reflection::EPropertyType type, void* pData, Reflection::MetaData* metaData = nullptr)
	{
		using namespace Reflection;
		switch (type)
		{
		case EPropertyType::Uint8:
			{
				renderScalarInput<uint8>(ToImGuiDataType(type), pData, metaData);
			}
			break;
		case EPropertyType::Uint16:
			{
				renderScalarInput<uint16>(ToImGuiDataType(type), pData, metaData);
			}
			break;
		case EPropertyType::Uint32:
			{
				renderScalarInput<uint32>(ToImGuiDataType(type), pData, metaData);
			}
			break;
		case EPropertyType::Uint64:
			{
				renderScalarInput<uint64>(ToImGuiDataType(type), pData, metaData);
			}
			break;
		case EPropertyType::Int8:
			{
				renderScalarInput<int8>(ToImGuiDataType(type), pData, metaData);
			}
			break;
		case EPropertyType::Int16:
			{
				renderScalarInput<int16>(ToImGuiDataType(type), pData, metaData);
			}
			break;
		case EPropertyType::Int32:
			{
				renderScalarInput<int32>(ToImGuiDataType(type), pData, metaData);
			}
			break;
		case EPropertyType::Int64:
			{
				renderScalarInput<int64>(ToImGuiDataType(type), pData, metaData);
			}
			break;
		case EPropertyType::Bool:
			{
				verify(ImGui::Checkbox("##Property", (bool*)pData));
			}
			break;
		case EPropertyType::Float:
			{
				auto slider = GetSliderMeta<float>(metaData);
				if (slider)
				{
					ImGuiSliderFlags flags = 0;
					verify(ImGui::SliderFloat("##Property", (float*)pData, slider->min, slider->max, "%.3f", flags));
				}
				else
				{
					ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue;
					ImGui::InputFloat("##Property", (float*)pData, 0.0f, 0.0f, "%.3f", flags);
					verify(ImGui::IsItemDeactivatedAfterEdit());
				}
			}
			break;
		case EPropertyType::Double:
			{
				ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue;
				ImGui::InputDouble("##Property", (double*)pData, 0.0f, 0.0f, "%.3f", flags);
				verify(ImGui::IsItemDeactivatedAfterEdit());
			}
			break;
		}
	}

	void renderEnumValue(EnumType* enumData, void* pData)
	{
		auto values = enumData->values;
		int64 curValue = enumData->getValue(pData);
		int selection = values.findIndexPred([curValue](ReflectEnumValueInfo const& info)
		{
			return info.value == curValue;
		});
		if (ImGui::BeginCombo("##Property", selection == INDEX_NONE ? "" : values[selection].text))
		{
			int index = 0;
			for (auto const& value : values)
			{
				const bool isSelected = (selection == index);
				bool bChanged = ImGui::Selectable(value.text, isSelected);
				verify(bChanged);
				if (bChanged)
				{
					selection = index;
					enumData->setValue(pData, value.value);
					bChanged = true;
				}

				if (isSelected)
				{
					ImGui::SetItemDefaultFocus();
				}
				++index;
			}
			ImGui::EndCombo();
		}
	}

	void RenderPropertyValue(EDisplayType displayType, void* pData, Reflection::PropertyBase* property)
	{
		switch (displayType)
		{
		case EDisplayType::Primitive:
			{
				renderPrimitivePropertyValue(property->getType(), pData, &property->meta);
			}
			break;
		case EDisplayType::Vector2:
			{
				ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue;
				ImGui::InputFloat2("##Property", (float*)pData, "%.3f", flags);
				verify(ImGui::IsItemDeactivatedAfterEdit());
			}
			break;
		case EDisplayType::Vector3:
			{
				ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue;
				ImGui::InputFloat3("##Property", (float*)pData, "%.3f", flags);
				verify(ImGui::IsItemDeactivatedAfterEdit());
			}
			break;
		case EDisplayType::Color3f:
			{
				ImGuiColorEditFlags flags =
					ImGuiColorEditFlags_Float |
					ImGuiColorEditFlags_InputRGB |
					ImGuiColorEditFlags_DisplayRGB |
					ImGuiColorEditFlags_PickerHueBar;
				verify(ImGui::ColorEdit3("##Property", (float*)pData, flags));
			}
			break;
		case EDisplayType::Color4f:
			{
				ImGuiColorEditFlags flags =
					ImGuiColorEditFlags_Float |
					ImGuiColorEditFlags_InputRGB |
					ImGuiColorEditFlags_DisplayRGB |
					ImGuiColorEditFlags_PickerHueBar;
				verify(ImGui::ColorEdit4("##Property", (float*)pData, flags));
			}
			break;
		case EDisplayType::Enum:
			{
				auto enumProperty = static_cast<EnumProperty*>(property);
				if (enumProperty->enumData)
				{
					renderEnumValue(enumProperty->enumData, pData);
				}
				else
				{
					renderPrimitivePropertyValue(enumProperty->underlyingType, pData);
				}
			}
			break;
		case EDisplayType::Quaternion:
			{
				Math::Quaternion& q = *(Math::Quaternion*)pData;
				Math::Vector3 eulerVal = q.getEulerZYX();
				float angles[3] = { Math::RadToDeg(eulerVal.x), Math::RadToDeg(eulerVal.y), Math::RadToDeg(eulerVal.z) };
				if (ImGui::DragFloat3("##Property", angles, 0.1f))
				{
					q.setEulerZYX(Math::DegToRad(angles[2]), Math::DegToRad(angles[1]), Math::DegToRad(angles[0]));
				}
				verify(ImGui::IsItemDeactivatedAfterEdit());
			}
			break;
		case EDisplayType::String:
			{
				std::string& str = *(std::string*)pData;
				char buffer[1024];
				FCString::Copy(buffer, str.c_str());
				if (ImGui::InputText("##Property", buffer, ARRAY_SIZE(buffer), ImGuiInputTextFlags_EnterReturnsTrue))
				{
					str = buffer;
					verify(true);
				}
			}
			break;
		}
	}

	void renderStructRow(char const* name, StructType* structData, void* pData, int level, std::function<void()> const& removeCallback = nullptr)
	{
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanFullWidth;
		bool bOpened = ImGui::TreeNodeEx(name, flags);
		ImGui::TableSetColumnIndex(1);
		ImGui::AlignTextToFramePadding();
		if (removeCallback)
		{
			if (ImGui::SmallButton("-"))
			{
				removeCallback();
			}
			ImGui::SameLine();
		}

		ImGui::Text("%u members", structData->properties.size());
		if (bOpened)
		{
			renderStructContentRow(structData, pData, level + 1);
			ImGui::TreePop();
		}
	}

	void renderPropertyRow(char const* name, Reflection::PropertyBase* property, void* pData, int level, std::function<void()> const& removeCallback = nullptr)
	{
		EDisplayType displayType = GetDisplayType(property);

		switch (displayType)
		{
		case EDisplayType::Struct:
			{
				void* structPtr = property->getDataInStruct(pData);
				StructProperty* structProperty = static_cast<StructProperty*>(property);
				renderStructRow(name, structProperty->structData, structPtr, level, removeCallback);
			}
			break;
		case EDisplayType::Array:
			{
				ArrayPropertyBase* arrayProperty = static_cast<ArrayPropertyBase*>(property);
				void* arrayPtr = arrayProperty->getDataInStruct(pData);
				int count = arrayProperty->getElementCount(arrayPtr);

				ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanFullWidth;
				bool bOpened = ImGui::TreeNodeEx(name, flags);

				ImGui::TableSetColumnIndex(1);
				ImGui::AlignTextToFramePadding();
				if (removeCallback)
				{
					if (ImGui::SmallButton("-"))
					{
						removeCallback();
					}
					ImGui::SameLine();
				}

				ImGui::Text("%d elements", count);
				ImGui::SameLine();
				if (ImGui::SmallButton("+"))
				{
					arrayProperty->resize(arrayPtr, count + 1);
					verify(true);
				}
				ImGui::SameLine();
				if (ImGui::SmallButton("-"))
				{
					if (count > 0)
					{
						arrayProperty->resize(arrayPtr, count - 1);
						verify(true);
					}
				}
				ImGui::SameLine();
				if (ImGui::SmallButton("Clear"))
				{
					arrayProperty->resize(arrayPtr, 0);
					verify(true);
				}

				if (bOpened)
				{
					for (int index = 0; index < count; ++index)
					{
						ImGui::TableNextRow();

						void* elementPtr = arrayProperty->getElement(arrayPtr, index);
						ImGui::PushID(elementPtr);
						ImGui::TableSetColumnIndex(0);
						ImGui::AlignTextToFramePadding();
						InlineString<32> str;
						str.format("[%d]", index);
						renderPropertyRow(str.c_str(), arrayProperty->elementProperty, elementPtr, level + 1, [arrayProperty, arrayPtr, index, this]()
						{
							arrayProperty->removeElement(arrayPtr, index);
							verify(true);
						});
						ImGui::PopID();
					}

					ImGui::TreePop();
				}
			}
			break;
		default:
			{
				ImGui::TextUnformatted(name);
				ImGui::TableSetColumnIndex(1);
				ImGui::AlignTextToFramePadding();
				if (removeCallback)
				{
					if (ImGui::SmallButton("-"))
					{
						removeCallback();
					}
					ImGui::SameLine();
				}
				ImGui::SetNextItemWidth(-FLT_MIN);
				RenderPropertyValue(displayType, property->getDataInStruct(pData), property);
			}
			break;
		}
	}

	void renderStructContentRow(Reflection::StructType* structData, void* pData, int level = 0)
	{
		ImGui::PushID(pData);

		for (auto property : structData->properties)
		{
			ImGui::TableNextRow();

			ImGui::PushID(property);
			ImGui::TableSetColumnIndex(0);
			ImGui::AlignTextToFramePadding();
			renderPropertyRow(property->name, property, pData, level);
			ImGui::PopID();
		}
		ImGui::PopID();
	}

	void render()
	{
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::AlignTextToFramePadding();
		switch (propertyView.type)
		{
		case EViewType::Primitive:
			if (propertyView.name.length())
			{
				ImGui::TextUnformatted(propertyView.name.c_str());
			}

			{
				ImGui::TableSetColumnIndex(1);
				ImGui::SetNextItemWidth(-FLT_MIN);
				renderPrimitivePropertyValue(propertyView.primitiveType, propertyView.ptr);
			}
			break;
		case EViewType::Struct:
			{
				if (propertyView.name.length())
				{
					renderStructRow(propertyView.name.c_str(), propertyView.structData, propertyView.ptr, 0);
				}
				else
				{
					renderStructContentRow(propertyView.structData, propertyView.ptr);
				}
			}
			break;
		case EViewType::Enum:
			if (propertyView.name.length())
			{
				ImGui::TextUnformatted(propertyView.name.c_str());
			}
			{
				ImGui::TableSetColumnIndex(1);
				ImGui::SetNextItemWidth(-FLT_MIN);
				renderEnumValue(propertyView.enumData, propertyView.ptr);
			}
			break;
		case EViewType::Property:
			{
				renderPropertyRow(propertyView.name.c_str(), propertyView.property, propertyView.ptr, 0);
			}
			break;
		}
	}
};

void DetailViewPanel::update()
{
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));

	// Determine if we have any categorized views
	bool bHasCategories = !mCategories.empty();

	if (bHasCategories)
	{
		// Render uncategorized items first (no table wrapping needed between categories)
		// Render each category as a CollapsingHeader, then its items in a table inside
		auto renderCategoryItems = [&](int categoryIndex, int id)
		{
			ImGui::PushID(id);
			if (ImGui::BeginTable("split", 2, ImGuiTableFlags_Resizable))
			{
				for (auto& propertyView : mPropertyViews)
				{
					if (propertyView.categoryIndex != categoryIndex)
						continue;

					ImGui::PushID((int)propertyView.handle);
					RenderContext context(propertyView, *this);
					context.render();
					ImGui::PopID();
				}
				ImGui::EndTable();
			}
			ImGui::PopID();
		};

		renderCategoryItems(INDEX_NONE, 0);

		// Each named category under a CollapsingHeader
		for (int catIndex = 0; catIndex < mCategories.size(); ++catIndex)
		{
			auto& catInfo = mCategories[catIndex];
			ImGuiTreeNodeFlags flags = catInfo.bDefaultOpen ? ImGuiTreeNodeFlags_DefaultOpen : 0;
			if (ImGui::CollapsingHeader(catInfo.name.c_str(), flags))
			{
				renderCategoryItems(catIndex, catIndex + 1);
			}
		}
	}
	else
	{
		// No categories: original flat table rendering
		if (ImGui::BeginTable("split", 2, ImGuiTableFlags_Resizable))
		{
			for (auto& propertyView : mPropertyViews)
			{
				ImGui::PushID((int)propertyView.handle);
				RenderContext context(propertyView, *this);
				context.render();
				ImGui::PopID();
			}
			ImGui::EndTable();
		}
	}

	ImGui::PopStyleVar();
}

DetailViewPanel::PropertyViewInfo* DetailViewPanel::getViewInfo(PropertyViewHandle handle)
{
	int index = mPropertyViews.findIndexPred([handle](PropertyViewInfo const& info)
	{
		return info.handle == handle;
	});
	if (index == INDEX_NONE)
		return nullptr;

	return &mPropertyViews[index];
}

PropertyViewHandle DetailViewPanel::addView(Reflection::EPropertyType type, void* ptr, char const* name, char const* category)
{
	PropertyViewInfo info;
	if (name)
		info.name = name;
	
	info.categoryIndex = getOrAddCategoryIndex(category);
	info.handle = ++mNextHandle;
	info.type = EViewType::Primitive;
	info.ptr = ptr;
	info.primitiveType = type;
	mPropertyViews.push_back(std::move(info));
	return mPropertyViews.back().handle;
}

PropertyViewHandle DetailViewPanel::addView(Reflection::StructType* structData, void* ptr, char const* name, char const* category)
{
	PropertyViewInfo info;
	if (name)
		info.name = name;

	info.categoryIndex = getOrAddCategoryIndex(category);
	info.handle = ++mNextHandle;
	info.type = EViewType::Struct;
	info.ptr = ptr;
	info.structData = structData;
	mPropertyViews.push_back(std::move(info));
	return mPropertyViews.back().handle;
}

PropertyViewHandle DetailViewPanel::addView(Reflection::EnumType* enumData, void* ptr, char const* name, char const* category)
{
	PropertyViewInfo info;
	if (name)
		info.name = name;

	info.categoryIndex = getOrAddCategoryIndex(category);
	info.handle = ++mNextHandle;
	info.type = EViewType::Enum;
	info.ptr = ptr;
	info.enumData = enumData;
	mPropertyViews.push_back(std::move(info));
	return mPropertyViews.back().handle;
}

PropertyViewHandle DetailViewPanel::addView(Reflection::PropertyBase* property, void* ptr, char const* name, char const* category)
{
	CHECK(property);

	PropertyViewInfo info;
	if (name)
		info.name = name;
	
	info.categoryIndex = getOrAddCategoryIndex(category);
	info.handle = ++mNextHandle;
	info.type = EViewType::Property;
	info.ptr = ptr;
	info.property = property;
	mPropertyViews.push_back(std::move(info));
	return mPropertyViews.back().handle;
}

void DetailViewPanel::addCategory(char const* name, bool bDefaultOpen)
{
	int index = getOrAddCategoryIndex(name);
	if (index != INDEX_NONE)
	{
		mCategories[index].bDefaultOpen = bDefaultOpen;
	}
}

void DetailViewPanel::addCallback(PropertyViewHandle handle, std::function<void(char const*)> const& callback)
{
	auto view = getViewInfo(handle);
	if (view)
	{
		view->callback = callback;
	}
}

void DetailViewPanel::addCategoryCallback(char const* category, std::function<void(char const*)> const& callback)
{
	int index = getCategoryIndex(category);
	if (index != INDEX_NONE)
	{
		mCategories[index].callback = callback;
	}
}

void DetailViewPanel::removeView(PropertyViewHandle handle)
{
	mPropertyViews.removePred([handle](PropertyViewInfo const& info)
	{
		return info.handle == handle;
	});
}

void DetailViewPanel::clearAllViews()
{
	mPropertyViews.clear();
	mCategories.clear();
}

void DetailViewPanel::clearCategoryViews(char const* name)
{
	if (!name)
		return;

	int catIndex = getCategoryIndex(name);
	if (catIndex == INDEX_NONE)
		return;

	mPropertyViews.removePred([catIndex](PropertyViewInfo const& info)
	{
		return info.categoryIndex == catIndex;
	});

	mCategories.removeIndex(catIndex);

	for (auto& view : mPropertyViews)
	{
		if (view.categoryIndex != INDEX_NONE && view.categoryIndex > catIndex)
		{
			view.categoryIndex--;
		}
	}
}

int DetailViewPanel::getOrAddCategoryIndex(char const* name)
{
	if (!name)
		return INDEX_NONE;

	int index = getCategoryIndex(name);
	if (index != INDEX_NONE)
		return index;

	CategoryInfo cat;
	cat.name = name;
	mCategories.push_back(std::move(cat));
	return mCategories.size() - 1;
}

int DetailViewPanel::getCategoryIndex(char const* name) const
{
	if (!name)
		return INDEX_NONE;

	return mCategories.findIndexPred([name](CategoryInfo const& cat)
	{
		return cat.name == name;
	});
}

