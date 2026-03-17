#include "EditorPanel.h"
#include "EditorRender.h"

#include "EditorInternal.h"

#if EDITOR_EXPORT
TArray< EditorPanelInfo >& EditorPanelInfo::GetList()
{
	static TArray< EditorPanelInfo > List;
	return List;
}
#endif

EditorRenderGloabal& EditorRenderGloabal::Get()
{
	static EditorRenderGloabal StaticInstance;
	return StaticInstance;
}

void EditorRenderGloabal::RestoreState()
{
	GEditorRenderer->restoreRenderState();
}

RHIGraphics2D& EditorRenderGloabal::getGraphics()
{
	return *GEditor->mGraphics;
}

