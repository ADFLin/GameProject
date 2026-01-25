#include "CodeEditor.h"

#include "StringParse.h"

#include "GameGlobal.h"
#include "GameGUISystem.h"

#include <sstream>

CodeEditorSettings GEditorSettings;

CodeEditorSettings::CodeEditorSettings()
{
	Color3ub DefaultKeywordColor{ 255, 188, 66 };
	Color3ub FlowKeywordColor{ 255, 188, 66 };
	mWordColorMap["if"] = FlowKeywordColor;
	mWordColorMap["elseif"] = FlowKeywordColor;
	mWordColorMap["else"] = FlowKeywordColor;
	mWordColorMap["end"] = FlowKeywordColor;
	mWordColorMap["goto"] = FlowKeywordColor;
	mWordColorMap["repeat"] = FlowKeywordColor;
	mWordColorMap["for"] = FlowKeywordColor;
	mWordColorMap["while"] = FlowKeywordColor;
	mWordColorMap["do"] = FlowKeywordColor;
	mWordColorMap["until"] = FlowKeywordColor;
	mWordColorMap["return"] = FlowKeywordColor;
	mWordColorMap["then"] = FlowKeywordColor;
	mWordColorMap["break"] = FlowKeywordColor;
	mWordColorMap["in"] = FlowKeywordColor;

	mWordColorMap["function"] = DefaultKeywordColor;
	mWordColorMap["local"] = DefaultKeywordColor;

	mWordColorMap["true"] = DefaultKeywordColor;
	mWordColorMap["false"] = DefaultKeywordColor;
	mWordColorMap["nil"] = DefaultKeywordColor;

	mWordColorMap["and"] = DefaultKeywordColor;
	mWordColorMap["or"] = DefaultKeywordColor;
	mWordColorMap["not"] = DefaultKeywordColor;

	Color3ub StringColor{ 206, 145, 120 }; // Orange/Brown
	Color3ub NumberColor{ 181, 206, 168 }; // Light Green
	Color3ub CommentColor{ 106, 153, 85 }; // Green
	Color3ub OperatorColor{ 220, 220, 220 }; // White

	mWordColorMap["string"] = StringColor;
	mWordColorMap["number"] = NumberColor;
	mWordColorMap["comment"] = CommentColor;
	mWordColorMap["operator"] = OperatorColor;

	mFont = &RenderUtility::GetFontDrawer(FONT_S10);
}

CodeEditorBase::CodeEditorBase(int id, Vec2i const& pos, Vec2i const& size, GWidget* parent) 
	:BaseClass(id, pos, size, parent)
{
	bCanResize = true;
	Vec2i buttonSize(ButtonSizeX, ButtonSizeY);
	mMinSize = Vec2i(MinSizeX, MinSizeY);
	mExecButton = new ExecButton(UI_EXEC, Vec2i(Margin, Margin), buttonSize, this);
	mExecButton->isExecutingFunc = []() { return TFWR::IViewModel::Get().isExecutingCode(); };
	mExecStepButton = new ExecButton(UI_EXEC_STEP, Vec2i(2 * Margin + buttonSize.x, Margin), buttonSize, this);
	mExecStepButton->bStep = true;
	mExecStepButton->isExecutingFunc = mExecButton->isExecutingFunc;
	mMinimizeButton = new MinimizeButton(UI_MINIMIZE_BUTTON, Vec2i(size.x - Margin - 2 * buttonSize.x - Margin, Margin), buttonSize, this);
	mCloseButton = new CloseButton(UI_CLOSE_BUTTON, Vec2i(size.x - Margin - buttonSize.x, Margin), buttonSize, this);

	// Create Canvas
	Vec2i textRectSize(size.x - 2 * Border, size.y - 2 * Border - HeaderHeight);
	Vec2i canvasPos(Border, Border + HeaderHeight);
	mCanvas = new CodeEditCanvas(UI_CANVAS, canvasPos, textRectSize, this);
	mCanvas->setSendParentMouseMsg();

	mValueTooltip = new ValueTooltip(0, Vec2i(0, 0), nullptr);
	mValueTooltip->show(false);
	::Global::GUI().addWidget(mValueTooltip);
}

