#include "MaterialGraphPanel.h"

REGISTER_EDITOR_PANEL(MaterialGraphPanel, "MaterialGraph", true, true);


void MaterialGraphPanel::onOpen()
{
	if (mNodeContext == nullptr)
	{
		mNodeContext = ImNode::CreateEditor();
	}
}

void MaterialGraphPanel::onClose()
{
#if 0
	ImNode::DestroyEditor(mNodeContext);
	mNodeContext = nullptr;
#endif
}

