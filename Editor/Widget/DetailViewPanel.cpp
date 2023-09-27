
#include "DetailViewPanel.h"
#include "InlineString.h"

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
			else
			{
				return EDisplayType::Struct;
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
	RenderContext(PropertyViewInfo& propertyView)
		:propertyView(propertyView)
	{


	}

	PropertyViewInfo& propertyView;
	char const* propertyName = nullptr;


	void verify(bool bChanged)
	{
		if (bChanged)
		{
			if (propertyView.callback)
			{
				propertyView.callback(propertyName);
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
			verify(ImGui::InputScalar("##Property", ScalarType, pData, NULL, NULL, "%d", flags));
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
					verify(ImGui::InputFloat("##Property", (float*)pData, 0.0f, 0.0f, "%.3f", flags));
				}
			}
			break;
		case EPropertyType::Double:
			{
				ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue;
				verify(ImGui::InputDouble("##Property", (double*)pData, 0.0f, 0.0f, "%.3f", flags));
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
				verify(ImGui::InputFloat2("##Property", (float*)pData, "%.3f", flags));
			}
			break;
		case EDisplayType::Vector3:
			{
				ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue;
				verify(ImGui::InputFloat3("##Property", (float*)pData, "%.3f", flags));
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
		}
	}

	void renderStructRow(char const* name, StructType* structData, void* pData, int level)
	{
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen;
		bool bOpened = ImGui::TreeNodeEx(name, flags);
		ImGui::TableSetColumnIndex(1);
		ImGui::SetNextItemWidth(-FLT_MIN);
		ImGui::Text("%u members", structData->properties.size());
		if (bOpened)
		{
			renderStructContentRow(structData, pData, level + 1);
			ImGui::TreePop();
		}
	}

	void renderPropertyRow(char const* name, Reflection::PropertyBase* property, void* pData, int level)
	{
		EDisplayType displayType = GetDisplayType(property);

		switch (displayType)
		{
		case EDisplayType::Struct:
			{
				void* structPtr = property->getDataInStruct(pData);
				StructProperty* structProperty = static_cast<StructProperty*>(property);
				renderStructRow(name, structProperty->structData, structPtr, level);
			}
			break;
		case EDisplayType::Array:
			{
				ArrayPropertyBase* arrayProperty = static_cast<ArrayPropertyBase*>(property);
				void* arrayPtr = arrayProperty->getDataInStruct(pData);
				int count = arrayProperty->getElementCount(arrayPtr);

				ImGuiTreeNodeFlags flags = 0;
				bool bOpened = ImGui::TreeNodeEx(name, flags);

				ImGui::TableSetColumnIndex(1);
				ImGui::SetNextItemWidth(-FLT_MIN);
				ImGui::Text("%d Array elements", count);

				if (bOpened)
				{
					for (int index = 0; index < count; ++index)
					{
						ImGui::TableNextRow();

						void* elementPtr = arrayProperty->getElement(arrayPtr, index);
						ImGui::PushID(elementPtr);
						ImGui::TableSetColumnIndex(0);
						ImGui::AlignTextToFramePadding();
						InlineString<256> str;
						str.format("index[%d]", index);
						renderPropertyRow(str, arrayProperty->elementProperty, elementPtr, level + 1);
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

void DetailViewPanel::render()
{
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
	if (ImGui::BeginTable("split", 2, ImGuiTableFlags_Resizable))
	{
		for (auto& propertyView : mPropertyViews)
		{
			RenderContext context(propertyView);
			context.render();
		}
		ImGui::EndTable();
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

PropertyViewHandle DetailViewPanel::addView(Reflection::EPropertyType type, void* ptr, char const* name)
{
	PropertyViewInfo info;
	if (name)
		info.name = name;

	info.type = EViewType::Primitive;
	info.ptr = ptr;
	info.primitiveType = type;
	mPropertyViews.push_back(std::move(info));
	return info.handle;
}

PropertyViewHandle DetailViewPanel::addView(Reflection::StructType* structData, void* ptr, char const* name)
{
	PropertyViewInfo info;
	if (name)
		info.name = name;

	info.handle = ++mNextHandle;
	info.type = EViewType::Struct;
	info.ptr = ptr;
	info.structData = structData;
	mPropertyViews.push_back(std::move(info));
	return info.handle;
}

PropertyViewHandle DetailViewPanel::addView(Reflection::EnumType* enumData, void* ptr, char const* name)
{
	PropertyViewInfo info;
	if (name)
		info.name = name;

	info.handle = ++mNextHandle;
	info.type = EViewType::Enum;
	info.ptr = ptr;
	info.enumData = enumData;
	mPropertyViews.push_back(std::move(info));
	return info.handle;
}

PropertyViewHandle DetailViewPanel::addView(Reflection::PropertyBase* property, void* ptr, char const* name)
{
	CHECK(property);

	PropertyViewInfo info;
	if (name)
		info.name = name;

	info.handle = ++mNextHandle;
	info.type = EViewType::Property;
	info.ptr = ptr;
	info.property = property;
	mPropertyViews.push_back(std::move(info));
	return info.handle;
}

void DetailViewPanel::addCallback(PropertyViewHandle handle, std::function<void(char const*)> const& callback)
{
	auto view = getViewInfo(handle);
	if (view)
	{
		view->callback = callback;
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
}