void CodeEditorBase::loadFromCode(std::string const& code)
{
	mCanvas->loadFromCode(code);
}



MsgReply CodeEditorBase::onKeyMsg(KeyMsg const& msg)
{
	return BaseClass::onKeyMsg(msg);
}

MsgReply CodeEditorBase::onCharMsg(unsigned code)
{
	return BaseClass::onCharMsg(code);
}

MsgReply CodeEditorBase::onMouseMsg(MouseMsg const& msg)
{
	if (msg.onMoving())
	{
		Vec2i localPos = msg.getPos() - getWorldPos();
		if (mHoverMousePos != localPos)
		{
			mValueTooltip->show(false);
			mHoverMousePos = localPos;
			mHoverTimer = mCanvas->mBlinkTimer;
			mShouldCheckHover = true;
		}
	}
	return BaseClass::onMouseMsg(msg);
}



void CodeEditorBase::onResize(Vec2i const& size)
{
	// Update MinimizeButton position
	Vec2i buttonSize(ButtonSizeX, ButtonSizeY);
	mMinimizeButton->setPos(Vec2i(size.x - Margin - 2 * buttonSize.x - Margin, Margin));
	mCloseButton->setPos(Vec2i(size.x - Margin - buttonSize.x, Margin));

	Vec2i textRectSize(size.x - 2 * Border, size.y - 2 * Border - HeaderHeight);
	mCanvas->setSize(textRectSize);
}

void CodeEditorBase::onRender()
{
	Vec2i pos = getWorldPos();
	Vec2i size = getSize();

	RHIGraphics2D& g = Global::GetRHIGraphics2D();

	g.enablePen(false);
	g.setBrush(Color3ub(86, 86, 86));
	g.drawRoundRect(pos, size, Vector2(4, 4));

	if (bMinimized)
	{
		return;
	}

	mCanvas->mBlinkTimer++; 

	if (mShouldCheckHover && mCanvas->mBlinkTimer - mHoverTimer > 30)
	{
		mShouldCheckHover = false;
		
		Vector2 textRectPos(Border, Border + HeaderHeight);
		Vec2i canvasPos = mCanvas->getWorldPos();
		Vec2i localPos = mHoverMousePos + getWorldPos() - canvasPos;

		mHoverWord = mCanvas->getWordAtPosition(localPos);

		if (!mHoverWord.empty())
		{
			if (auto value = TFWR::IViewModel::Get().getVariableValue(*codeFile, mHoverWord.c_str()))
			{
				std::string displayText = mHoverWord + " = " + *value;
				Vector2 size = GEditorSettings.mFont->calcTextExtent(displayText.c_str(), 1.0f);
				mValueTooltip->setValue(displayText.c_str(), Vec2i(size.x, size.y));
				Vec2i pos = getWorldPos() + mHoverMousePos + Vec2i(10, -30);
				auto layer = getLayer();
				if (layer)
				{
					pos = layer->worldToScreen.transformPosition(pos);
				}
				mValueTooltip->setPos(pos);
				mValueTooltip->show(true);
				mValueTooltip->setTop(true);
			}
		}
	}
}


bool CodeEditorBase::onChildEvent(int event, int id, GWidget* ui)
{
	switch (id)
	{
	case UI_MINIMIZE_BUTTON:
		{
			bMinimized = !bMinimized;
			if (bMinimized)
			{
				mSizePrev = getSize();
				setSize(Vec2i(mSizePrev.x, HeaderHeight + Border));
				mCanvas->show(false);
			}
			else
			{
				setSize(mSizePrev);
				mCanvas->show(true);
			}
			//updateScrollBar();
			return true;
		}
	case UI_CLOSE_BUTTON:
		{
			show(false);
			return true;
		}
	default:
		break;
	}
	return true;
}

