#pragma once
#ifndef ProfilerPanel_H_C1D3393C_A1C9_4743_A49C_E082A42DFAE0
#define ProfilerPanel_H_C1D3393C_A1C9_4743_A49C_E082A42DFAE0

#include "EditorPanel.h"
#include "Math/Vector2.h"

class ProfilerPanel : public IEditorPanel
{
public:
	void update();
	void getUpdateParams(WindowUpdateParams& params) const override;
};

class ProfilerListPanel : public IEditorPanel
{
public:
	void update();
};


#endif // ProfilerPanel_H_C1D3393C_A1C9_4743_A49C_E082A42DFAE0