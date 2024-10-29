#pragma once
#ifndef TextureViewerPanel_H_8D8B3DB6_CAA9_42A8_AF55_1145256DF38A
#define TextureViewerPanel_H_8D8B3DB6_CAA9_42A8_AF55_1145256DF38A

#include "EditorPanel.h"

namespace Render
{
	class ITextureShowManager;
}

class TextureViewerPanel : public IEditorPanel
{

public:

	static constexpr char const* ClassName = "TextureViewer";

	Render::ITextureShowManager* mTextureShowManager = nullptr;
	float sizeScale = 1.0;
	bool  bUseAlpha = false;
	bool  bMaskR = true;
	bool  bMaskG = true;
	bool  bMaskB = true;
	bool  bMaskA = true;
	bool  bGrayScale = true;
	bool  bVFlip = false;
	float gamma = 1.0f;
	float scale = 1.0f;
	
	int selection = 0;
	
	void render() override;
	void getRenderParams(WindowRenderParams& params) const override;

};

#endif // TextureViewerPanel_H_8D8B3DB6_CAA9_42A8_AF55_1145256DF38A