// Button implementations moved to CommonWidgets.cpp


void CodeEditCanvas::moveCursor(int dx, int dy)
{
	mCursorPos.y += dy;
	if (mCursorPos.y < 0) mCursorPos.y = 0;
	if (mCursorPos.y >= mLines.size()) mCursorPos.y = mLines.size() - 1;

	mCursorPos.x += dx;
	if (mCursorPos.x < 0)
	{
		if (mCursorPos.y > 0)
		{
			mCursorPos.y--;
			mCursorPos.x = mLines[mCursorPos.y].content.length();
		}
		else
		{
			mCursorPos.x = 0;
		}
	}

	if (mCursorPos.x > mLines[mCursorPos.y].content.length())
	{
		if (dx > 0 && mCursorPos.y < mLines.size() - 1)
		{
			mCursorPos.y++;
			mCursorPos.x = 0;
		}
		else
		{
			mCursorPos.x = mLines[mCursorPos.y].content.length();
		}
	}
	ensureCursorVisible();
}

void CodeEditCanvas::ensureCursorVisible()
{
	Vector2 textRectSize = getTextRectSize();
	int visibleLines = textRectSize.y / LineHeight;
	int scrollOffset = mScrollBar->getValue();

	if (mCursorPos.y < scrollOffset)
	{
		mScrollBar->setValue(mCursorPos.y);
	}
	else if (mCursorPos.y >= scrollOffset + visibleLines)
	{
		mScrollBar->setValue(mCursorPos.y - visibleLines + 1);
	}
}

void CodeEditCanvas::insertChar(char c)
{
	if (c == '\r') 
		c = '\n';
	if (c == '\n')
	{
		doEnter();
		return;
	}

	if (mCursorPos.y < mLines.size())
	{
		mLines[mCursorPos.y].content.insert(mCursorPos.x, 1, c);
		mCursorPos.x++;
		parseLine(mCursorPos.y);
		notifyModified();
	}
}

void CodeEditCanvas::doBackspace()
{
	if (mCursorPos.x > 0)
	{
		mLines[mCursorPos.y].content.erase(mCursorPos.x - 1, 1);
		mCursorPos.x--;
		parseLine(mCursorPos.y);
		notifyModified();
	}
	else if (mCursorPos.y > 0)
	{
		int prevLen = mLines[mCursorPos.y - 1].content.length();
		mLines[mCursorPos.y - 1].content += mLines[mCursorPos.y].content;
		mLines.removeIndex(mCursorPos.y);
		mCursorPos.y--;
		mCursorPos.x = prevLen;
		parseLine(mCursorPos.y);
		notifyModified();
		updateScrollBar();
	}
}

void CodeEditCanvas::doDelete()
{
	if (mCursorPos.x < mLines[mCursorPos.y].content.length())
	{
		mLines[mCursorPos.y].content.erase(mCursorPos.x, 1);
		parseLine(mCursorPos.y);
		notifyModified();
	}
	else if (mCursorPos.y < mLines.size() - 1)
	{
		mLines[mCursorPos.y].content += mLines[mCursorPos.y + 1].content;
		mLines.removeIndex(mCursorPos.y + 1);
		parseLine(mCursorPos.y);
		notifyModified();
		updateScrollBar();
	}
}

void CodeEditCanvas::doEnter()
{
	std::string currentLine = mLines[mCursorPos.y].content;
	std::string newLine = currentLine.substr(mCursorPos.x);
	mLines[mCursorPos.y].content = currentLine.substr(0, mCursorPos.x);
	mLines.insert(mLines.begin() + mCursorPos.y + 1, CodeLine());
	mLines[mCursorPos.y + 1].content = newLine;
	mCursorPos.y++;
	mCursorPos.x = 0;
	parseLine(mCursorPos.y - 1);
	parseLine(mCursorPos.y);
	notifyModified();
	updateScrollBar();
}

