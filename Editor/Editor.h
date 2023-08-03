#pragma once
#ifndef Editor_H_54F7990F_F10C_4F74_A15A_6823A63F68F4
#define Editor_H_54F7990F_F10C_4F74_A15A_6823A63F68F4

#include "EditorConfig.h"

#include "CompilerConfig.h"
#include "SystemMessage.h"
#include "RHI/RHICommon.h"
#include "RHI/RHIGraphics2D.h"


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
};

#endif // Editor_H_54F7990F_F10C_4F74_A15A_6823A63F68F4