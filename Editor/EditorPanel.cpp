#include "EditorPanel.h"

#if EDITOR_EXPORT

TArray< EditorPanelInfo >& EditorPanelInfo::GetList()
{
	static TArray< EditorPanelInfo > List;
	return List;
}

#endif