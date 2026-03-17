#pragma once
#ifndef Editor_H_54F7990F_F10C_4F74_A15A_6823A63F68F4
#define Editor_H_54F7990F_F10C_4F74_A15A_6823A63F68F4

#include "EditorConfig.h"

#include "CompilerConfig.h"
#include "SystemMessage.h"
#include "RHI/RHICommon.h"
#include "RHI/RHIGraphics2D.h"
#include "Reflection.h"

#include "DetailCustomization.h"

class BitmapDC;
namespace Render
{
	class ITextureShowManager;
	class RHIFrameBuffer;
}


class IEditorViewportUpdateContext
{
public:
	virtual ~IEditorViewportUpdateContext() = default;
	virtual void setRenderTexture(Render::RHITexture2D& texture) = 0;
	virtual void copyToRenderTarget(BitmapDC& bufferDC) = 0;

};

class IEditorViewportRenderContext
{
public:
	virtual ~IEditorViewportRenderContext() = default;

	Render::RHIFrameBuffer* frameBuffer;
	RHIGraphics2D* graphics;
};

class IEditorGameViewport
{
public:
	virtual TVector2<int> getInitialSize() = 0;
	virtual void resizeViewport(int w, int h) = 0;
	virtual void updateViewport(IEditorViewportUpdateContext& context){}
	virtual void renderViewport(IEditorViewportRenderContext& context){}
	virtual void onViewportMouseEvent(MouseMsg const& msg) = 0;
	virtual void onViewportKeyEvent(unsigned key, bool isDown) = 0;
	virtual void onViewportCharEvent(unsigned code) = 0;
};


struct DetailViewConfig
{
	char const* name = nullptr;
};

typedef uint32 PropertyViewHandle;


class IEditorDetailView
{
public:
	virtual ~IEditorDetailView() = default;
	virtual void release() = 0;

	virtual PropertyViewHandle addView(Reflection::EPropertyType type, void* ptr, char const* name = nullptr, char const* category = nullptr) = 0;
	virtual PropertyViewHandle addView(Reflection::StructType* structData, void* ptr, char const* name = nullptr, char const* category = nullptr) = 0;
	virtual PropertyViewHandle addView(Reflection::EnumType* enumData, void* ptr, char const* name = nullptr, char const* category = nullptr) = 0;
	virtual PropertyViewHandle addView(Reflection::PropertyBase* property, void* ptr, char const* name = nullptr, char const* category = nullptr) = 0;

	virtual void addCallback(PropertyViewHandle handle, std::function<void(char const*)> const& callback) = 0;
	virtual void addCategoryCallback(char const* category, std::function<void(char const*)> const& callback) = 0;
	virtual void removeView(PropertyViewHandle handle) = 0;
	virtual void clearAllViews() = 0;
	virtual void clearCategoryViews(char const* name) = 0;

	// Set the active category for subsequent addView / addValue / addStruct calls.
	virtual void setCategory(char const* name, bool bDefaultOpen = true) = 0;
	// Remove all views belonging to the given category and unregister that category.

	// Stop tagging subsequent calls to any category (equivalent to setCategory(nullptr)).
	void clearCategory() { setCategory(nullptr); }

	template< typename TStruct >
	PropertyViewHandle addStruct(TStruct& data, char const* name = nullptr)
	{
		return addView(Reflection::GetStructType<TStruct>(), &data, name, mCurrentCategory);
	}

	template< typename T , TEnableIf_Type<Meta::IsPrimary<T>::Value , bool > = true >
	PropertyViewHandle addValue(T& value, char const* name = nullptr)
	{
		return addView(Reflection::PrimaryTypeTraits<T>::Type, &value, name, mCurrentCategory);
	}

	template< typename T, TEnableIf_Type<std::is_enum_v<T>, bool > = true >
	PropertyViewHandle addValue(T& value, char const* name = nullptr)
	{
		return addView(Reflection::GetEnumType<T>(), &value, name, mCurrentCategory);
	}

	template< typename T, TEnableIf_Type<!Meta::IsPrimary<T>::Value, bool > = true >
	PropertyViewHandle addValue(T& value, char const* name = nullptr)
	{
		auto property = Reflection::PropertyCollector::GetProperty<T>();
		return addView(property, &value, name, mCurrentCategory);
	}

protected:
	char const* mCurrentCategory = nullptr;
};

struct ToolBarConfig
{
	char const* name = nullptr;
};

class IEditorToolBar
{
public:
	virtual ~IEditorToolBar() {}
	virtual void release() = 0;
	virtual void addButton(char const* label, std::function<void()> const& onClick) = 0;
	virtual void addSeparator() = 0;
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
	virtual void removeGameViewport(IEditorGameViewport* viewport) = 0;
	virtual void setTextureShowManager(Render::ITextureShowManager* manager) = 0;
	virtual IEditorDetailView* createDetailView(DetailViewConfig const& config) = 0;
	virtual IEditorToolBar* createToolBar(ToolBarConfig const& config) = 0;

	virtual void registerCustomization(Reflection::StructType* type, std::shared_ptr<IDetailCustomization> customization) = 0;
};

#endif // Editor_H_54F7990F_F10C_4F74_A15A_6823A63F68F4