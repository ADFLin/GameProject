#pragma once
#ifndef SpriteSheetPanel_H_
#define SpriteSheetPanel_H_

#include "../EditorPanel.h"
#include "Renderer/SpriteRender.h"
#include "Math/Vector2.h"
#include "HashString.h"
#include <string>

#include <vector>
#include <algorithm>

using namespace Math;

class SpriteSheetPanel : public IEditorPanel
{
public:
	static constexpr char const* ClassName = "SpriteSheet";

	SpriteSheetPanel();
	virtual ~SpriteSheetPanel() = default;

	void update() override;
	void getUpdateParams(WindowUpdateParams& params) const override;

private:
	SpriteSheet* mEditedSheet = nullptr;
	std::string mTexturePath;

	float mZoomScale = 1.0f;
	float mRegionListHeight = 300.0f;
	float mAnimListWidth = 150.0f;
	std::vector<int> mSelectedRegions;

	HashString mSelectedAnimName;
	int mSelectedFrameIndex = -1;
	bool mIsPlaying = false;
	float mAnimTimer = 0.0f;
	int mCurrentPlaybackFrame = 0;

	int mGridRows = 1;
	int mGridCols = 1;
	int mCellSpacingX = 0;
	int mCellSpacingY = 0;

	bool mIsDraggingNew = false;
	bool mShowSlicePreview = false;
	Math::Vector2 mDragStartPos;
	int mDragMode = 0; // 0: None, 1: Create, 2: Move, 3: Resize RB

	bool mShowGrid = false;
	bool mSnapToGrid = false;
	int mGridSpacing = 16;

	int mPendingFocusRegionIndex = -1;
	void drawToolBar();
	void drawPropertiesPanel();
	void drawTexturePreview();
	void drawAnimEditor();
};

#endif // SpriteSheetPanel_H_
