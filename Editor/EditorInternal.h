#pragma once

#ifndef EditorInternal_h__
#define EditorInternal_h__

#include "Editor.h"
#include "EditorPanel.h"

extern class EditorInternal* GEditor;

class EditorInternal : public IEditor
{
public:
	
	EditorInternal()
	{
		GEditor = this;
	}


	void destroyPanel(IEditorPanel* widget)
	{
		int index = mPanels.findIndexPred([widget](ActivePanel const& panel)
		{
			return panel.widget == widget;
		});

		if (index != INDEX_NONE)
		{
			mPanels.removeIndex(index);
			delete widget;
		}
	}


	struct ActivePanel
	{
		std::string name;
		EditorPanelInfo const* info;
		IEditorPanel* widget = nullptr;
		bool bOpenRequest = false;
		bool bOpened = false;
		bool bShown = false;
	};

	TArray< ActivePanel > mPanels;

	RHIGraphics2D* mGraphics = nullptr;
	IDetailCustomization* findCustomization(Reflection::StructType* type);

	std::unordered_map< Reflection::StructType*, std::shared_ptr<IDetailCustomization> > mCustomizations;
};




#endif // EditorInternal_h__