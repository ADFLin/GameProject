#include "EditorPanel.h"
#include "EditorRender.h"

#if EDITOR_EXPORT

TArray< EditorPanelInfo >& EditorPanelInfo::GetList()
{
	static TArray< EditorPanelInfo > List;
	return List;
}

#endif

void EditorRenderGloabal::RestoreState()
{
	GEditorRenderer->restoreRenderState();
}