void CodeEditCanvas::parseLine(int index)
{
	auto& codeLine = mLines[index];
	std::string const& content = mLines[index].content;

	codeLine.vertices.clear();
	codeLine.colors.clear();

	if (content.empty())
	{
		codeLine.totalWidth = 0.0f;
		return;
	}

	codeLine.totalWidth = GEditorSettings.mFont->generateLineVertices(Vector2::Zero(), content, 1.0, codeLine.vertices);
	codeLine.colors.resize(codeLine.vertices.size() / 4);

	char const* pStr = content.data();
	char const* pStrEnd = content.data() + content.size();
	auto* pColor = codeLine.colors.data();
	auto* pColorEnd = codeLine.colors.data() + codeLine.colors.size();

	auto FillColor = [&](char const* start, char const* end, Color4ub color)
	{
		for (char const* p = start; p < end; ++p)
		{
			if (!FCString::IsSpace(*p))
			{
				if (pColor < pColorEnd)
				{
					*pColor++ = color;
				}
			}
		}
	};

	while (pStr < pStrEnd)
	{
		// Handle Comments
		if (pStr + 1 < pStrEnd && pStr[0] == '-' && pStr[1] == '-')
		{
			FillColor(pStr, pStrEnd, GEditorSettings.getColor("comment"));
			break;
		}

		// Handle Strings
		if (*pStr == '\"' || *pStr == '\'')
		{
			char quote = *pStr;
			char const* pStart = pStr;
			pStr++;
			while (pStr < pStrEnd)
			{
				if (*pStr == quote)
				{
					pStr++;
					break;
				}
				if (*pStr == '\\' && pStr + 1 < pStrEnd)
				{
					pStr += 2;
				}
				else
				{
					pStr++;
				}
			}
			FillColor(pStart, pStr, GEditorSettings.getColor("string"));
			continue;
		}

		// Handle Numbers
		if (FCString::IsDigit(*pStr))
		{
			char const* pStart = pStr;
			while (pStr < pStrEnd && (isalnum((unsigned char)*pStr) || *pStr == '.'))
			{
				pStr++;
			}
			FillColor(pStart, pStr, GEditorSettings.getColor("number"));
			continue;
		}

		// Handle Identifiers and Keywords
		if (FCString::IsAlpha(*pStr) || *pStr == '_')
		{
			char const* pStart = pStr;
			while (pStr < pStrEnd && (FCString::IsAlphaNumeric(*pStr) || *pStr == '_'))
			{
				pStr++;
			}
			StringView token(pStart, pStr - pStart);
			FillColor(pStart, pStr, GEditorSettings.getColor(token));
			continue;
		}

		// Handle Operators and Punctuation
		if (ispunct((unsigned char)*pStr))
		{
			FillColor(pStr, pStr + 1, GEditorSettings.getColor("operator"));
			pStr++;
			continue;
		}

		// Handle Whitespace
		if (FCString::IsSpace(*pStr))
		{
			// Skip whitespace, no color needed as per vertex generation assumption
			pStr++;
			continue;
		}

		// Fallback
		FillColor(pStr, pStr + 1, Color4ub(255, 255, 255));
		pStr++;
	}
}

void CodeEditCanvas::updateScrollBar()
{
	int totalLines = (int)mLines.size();
	Vector2 textRectSize = getTextRectSize();
	int visibleLines = textRectSize.y / LineHeight;

	if (totalLines <= visibleLines)
	{
		mScrollBar->show(false);
		mScrollBar->setValue(0);
	}
	else
	{
		mScrollBar->show(true);
		mScrollBar->setRange(std::max(0, totalLines - visibleLines), visibleLines);
	}
}

