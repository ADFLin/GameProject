#pragma once
#ifndef GameViewportPanel_H_54F14969_16D6_40F8_9F97_E1449B7025E0
#define GameViewportPanel_H_54F14969_16D6_40F8_9F97_E1449B7025E0

#include "EditorPanel.h"
#include "RHI/RHICommon.h"

class IEditorGameViewport;

class GameViewportPanel : public IEditorPanel
{
public:

	IEditorGameViewport* mViewport = nullptr;
	Render::RHITexture2DRef mTexture;

	void onOpen() override;
	void onClose() override;

	void render() override;

	uint16 mMouseState;

};


#endif // GameViewportPanel_H_54F14969_16D6_40F8_9F97_E1449B7025E0
