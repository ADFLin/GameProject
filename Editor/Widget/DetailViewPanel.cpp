
#include "DetailViewPanel.h"

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

int ToImGuiDataType(EPropertyType type)
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


bool RenderPrimitivePropertyValue(Reflection::EPropertyType type, void* pData)
{
	using namespace Reflection;

	bool bChanged = false;
	switch (type)
	{
	case EPropertyType::Uint8:
	case EPropertyType::Uint16:
	case EPropertyType::Uint32:
	case EPropertyType::Uint64:
	case EPropertyType::Int8:
	case EPropertyType::Int16:
	case EPropertyType::Int32:
	case EPropertyType::Int64:
		{
			ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue;
			bChanged = ImGui::InputScalar("##Property", ToImGuiDataType(type), pData, NULL, NULL, "%d", flags);
		}
		break;
	case EPropertyType::Bool:
		{
			bChanged = ImGui::Checkbox("##Property", (bool*)pData);
		}
		break;
	case EPropertyType::Float:
		{
			ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue;
			bChanged = ImGui::InputFloat("##Property", (float*)pData, 0.0f, 0.0f, "%.3f", flags);
		}
		break;
	case EPropertyType::Double:
		{
			ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue;
			bChanged = ImGui::InputDouble("##Property", (double*)pData, 0.0f, 0.0f, "%.3f", flags);
		}
		break;
	}
	return bChanged;
}

bool RenderEnumValue(EnumType* enumData, void* pData)
{
	bool bChanged = false;
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
			const bool is_selected = (selection == index);
			if (ImGui::Selectable(value.text, is_selected))
			{
				selection = index;
				enumData->setValue(pData, value.value);
				bChanged = true;
			}
			if (is_selected)
				ImGui::SetItemDefaultFocus();
			++index;
		}
		ImGui::EndCombo();
	}
	return bChanged;
}

bool RenderPropertyValue(EDisplayType displayType, void* pData, Reflection::PropertyBase* property = nullptr)
{
	bool bChanged = false;
	switch (displayType)
	{
	case EDisplayType::Primitive:
		{
			bChanged = RenderPrimitivePropertyValue(property->getType(), pData);
		}
		break;
	case EDisplayType::Vector2:
		{
			ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue;
			bChanged = ImGui::InputFloat2("##Property", (float*)pData, "%.3f", flags);
		}
		break;
	case EDisplayType::Vector3:
		{
			ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue;
			bChanged = ImGui::InputFloat3("##Property", (float*)pData, "%.3f", flags);
		}
		break;
	case EDisplayType::Color3f:
		{
			ImGuiColorEditFlags flags =
				ImGuiColorEditFlags_Float |
				ImGuiColorEditFlags_InputRGB |
				ImGuiColorEditFlags_DisplayRGB |
				ImGuiColorEditFlags_PickerHueBar;
			bChanged = ImGui::ColorEdit3("##Property", (float*)pData, flags);
		}
		break;
	case EDisplayType::Color4f:
		{
			ImGuiColorEditFlags flags =
				ImGuiColorEditFlags_Float |
				ImGuiColorEditFlags_InputRGB |
				ImGuiColorEditFlags_DisplayRGB |
				ImGuiColorEditFlags_PickerHueBar;
			bChanged = ImGui::ColorEdit4("##Property", (float*)pData, flags);
		}
		break;
	case EDisplayType::Enum:
		{
			auto enumProperty = static_cast<EnumProperty*>(property);
			if (enumProperty->enumData)
			{
				bChanged = RenderEnumValue(enumProperty->enumData, pData);
			}
			else
			{
				bChanged = RenderPrimitivePropertyValue(enumProperty->underlyingType, pData);
			}
		}
		break;
	}
	return bChanged;
}

bool RenderStruct(Reflection::StructType* structData, void* pData, int level = 0)
{
	bool bChanged = false;

	ImGui::PushID(pData);

	for (auto property : structData->properties)
	{
		ImGui::PushID(property);

		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::AlignTextToFramePadding();

		EDisplayType displayType = GetDisplayType(property);

		if (displayType == EDisplayType::Struct)
		{
			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen;
			if (ImGui::TreeNodeEx(property->name, flags))
			{
				ImGui::TableSetColumnIndex(1);
				ImGui::SetNextItemWidth(-FLT_MIN);
				void* structPtr = property->getDataInStruct(pData);
				auto structProperty = static_cast<StructProperty*>(property);
				bChanged = RenderStruct(structProperty->structData, structPtr, level + 1);
				ImGui::TreePop();
			}
		}
		else
		{
			ImGui::TextUnformatted(property->name);

			ImGui::TableSetColumnIndex(1);
			ImGui::SetNextItemWidth(-FLT_MIN);

			RenderPropertyValue(displayType, property->getDataInStruct(pData), property);
		}

		ImGui::NextColumn();
		ImGui::PopID();
	}
	ImGui::PopID();
	return bChanged;
}

void DetailViewPanel::render()
{
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
	if (ImGui::BeginTable("split", 2, ImGuiTableFlags_Resizable))
	{
		for (auto const propertyView : mPropertyViews)
		{
			switch (propertyView.type)
			{
			case EPropertyType::Struct:
				{
					RenderStruct(propertyView.structData, propertyView.ptr);
				}
				break;
			case EPropertyType::Enum:
				{
					RenderEnumValue(propertyView.enumData, propertyView.ptr);
				}
				break;
			default:
				RenderPrimitivePropertyValue(propertyView.type, propertyView.ptr);
				break;
			}
		}
		ImGui::EndTable();
	}
	ImGui::PopStyleVar();
}

void DetailViewPanel::clearProperties()
{
	mPropertyViews.clear();
}

void DetailViewPanel::addPrimitive(Reflection::EPropertyType type, void* ptr)
{
	PropertyViewInfo info;
	info.type = type;
	info.ptr = ptr;
	mPropertyViews.push_back(info);
}

void DetailViewPanel::addStruct(Reflection::StructType* structData, void* ptr)
{
	PropertyViewInfo info;
	info.type = EPropertyType::Struct;
	info.ptr = ptr;
	info.structData = structData;
	mPropertyViews.push_back(info);
}

void DetailViewPanel::addEnum(Reflection::EnumType* enumData, void* ptr)
{
	PropertyViewInfo info;
	info.type = EPropertyType::Enum;
	info.ptr = ptr;
	info.enumData = enumData;
	mPropertyViews.push_back(info);
}