MsgReply CodeEditCanvas::onMouseMsg(MouseMsg const& msg)
{
	if (msg.onLeftDown())
	{
		Vec2i localPos = msg.getPos() - getWorldPos();
		Vector2 textRectSize = getTextRectSize();
		Vector2 textRectPos = getTextRectPos();
		if (localPos.x >= Border && localPos.x < Border + BreakpointMargin &&
			localPos.y >= textRectPos.y && localPos.y < textRectPos.y + textRectSize.y)
		{
			int relativeY = localPos.y - textRectPos.y;
			int lineIndex = mScrollBar->getValue() + relativeY / LineHeight;
			if (lineIndex >= 0 && lineIndex < mLines.size())
			{
				toggleBreakPoint(lineIndex);
			}
			return MsgReply::Handled();
		}

		if (localPos.x >= textRectPos.x && localPos.x < textRectPos.x + textRectSize.x &&
			localPos.y >= textRectPos.y && localPos.y < textRectPos.y + textRectSize.y)
		{
			int relativeY = localPos.y - textRectPos.y;
			int lineIndex = mScrollBar->getValue() + relativeY / LineHeight;

			if (lineIndex >= 0 && lineIndex < mLines.size())
			{
				mCursorPos.y = lineIndex;
				int relativeX = localPos.x - textRectPos.x;
				auto const& codeLine = mLines[lineIndex];
				mCursorPos.x = calculateCursorCol(codeLine.content, relativeX, codeLine.totalWidth);
			}
			return MsgReply::Handled();
		}
	}
	return BaseClass::onMouseMsg(msg);
}

MsgReply CodeEditCanvas::onKeyMsg(KeyMsg const& msg)
{
	if (msg.isDown())
	{
		switch (msg.getCode())
		{
		case EKeyCode::Left:  moveCursor(-1, 0); break;
		case EKeyCode::Right: moveCursor(1, 0); break;
		case EKeyCode::Up:    moveCursor(0, -1); break;
		case EKeyCode::Down:  moveCursor(0, 1); break;
		case EKeyCode::Back:  doBackspace(); break;
		case EKeyCode::Delete: doDelete(); break;
		case EKeyCode::Return: doEnter(); break;
		}
	}
	return BaseClass::onKeyMsg(msg);
}

MsgReply CodeEditCanvas::onCharMsg(unsigned code)
{
	if (code >= 32 && code != 127)
	{
		insertChar((char)code);
		return MsgReply::Handled();
	}
	return BaseClass::onCharMsg(code);
}

void CodeEditCanvas::onResize(Vec2i const& size)
{
	Vector2 textRectSize = getTextRectSize();

	// Update ScrollBar position and size
	Vec2i scrollBarPos(size.x - Border - ScrollbarWidth, Border);
	mScrollBar->setPos(scrollBarPos);
	mScrollBar->setSize(Vec2i(ScrollbarWidth, textRectSize.y));

	// Update ScrollBar range based on new size
	updateScrollBar();
}

