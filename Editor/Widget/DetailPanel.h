#pragma once
#ifndef DetailPanel_H_B3999968_7BC2_4C9B_8C42_E43EF34171CA
#define DetailPanel_H_B3999968_7BC2_4C9B_8C42_E43EF34171CA

#include "EditorPanel.h"
#include "ReflectionCollect.h"



struct TestData
{
	float floatValue = 1.0f;
	int   intValue = 2;

	REFLECT_OBJECT_BEGIN(TestData)
		REF_PROPERTY(floatValue)
		REF_PROPERTY(intValue)
	REFLECT_OBJECT_END()
};

namespace Reflection
{
	enum EPropertyType
	{
		Uint32,
		Int32,
		Float,
		Struct,

	};

	class PropertyBase
	{
	public:
		virtual EPropertyType getType() = 0;
	
	};

	template< typename T >
	struct PrimaryTypeTraits
	{
	};

	template< typename T >
	class TPrimaryTypePorperty : public PropertyBase
	{

	};

	struct StructType
	{


		TArray<PropertyBase*> Properties;
	};
	class PropertyCollector
	{
	public:
		template< class T >
		void beginClass(char const* name)
		{

		}

		template< typename T, typename Base >
		void addBaseClass()
		{

		}

		template< class T , typename P >
		void addProperty(P (T::*Member) , char const* name)
		{
		}

		void endClass()
		{

		}

		StructType* mOwner;
	};




}


class PropertyCollector
{
	template< class T >
	void beginClass(char const* name);

	template< typename T, typename Base >
	void addBaseClass();

	template< class T >
	void addProperty(T var, char const* name);

	void endClass();
};

class DetailPanel : public IEditorPanel
{
public:

	void render()
	{
		if (ImGui::SmallButton("Add Debug Text"))
		{
		}

		ImGui::InputFloat("##value", &data.floatValue, 0.0f);
		ImGui::InputInt("##value2", &data.intValue, 1);
	}

	TestData data;
};
#endif // DetailPanel_H_B3999968_7BC2_4C9B_8C42_E43EF34171CA