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

namespace Reflection
{
	class StructType;
}

class IEditorDetailView
{
public:
	virtual void release() = 0;

	virtual void addPrimitive(Reflection::EPropertyType type, void* ptr) = 0;
	virtual void addStruct(Reflection::StructType* structData, void* ptr) = 0;
	virtual void addEnum(Reflection::EnumType* enumData, void* ptr) = 0;
	virtual void clearProperties() = 0;

	template< typename TStruct >
	void addStruct(TStruct& data)
	{
		addStruct(Reflection::GetStructType<TStruct>(), &data);
	}
	template< typename T , TEnableIf_Type<Meta::IsPrimary<T>::Value , bool > = true >
	void addValue(T& value)
	{
		addPrimitive(Reflection::PrimaryTypeTraits<T>::Type, &value);
	}
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