void CodeEditCanvas::onRender()
{
	Vec2i pos = getWorldPos();
	Vec2i size = getSize();

	RHIGraphics2D& g = Global::GetRHIGraphics2D();

	g.enablePen(false);
	g.setBrush(Color3ub(30, 30, 30)); // Darker background
	g.drawRoundRect(pos, size, Vector2(4, 4));

	g.setBrush(Color3ub(30, 30, 30)); // Match background
	Vector2 textRectPos = Vector2(pos) + getTextRectPos();
	Vector2 textRectSize = getTextRectSize();
	g.drawRoundRect(textRectPos, textRectSize, Vector2(8, 8));

	Vector2 clipRectSize = textRectSize;
	if (mScrollBar->isShow())
	{
		clipRectSize.x -= ScrollbarWidth;
	}

	g.pushXForm();

	++showFrame;
	if (showFrame > 4)
	{
		if (!mLineShowQueue.empty())
		{
			mShowExecuteLine = mLineShowQueue.front();
			mLineShowQueue.removeIndex(0);
			showFrame = 0;
		}
	}
	g.setBrush(Color3ub(255, 255, 255));
	g.drawTextF(pos, "%d", mShowExecuteLine);
	g.translateXForm(textRectPos.x, textRectPos.y);

	g.setTexture(GEditorSettings.mFont->getTexture());
	g.setTextRemoveRotation(false);
	g.setTextRemoveScale(false);
	g.setBlendState(ESimpleBlendMode::Translucent);

	int curline = 0;
	int visibleLines = textRectSize.y / LineHeight;

	int scrollOffset = mScrollBar->getValue();

	// Apply scroll offset
	// We can skip lines that are scrolled out
	int startLine = scrollOffset;
	int endLine = startLine + visibleLines + 1;


	for (int i = scrollOffset; i < mLines.size(); ++i)
	{
		// Check if out of view
		if ((i - scrollOffset) * LineHeight > textRectSize.y)
			break;

		auto const& line = mLines[i];

		if (hasBreakPoint(i))
		{
			g.setBrush(Color3ub(200, 50, 50));
			g.setBlendState(ESimpleBlendMode::None);
			g.drawCircle(Vector2(-BreakpointMargin / 2 - 2, (i - scrollOffset) * LineHeight + LineHeight / 2), 4);
			g.setTexture(GEditorSettings.mFont->getTexture());
			g.setBlendState(ESimpleBlendMode::Translucent);
		}
	}

	auto layer = getLayer();
	if (layer)
	{
		Vector2 min = layer->worldToScreen.transformPosition(textRectPos);
		Vector2 max = layer->worldToScreen.transformPosition(textRectPos + clipRectSize);
		g.beginClip(min, max - min);
	}
	else
	{
		g.beginClip(textRectPos, clipRectSize);
	}

	for (int i = scrollOffset; i < mLines.size(); ++i)
	{
		// Check if out of view
		if ((i - scrollOffset) * LineHeight > textRectSize.y)
			break;

		auto const& line = mLines[i];

		if ((i + 1) == mShowExecuteLine)
		{
			// Draw execution highlight
			g.setBrush(Color3ub(32, 32, 32));
			g.setBlendState(ESimpleBlendMode::Add);
			g.drawRect(Vector2(-5, (i - scrollOffset) * LineHeight), Vector2(textRectSize.x, LineHeight));

			g.setBrush(Color3ub(255, 255, 255));
			g.setTexture(GEditorSettings.mFont->getTexture());
			g.setBlendState(ESimpleBlendMode::Translucent);
		}

		// We need to draw at correct offset.
		// Since drawTextQuad uses vertices relative to (0,0), we need to translate.
		// But we can't translate cumulatively if we skip lines.

		g.pushXForm();
		g.translateXForm(0, (i - scrollOffset) * LineHeight);
		g.drawTextQuad(line.vertices, line.colors);

		// Draw Cursor
		if (i == mCursorPos.y)
		{
			// Calculate accurate cursor position using actual text width
			int cursorX = 0;
			if (mCursorPos.x > 0 && i < mLines.size())
			{
				StringView textBeforeCursor(mLines[i].content.data(), mCursorPos.x);
				Vector2 extent = GEditorSettings.mFont->calcTextExtent(textBeforeCursor, 1.0f);
				cursorX = (int)extent.x;
			}

			if ((mBlinkTimer / 30) % 2 == 0) // Blink speed
			{
				g.setBrush(Color3ub(255, 255, 255));
				g.setBlendState(ESimpleBlendMode::Translucent); // Ensure visible
				g.drawRect(Vector2(cursorX, 0), Vector2(2, 16));
				g.setTexture(GEditorSettings.mFont->getTexture());
			}
		}
		g.popXForm();
	}

	g.popXForm();
	g.endClip();

	g.enablePen(true);
}

