#pragma once
#ifndef Editor_H_54F7990F_F10C_4F74_A15A_6823A63F68F4
#define Editor_H_54F7990F_F10C_4F74_A15A_6823A63F68F4

#include "EditorConfig.h"

#include "CompilerConfig.h"
#include "SystemMessage.h"
#include "RHI/RHICommon.h"
#include "RHI/RHIGraphics2D.h"
#include "Reflection.h"

namespace Render
{
	class ITextureShowManager;
}

class IEditorRenderContext
{
public:
	virtual ~IEditorRenderContext() = default;
	virtual void copyToRenderTarget(void* SrcHandle) = 0;
	virtual void setRenderTexture(Render::RHITexture2D& texture) = 0;
};

class IEditorGameViewport
{
public:
	virtual void resizeViewport(int w, int h) = 0;
	virtual void renderViewport(IEditorRenderContext& context) = 0;
	virtual void onViewportMouseEvent(MouseMsg const& msg) = 0;
};


struct DetailViewConfig
{
	char const* name = nullptr;
};

typedef uint32 PropertyViewHandle;

class IEditorDetailView
{
public:
	virtual void release() = 0;

	virtual PropertyViewHandle addView(Reflection::EPropertyType type, void* ptr, char const* name = nullptr) = 0;
	virtual PropertyViewHandle addView(Reflection::StructType* structData, void* ptr, char const* name = nullptr) = 0;
	virtual PropertyViewHandle addView(Reflection::EnumType* enumData, void* ptr, char const* name = nullptr) = 0;
	virtual PropertyViewHandle addView(Reflection::PropertyBase* property, void* ptr, char const* name = nullptr) = 0;

	virtual void addCallback(PropertyViewHandle handle, std::function<void(char const*)> const& callback) = 0;
	virtual void removeView(PropertyViewHandle handle) = 0;
	virtual void clearAllViews() = 0;

	template< typename TStruct >
	PropertyViewHandle addStruct(TStruct& data, char const* name = nullptr)
	{
		return addView(Reflection::GetStructType<TStruct>(), &data, name);
	}

	template< typename T , TEnableIf_Type<Meta::IsPrimary<T>::Value , bool > = true >
	PropertyViewHandle addValue(T& value, char const* name = nullptr)
	{
		return addView(Reflection::PrimaryTypeTraits<T>::Type, &value, name);
	}

	template< typename T, TEnableIf_Type<std::is_enum_v<T>, bool > = true >
	PropertyViewHandle addValue(T& value, char const* name = nullptr)
	{
		return addView(Reflection::GetEnumType<T>(), &value, name);
	}

	template< typename T, TEnableIf_Type<!Meta::IsPrimary<T>::Value, bool > = true >
	PropertyViewHandle addValue(T& value, char const* name = nullptr)
	{
		auto property = Reflection::PropertyCollector::CreateProperty<T>();
		mProperties.push_back(property);
		return addView(property, &value, name);
	}

	virtual ~IEditorDetailView()
	{
		for (auto property : mProperties)
		{
			delete property;
		}
	}

	TArray< Reflection::PropertyBase* > mProperties;
};

class IEditor
{
public:
	static EDITOR_API IEditor* Create();

	virtual ~IEditor(){}
	virtual void release() = 0;
	virtual bool initializeRender() = 0;
	virtual void update() = 0;
	virtual void render() = 0;
	virtual void addGameViewport(IEditorGameViewport* viewport) = 0;
	virtual void setTextureShowManager(Render::ITextureShowManager* manager) = 0;
	virtual IEditorDetailView* createDetailView(DetailViewConfig const& config) = 0;
};

#endif // Editor_H_54F7990F_F10C_4F74_A15A_6823A63F68F4