CodeEditCanvas::CodeEditCanvas(int id, Vec2i const& pos, Vec2i const& size, GWidget* parent)
	:BaseClass(pos, size, parent)
{
	mID = id;
	// Create ScrollBar
	Vec2i textRectSize = getTextRectSize();
	Vec2i scrollBarPos(size.x - Border - ScrollbarWidth, Border);
	int scrollBarHeight = textRectSize.y;
	mScrollBar = new GScrollBar(0, scrollBarPos, scrollBarHeight, false, this);
	mScrollBar->setRange(0, 10); // Initial range
}

void CodeEditCanvas::loadFromCode(std::string const& code)
{
	auto AddLine = [&](std::string&& content)
	{
		auto line = mLines.addDefault();
		line->content = std::move(content);
	};

	mLines.clear();
	std::stringstream ss(code);
	std::string line;
	while (std::getline(ss, line))
	{
		if (!line.empty() && line.back() == '\r')
			line.pop_back();

		AddLine(std::move(line));
	}
	if (mLines.empty())
		AddLine("");

	parseCode();
}

std::string CodeEditCanvas::syncCode()
{
	std::string newCode;
	for (size_t i = 0; i < mLines.size(); ++i)
	{
		newCode += mLines[i].content;
		if (i < mLines.size() - 1) newCode += "\n";
	}
	return newCode;
}

void CodeEditCanvas::parseCode()
{
	for (int i = 0; i < mLines.size(); ++i)
	{
		parseLine(i);
	}
	updateScrollBar();
}

std::string CodeEditCanvas::getWordAtPosition(Vec2i const& pos)
{
	Vector2 textRectPos = getTextRectPos();
	int relativeY = pos.y - textRectPos.y;
	int lineIndex = mScrollBar->getValue() + relativeY / LineHeight;

	if (lineIndex >= 0 && lineIndex < mLines.size())
	{
		// Calculate accurate column position
		int relativeX = pos.x - textRectPos.x;
		auto const& codeLine = mLines[lineIndex];
		std::string const& line = codeLine.content;
		int bestCol = calculateCursorCol(line, relativeX, codeLine.totalWidth);

		if (bestCol > 0 && bestCol <= line.length())
		{
			if (!FCString::IsAlphaNumeric(line[bestCol - 1]))
				return "";

			int start = bestCol - 1;
			while (start > 0 && FCString::IsAlphaNumeric(line[start - 1]))
			{
				start--;
			}
			int end = bestCol - 1;
			while (end < line.length() - 1 && FCString::IsAlphaNumeric(line[end + 1]))
			{
				end++;
			}

			return line.substr(start, end - start + 1);
		}
	}
	return "";
}

int CodeEditCanvas::calculateCursorCol(std::string const& line, int relativeX, float totalWidth)
{
	if (line.empty())
		return 0;

	if (relativeX <= 0)
		return 0;

	if (relativeX >= totalWidth)
		return line.length();

	int low = 0;
	int high = line.length();
	float wLow = 0;
	float wHigh = totalWidth;

	Render::CharDataSet* charSet = GEditorSettings.mFont->mCharDataSet;

	while (high - low > 1)
	{
		// Interpolation search
		float t = (relativeX - wLow) / (wHigh - wLow);
		int mid = low + (int)((high - low) * t);

		// Ensure we make progress and stay within bounds
		if (mid <= low) mid = low + 1;
		if (mid >= high) mid = high - 1;

		// Optimization: Calculate extent from 'low' instead of beginning
		StringView sub(line.data() + low, mid - low);
		float wSub = GEditorSettings.mFont->calcTextExtent(sub).x;
		float wMid = wLow + wSub;

		// Apply kerning correction
		if (low > 0)
		{
			float kerning = 0;
			if (charSet->getKerningPair((unsigned char)line[low - 1], (unsigned char)line[low], kerning))
			{
				wMid += kerning;
			}
		}

		if (wMid < relativeX)
		{
			low = mid;
			wLow = wMid;
		}
		else
		{
			high = mid;
			wHigh = wMid;
		}
	}

	if (std::abs(wLow - relativeX) <= std::abs(wHigh - relativeX))
	{
		return low;
	}
	else
	{
		return high;
	}
}
