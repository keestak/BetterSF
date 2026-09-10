
#pragma once

#include "IControl.h"

using namespace iplug;
using namespace igraphics;

const IColor bgcol_light = IColor(255, 120, 120, 120);
const IColor bgcol_mid = IColor(255, 90, 90, 90);
const IColor bgcol_middark = IColor(255, 80, 80, 80);
const IColor bgcol_dark = IColor(255, 40, 40, 40);
const IColor bgcol_verydark = IColor(255, 30, 30, 30);
const IColor ui_mid = IColor(255, 88, 104, 119);
const IColor ui_light = IColor(255, 150, 169, 183);
const IColor ui_verylight = IColor(255, 219, 236, 249);
const IColor ui_control_accent = IColor(255, 8, 176, 249);

class KsKnobControl : public IKnobControlBase
{
public:
	KsKnobControl(const IRECT& bounds)
		: IKnobControlBase(bounds)
	{
	}

	KsKnobControl(const IRECT& bounds, int paramIdx)
		: IKnobControlBase(bounds, paramIdx)
	{
	}

private:

	void OnMouseDblClick(float x, float y, const IMouseMod& mod) override
	{
		if (mDisabled)
			return;

		const IParam* param = GetParam();
		if (param == nullptr)
			SetValue(0);
		else
			SetValue(param->GetDefault() / param->GetMax());
		SetDirty(true);
	}

	void Draw(IGraphics& g) override
	{
		const float cx = mRECT.MW();
		const float cy = mRECT.MH();
		const float r = mRECT.W() * 0.5f;
		const IPattern knob_fill_gradient = IPattern::CreateLinearGradient(cx, mRECT.T, cx, mRECT.B, { IColorStop(bgcol_mid, 0.f), IColorStop(bgcol_dark, 1.f) });

		// draw background panel
		//g.FillRoundRect(bgcol_light, mRECT);
		//g.DrawRoundRect(bgcol_dark, mRECT);
		
		// draw fill bar background
		g.DrawArc(bgcol_verydark, cx, cy, r - 5, -130, 130, nullptr, 4.0f);

		// draw knob drop shadow
		g.DrawFastDropShadow(mRECT.GetPadded(-10), mRECT, 3.0f, mRECT.W() * 0.5, 20.0f);
		
		// draw knob base circle
		g.PathCircle(cx, cy, r - 8);
		g.PathClose();
		g.PathFill(knob_fill_gradient);
		g.PathClear();
		// draw knob rim circle
		g.DrawCircle(bgcol_mid, cx, cy, r - 9);
		
		float v = GetValue();

		// draw position indicator
		g.DrawRadialLine(bgcol_dark, cx, cy + 2, -130 + (130 * 2 * v), (r - 8) * 0.5, r - 8, nullptr, 2.0f);
		g.DrawRadialLine(ui_verylight, cx, cy, -130 + (130 * 2 * v), (r - 8) * 0.5, r - 8, nullptr, 2.0f);

		// draw fill bar
		g.DrawArc(ui_control_accent, cx, cy, r - 5, -130, (130 * 2 * v) - 130, nullptr, 3.0f);

	}
};

class KsEditableTextControl : public IControl
{
public:
	KsEditableTextControl(const IRECT& bounds, IActionFunction aF)
		: IControl(bounds, aF)
	{
		mText.mSize = 20.0f;
		mText.mAlign = EAlign::Near;
		mText.mFGColor = ui_control_accent;
	}

	const char* GetStr()
	{
		return mStr.c_str();
	}

private:
	std::string mStr = "";

	void OnMouseDown(float x, float y, const IMouseMod& mod) override
	{
		GetUI()->CreateTextEntry(*this, mText, mRECT, mStr.c_str());
	}

	void OnTextEntryCompletion(const char* str, int valIdx) override
	{
		mStr = str;
		SetDirty(true);
	}

	void Draw(IGraphics& g) override
	{
		//draw bg
		g.FillRoundRect(bgcol_dark, mRECT);
		g.DrawRoundRect(ui_control_accent, mRECT);
		g.DrawText(mText, mStr.c_str(), mRECT);
	}
};

class KsListViewControl : public IControl
{
public:
	KsListViewControl(const IRECT& bounds, IActionFunction aF)
		: IControl(bounds, aF)
	{
	}

	KsListViewControl(const IRECT& bounds, int paramIdx, IActionFunction aF)
		: IControl(bounds, paramIdx, aF)
	{
	}

	KsListViewControl(const IRECT& bounds, int paramIdx)
		: IControl(bounds, paramIdx)
	{
	}

	std::function<void(const char*)> mOnDropCallback = nullptr;

	void AddListItem(std::string item)
	{
		mListItems.push_back(item);
		SetDirty(false);
	}

	void RemoveItemAtIndex(int index)
	{
		mListItems.erase(mListItems.begin() + index);
		mScrollOffsetY = std::clamp(mScrollOffsetY, 0.0f, (mFilteredListItems.size() * mListItemHeight - mRECT.H()));
		SetDirty(false);
	}

	void ClearListItems()
	{
		mScrollOffsetY = 0.0f;
		mTrueSelectedIndex = -1;
		mVisibleSelectedIndex = -1;
		mListItems.clear();
		mFilteredListIndices.clear();
		mFilteredListItems.clear();
		SetDirty(false);
	}

	int LastClickedIndex()
	{
		return mTrueSelectedIndex;
	}

	void SelectIndex(int index, bool triggerAction = true, bool setDirty = true)
	{
		UpdateFilteredListItems();

		if (index == -1)
		{
			mTrueSelectedIndex = -1;
			mVisibleSelectedIndex = -1;
		}
		else
		{
			mTrueSelectedIndex = std::clamp(index, 0, std::max(0, (int)mListItems.size() - 1));

			mVisibleSelectedIndex = -1;
			for (int i = 0; i < mFilteredListIndices.size(); i++)
			{
				if (mFilteredListIndices[i] == index)
				{
					mVisibleSelectedIndex = i;
					break;
				}
			}
		}

		FocusOnSelected();

		if (setDirty)
			SetDirty(triggerAction);
		//OutputDebugString(std::format("SELECT INDEX: {}\n", index).c_str());
	}

	void FocusOnSelected()
	{
		mShouldFocusSelectedItem = true;
		SetDirty(false);
	}

	void SelectWithPrefix(std::string prefix, bool triggerAction = true)
	{
		//OutputDebugString(std::format("SELECT WITH PREFIX: {}\n", prefix).c_str());
		for (int i = 0; i < mListItems.size(); i++)
		{
			if (mListItems[i].compare(0, prefix.size(), prefix) == 0)
			{
				SelectIndex(i, triggerAction);
				return;
			}
		}
	}

	void SetSearchFilter(std::string filter)
	{
		mSearchFilter = filter;
		SetDirty(false);
	}

private:
	std::vector<std::string> mListItems;
	std::vector<std::string> mFilteredListItems;
	std::vector<int> mFilteredListIndices;
	std::string mSearchFilter = "";
	float mScrollOffsetY = 0.0f;
	float mTotalHeight = 0.0f;
	float mListItemHeight = 25.0f;
	float mScrollBarH = 100.0f;
	float mScrollBarW = 16.0f;
	int mTrueSelectedIndex = -1;	//selected index of mListItems
	int mVisibleSelectedIndex = -1;	//sekected index of mFilteredListItems
	bool mScrollBarVisible = false;
	bool mShouldFocusSelectedItem = false;
	float mScrollBarOffset = 0.0f;
	bool mDraggingScrollBar = false;
	float mScrollBarDragYOffset = 0.0f;

	int UpdateFilteredListItems()
	{
		std::string filterLower = mSearchFilter;
		std::transform(filterLower.begin(), filterLower.end(), filterLower.begin(), ::tolower);

		mFilteredListItems.clear();
		mFilteredListIndices.clear();
		for (int i = 0; i < mListItems.size(); i++)
		{
			if (mSearchFilter.empty())
			{
				mFilteredListItems.push_back(mListItems[i]);
				mFilteredListIndices.push_back(i);
				continue;
			}

			std::string sLower = mListItems[i];
			std::transform(sLower.begin(), sLower.end(), sLower.begin(), ::tolower);

			if (sLower.find(filterLower) != std::string::npos)
			{
				mFilteredListItems.push_back(mListItems[i]);
				mFilteredListIndices.push_back(i);
			}
		}
		return mFilteredListItems.size();
	}

	void Draw(IGraphics& g) override
	{
		//OutputDebugString(std::format("wtf {}\n", mSelectedIndex).c_str());

		UpdateFilteredListItems();

		g.FillRect(bgcol_mid, mRECT);

		IText t = IText();
		t.mSize = 20.0f;
		t.mAlign = EAlign::Near;
		t.mFGColor = ui_control_accent;

		IText st = IText();
		st.mSize = 20.0f;
		st.mAlign = EAlign::Near;
		st.mFGColor = COLOR_WHITE;

		mScrollBarVisible = mFilteredListItems.size() * mListItemHeight > mRECT.H();
		if (mScrollBarVisible)
		{
			float selectedIndexYOffset = mListItemHeight * mVisibleSelectedIndex - mScrollOffsetY;

			if (mShouldFocusSelectedItem && (selectedIndexYOffset < 0 || selectedIndexYOffset > mRECT.H()))
			{
				mShouldFocusSelectedItem = false;
				mScrollOffsetY = std::clamp(mVisibleSelectedIndex * mListItemHeight, 0.0f, (mFilteredListItems.size() * mListItemHeight - mRECT.H()));
			}
			mScrollBarH = std::max(mRECT.H() / (mFilteredListItems.size() * mListItemHeight) * (mRECT.H() - mScrollBarW * 2), 20.0f);
		}
		else
		{
			mScrollOffsetY = 0;
		}

		for (int i = 0; i < mFilteredListItems.size(); i++)
		{
			IRECT r = IRECT(mRECT.L, mRECT.T + i * mListItemHeight - mScrollOffsetY, mRECT.R, mRECT.T + i * mListItemHeight - mScrollOffsetY + mListItemHeight);

			if (!mRECT.Intersects(r))
				continue;

			if (i % 2 == 0)
			{
				g.FillRect(bgcol_middark, r);
			}
			if  (mFilteredListIndices[i] == mTrueSelectedIndex) //(i == mSelectedIndex)
			{
				g.FillRect(ui_control_accent, r);
			}
			g.DrawText((mFilteredListIndices[i] == mTrueSelectedIndex) ? st : t, mFilteredListItems[i].c_str(), r.GetHShifted(10));
		}

		//weird hacky inner drop shadow
		g.DrawFastDropShadow(mRECT.GetFromTLHC(mRECT.W() + 20, 20).GetTranslated(-10, -20), mRECT.GetFromTLHC(mRECT.W() + 20, 50).GetTranslated(-10, -20));
		g.DrawFastDropShadow(mRECT.GetFromTLHC(20, mRECT.H() + 20).GetTranslated(-20, -10), mRECT.GetFromTLHC(50, mRECT.H() + 20).GetTranslated(-20, -10));

		//draw scroll bar
		if (mScrollBarVisible)
		{
			g.FillRect(bgcol_dark, mRECT.GetFromRight(mScrollBarW));
			mScrollBarOffset = (mScrollOffsetY / (mFilteredListItems.size() * mListItemHeight - mRECT.H() - mScrollBarW * 2)) * (mRECT.H() - mScrollBarW * 2 - mScrollBarH) + mScrollBarW;
			g.FillRoundRect(bgcol_mid, mRECT.GetFromTRHC(mScrollBarW - 4, mScrollBarH).GetTranslated(-2, mScrollBarOffset), 3.0f);

			IRECT scrollButtonRect = mRECT.GetFromTRHC(mScrollBarW, mScrollBarW);
			g.FillRect(bgcol_verydark, scrollButtonRect);
			g.DrawArc(bgcol_mid, scrollButtonRect.MW(), scrollButtonRect.GetVShifted(3).MH(), mScrollBarW / 2.0 - 3, -90, 90, nullptr, 3.0);

			scrollButtonRect = mRECT.GetFromBRHC(mScrollBarW, mScrollBarW);
			g.FillRect(bgcol_verydark, scrollButtonRect);
			g.DrawArc(bgcol_mid, scrollButtonRect.MW(), scrollButtonRect.GetVShifted(-3).MH(), mScrollBarW / 2.0 - 3, -270, -90, nullptr, 3.0);
		}

		//draw border
		g.DrawRect(bgcol_verydark, mRECT);
	}

	void OnMouseDown(float x, float y, const IMouseMod& mod) override
	{
		mDraggingScrollBar = false;

		if (mScrollBarVisible && x > mRECT.R - mScrollBarW) //clicked on scroll bar
		{
			float relativeMouseY = y - mRECT.T;

			if (relativeMouseY < mScrollBarW)
			{
				mScrollOffsetY = std::clamp(mScrollOffsetY - mListItemHeight, 0.0f, (mFilteredListItems.size() * mListItemHeight - mRECT.H()));
				SetDirty(false);
			}
			else if (relativeMouseY > mRECT.H() - mScrollBarW)
			{
				mScrollOffsetY = std::clamp(mScrollOffsetY + mListItemHeight, 0.0f, (mFilteredListItems.size() * mListItemHeight - mRECT.H()));
				SetDirty(false);
			}
			else if (relativeMouseY > mScrollBarOffset && relativeMouseY < mScrollBarOffset + mScrollBarH)
			{
				mDraggingScrollBar = true;
				mScrollBarDragYOffset = y - mScrollBarOffset - mRECT.T;
			}
			return;
		}

		//calculate which list item is being clicked
		int clickedIndex = (int)std::floor((y + mScrollOffsetY - mRECT.T) / mListItemHeight);
		if (clickedIndex >= mFilteredListItems.size() || clickedIndex < 0)
		{
			SelectIndex(-1, false, false);
			SetDirty(false);
			return;
		}
		SelectIndex(mFilteredListIndices[clickedIndex]);
		//mSelectedIndex = clickedIndex;
		//OutputDebugString(std::format("SELECT BY CLICK: {}\n", clickedIndex).c_str());
	}

	void OnMouseWheel(float x, float y, const IMouseMod& mod, float d) override
	{
		if (!mScrollBarVisible) return;
		mScrollOffsetY = std::clamp(mScrollOffsetY - d * mListItemHeight, 0.0f, (mFilteredListItems.size() * mListItemHeight - mRECT.H()));
		SetDirty(false);
	}

	void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override
	{
		if (mDraggingScrollBar && mScrollBarVisible)
		{
			mScrollOffsetY = std::clamp((mFilteredListItems.size() * mListItemHeight - mRECT.H()) * (y - mScrollBarDragYOffset - mRECT.T) / (mRECT.H() - mScrollBarH), 0.0f, (mFilteredListItems.size() * mListItemHeight - mRECT.H()));
			SetDirty(false);
		}
	}

	void OnDrop(const char* str) override
	{
		if (mOnDropCallback)
			mOnDropCallback(str);
	}

};

class RoundRectBgPanel : public IControl
{
public:
	RoundRectBgPanel(const IRECT& bounds, IColor color = bgcol_mid, float radius = 5.0f, bool drawBorder = false, IColor borderColor = bgcol_dark)
		: IControl(bounds),
		mColor(color),
		mRadius(radius),
		mDrawBorder(drawBorder),
		mBorderColor(borderColor)
	{
	}

	void Draw(IGraphics& g) override
	{
		g.FillRoundRect(mColor, mRECT, mRadius);
		if (mDrawBorder)
			g.DrawRoundRect(mBorderColor, mRECT, mRadius);
	}

private:
	IColor mColor;
	IColor mBorderColor;
	float mRadius;
	bool mDrawBorder;
};

class FancyDotsDecoration : public IControl
{
public:
	FancyDotsDecoration(const IRECT& bounds, IColor color = bgcol_mid, float dotsRadius = 2.0f, float dotsSpacing = 10.0f, bool redrawOnResize = false)
		: IControl(bounds),
		mColor(color),
		mDotsSize(dotsRadius),
		mRedrawOnResize(redrawOnResize),
		mDotsSpacing(dotsSpacing)
	{
	}

	void Draw(IGraphics& g) override
	{
		if (!mLayer)
		{
			g.StartLayer(this, mRECT);

			for (float x = mDotsSpacing; x < mRECT.W(); x += mDotsSpacing)
			{
				for (float y = mDotsSpacing; y < mRECT.H(); y += mDotsSpacing)
				{
					g.FillCircle(mColor, x, y, mDotsSize);
				}
			}

			mLayer = g.EndLayer();
		}

		g.DrawLayer(mLayer);
	}

	void OnResize() override
	{
		if (mRedrawOnResize)
			mLayer = nullptr;
	}

private:
	IColor mColor;
	float mDotsSize;
	float mDotsSpacing;
	bool mRedrawOnResize;
	ILayerPtr mLayer = nullptr;
};

class FileLoaderDisplay : public IControl
{
public:
	std::function<void(const char*)> mLoadFileCallback = nullptr;

	FileLoaderDisplay(const IRECT& bounds, IActionFunction aF)
		: IControl(bounds, aF)
	{
	}

	FileLoaderDisplay(const IRECT& bounds, int paramIdx, IActionFunction aF)
		: IControl(bounds, paramIdx, aF)
	{
	}

	void AddFilePath(std::string file, bool setAsCurrent = true, bool triggerAction = true)
	{
		if (file.empty()) return;
		if (std::find(mPreviousLoadedFiles.begin(), mPreviousLoadedFiles.end(), file) == mPreviousLoadedFiles.end())
		{
			mPreviousLoadedFiles.push_back(file);
		}

		if (setAsCurrent)
		{
			mCurrentFilePath = file;
		}

		SetDirty(triggerAction);
	}

	void ClearFilePath()
	{
		mCurrentFilePath.clear();
		SetDirty(false);
	}

	const char* CurrentFilePath()
	{
		return mCurrentFilePath.c_str();
	}

private:
	std::string mCurrentFilePath = "";
	std::vector<std::string> mPreviousLoadedFiles;
	bool mIsLoadRectMouseOver = false;
	bool mIsTextRectMouseOver = false;


	void Draw(IGraphics& g) override
	{
		g.FillRoundRect(bgcol_dark, mRECT);
		g.DrawRoundRect(ui_control_accent, mRECT);

		IText t = IText();
		t.mSize = 20.0f;
		t.mAlign = EAlign::Near;
		t.mFGColor = ui_control_accent;

		IRECT pathTextBounds = mRECT.GetPadded(-10, -5, 0, -5).GetFromLeft(mRECT.W() - 100);
		IRECT b;

		std::string finalPathToDraw = mCurrentFilePath;

		if (finalPathToDraw == "")
			finalPathToDraw = "No Soundfont Loaded.";

		g.MeasureText(t, finalPathToDraw.c_str(), b);
		if (b.W() > pathTextBounds.W())
		{
			while (b.W() > pathTextBounds.W())
			{
				finalPathToDraw.pop_back();
				g.MeasureText(t, finalPathToDraw.c_str(), b);
			}
			finalPathToDraw += "...";
		}

		g.DrawText(t, finalPathToDraw.c_str(), pathTextBounds);

		IRECT loadRect = mRECT.GetFromRight(70);
		if (mIsLoadRectMouseOver)
			g.FillRoundRect(ui_control_accent.WithOpacity(0.25f), loadRect);
		if (mIsTextRectMouseOver)
			g.FillRoundRect(ui_control_accent.WithOpacity(0.25f), mRECT.GetFromLeft(mRECT.W()  - 70));
		g.DrawText(t, "Load...", loadRect.GetHShifted(10));
		g.DrawLine(ui_control_accent, mRECT.R - 70, mRECT.B - 5, mRECT.R - 70, mRECT.T + 5);
	}

	void OnMouseOver(float x, float y, const IMouseMod& mod) override
	{
		mIsLoadRectMouseOver = false;
		mIsTextRectMouseOver = false;

		IRECT loadRect = mRECT.GetFromRight(70);
		if (loadRect.Contains(x, y))
			mIsLoadRectMouseOver = true;
		else if (mRECT.Contains(x, y))
			mIsTextRectMouseOver = true;

		SetDirty(false);
	}

	void OnMouseOut() override
	{
		mIsLoadRectMouseOver = false;
		mIsTextRectMouseOver = false;
		SetDirty(false);
	}


	void OnMouseDown(float x, float y, const IMouseMod& mod) override
	{
		if (mIsLoadRectMouseOver)
		{
			SetDirty(true);
		}
		else if (mIsTextRectMouseOver)
		{
			if (mPreviousLoadedFiles.empty())
			{
				SetDirty(true);
			}
			else
			{
				IPopupMenu menu = IPopupMenu("Previous Files", {});
				for (int i = 0; i < mPreviousLoadedFiles.size(); i++)
				{
					menu.AddItem(new IPopupMenu::Item(mPreviousLoadedFiles[i].c_str()));
				}
				//menu.AddSeparator();
				//menu.AddItem(new IPopupMenu::Item("Set current as default"));
				GetUI()->CreatePopupMenu(*this, menu, mRECT);
			}
		}
	}

	void OnPopupMenuSelection(IPopupMenu* pSelectedMenu, int valIdx) override
	{
		if (pSelectedMenu)
		{
			int i = pSelectedMenu->GetChosenItemIdx();
	/*		if (i == mPreviousLoadedFiles.size() + 1)
			{
				// TODO
				DBGMSG("Set Default Path");
			}
			else */if (mLoadFileCallback)
			{
				mLoadFileCallback(mPreviousLoadedFiles[i].c_str());
			}
		}
	}

	void OnDrop(const char* str) override
	{
		if (mLoadFileCallback)
			mLoadFileCallback(str);
	}
};

class KsChannelSelector : public IControl
{
public:
	KsChannelSelector(const IRECT& bounds, int paramIdx, IActionFunction aF)
		: IControl(bounds, paramIdx, aF)
	{
	}

	KsChannelSelector(const IRECT& bounds, IActionFunction aF)
		: IControl(bounds, aF)
	{
	}

	int SelectedChannel()
	{
		return mSelectedChannel;
	}

	void SelectChannel(int channel, bool triggerAction)
	{
		mSelectedChannel = channel;
		SetDirty(triggerAction);
	}

private:
	int mSelectedChannel = 0;

	void Draw(IGraphics& g) override
	{
		g.FillRoundRect(bgcol_dark, mRECT);
		g.DrawRoundRect(bgcol_verydark, mRECT);

		IText t = IText();
		t.mSize = 15.0f;

		IRECT buttonRect = mRECT.GetFromTLHC(mRECT.H() - 4, mRECT.H() - 4).GetTranslated(2, 2);

		for (int c = 0; c < 16; c += 1)
		{
			float offset = c * (mRECT.W() / 16);

			if (c == mSelectedChannel)
			{
				t.mFGColor = COLOR_WHITE;
				g.FillRoundRect(ui_control_accent, buttonRect.GetHShifted(offset));
			}
			else
			{
				t.mFGColor = ui_control_accent;
				g.DrawRoundRect(bgcol_verydark, buttonRect.GetHShifted(offset));
			}
			g.DrawText(t, std::to_string(c + 1).c_str(), buttonRect.GetHShifted(offset));
		}
	}

	void OnMouseDown(float x, float y, const IMouseMod& mod) override
	{
		mSelectedChannel = (x - mRECT.L) / mRECT.W() * 16;
		SetDirty(true);
	}
};

class KsControlActiveToggle : public IControl
{
public:
	KsControlActiveToggle(const IRECT& bounds, const char* paramName)
		: IControl(bounds),
		mParamName(paramName)
	{
	}

	KsControlActiveToggle(const IRECT& bounds, const char* paramName, bool showToggle, int paramIdx, IActionFunction aF)
		: IControl(bounds, paramIdx, aF),
		mParamName(paramName),
		mShowToggle(showToggle)
	{
	}

	KsControlActiveToggle(const IRECT& bounds, const char* paramName, bool showToggle, int paramIdx)
		: IControl(bounds, paramIdx),
		mParamName(paramName),
		mShowToggle(showToggle)
	{
	}

	KsControlActiveToggle(const IRECT& bounds, const char* paramName, bool showToggle, IActionFunction aF)
		: IControl(bounds, aF),
		mParamName(paramName),
		mShowToggle(showToggle)
	{
	}

private:
	bool mShowToggle = false;
	const char* mParamName = "";

	void Draw(IGraphics& g) override
	{
		IText t = IText();
		t.mSize = 20.0f;
		t.mFGColor = COLOR_WHITE;

		IRECT toggleRect = mRECT.GetFromLeft(mShowToggle ? mRECT.H() : 1.0f);

		IRECT b;
		mRECT.GetFromLeft(g.MeasureText(t, mParamName, b));
		IRECT nameRect = mRECT.GetFromLeft(b.W()).GetHShifted(toggleRect.W() + 10);

		if (mShowToggle)
		{
			g.FillCircle(bgcol_verydark, toggleRect.MW(), toggleRect.MH(), mRECT.H() / 2);
			g.DrawCircle(ui_control_accent, toggleRect.MW(), toggleRect.MH(), mRECT.H() / 2);
			if (GetValue() > 0.5)
			{
				g.FillCircle(ui_control_accent, toggleRect.MW(), toggleRect.MH(), mRECT.H() / 2 - 3);
			}
		}

		g.DrawText(t, mParamName, nameRect);
		g.DrawLine(bgcol_verydark, nameRect.R + 10, nameRect.MH(), mRECT.R, nameRect.MH(), 0, 3.0f);
	}

	void OnMouseDown(float x, float y, const IMouseMod& mod) override
	{
		if (!mShowToggle) return;

		IRECT toggleRect = mRECT.GetFromLeft(mShowToggle ? mRECT.H() : 1.0f);

		if (toggleRect.Contains(x, y))
		{
			SetValue(GetValue() > 0.5 ? 0 : 1);
			SetDirty(true);
		}
	}
};

class KsTextButton : public IControl
{
public:
	KsTextButton(const IRECT& bounds, std::string text, IActionFunction aF)
		: IControl(bounds, aF)
	{
		SetText(text);
	}

	void SetText(std::string text)
	{
		mLabel = text;
		SetDirty(false);
	}

private:
	std::string mLabel = "button";
	int mMouseState = 0;

	void Draw(IGraphics& g) override
	{
		g.FillRoundRect(bgcol_dark, mRECT);
		g.DrawRoundRect(ui_control_accent, mRECT);
		g.FillRoundRect(ui_control_accent.WithOpacity(0.25f * (float)mMouseState), mRECT);

		IText t = IText();
		t.mSize = 20.0f;
		t.mAlign = EAlign::Center;
		t.mFGColor = ui_control_accent;

		if (!mLabel.empty())
		{
			g.DrawText(t, mLabel.c_str(), mRECT);
		}
	}

	void OnMouseOver(float x, float y, const IMouseMod& mod) override
	{
		mMouseState = 0;

		if (mRECT.Contains(x, y))
			mMouseState = 1;

		SetDirty(false);
	}

	void OnMouseOut() override
	{
		mMouseState = 0;
		SetDirty(false);
	}

	void OnMouseDown(float x, float y, const IMouseMod& mod) override
	{
		mMouseState = 2;
		SetDirty(true);
	}
	
	void OnMouseUp(float x, float y, const IMouseMod& mod) override
	{
		mMouseState = 1;
		SetDirty(false);
	}
};

class KsDropdownList : public IControl
{
public:
	std::function<void(const char*)> mLoadFileCallback = nullptr;

	KsDropdownList(const IRECT& bounds, int paramIdx)
		: IControl(bounds, paramIdx)
	{
	}

	KsDropdownList(const IRECT& bounds, int paramIdx, std::string label)
		: IControl(bounds, paramIdx),
		mLabel(label)
	{
	}

	void AddItem(std::string itemName)
	{
		if (itemName.empty()) return;
		if (std::find(mItemStrings.begin(), mItemStrings.end(), itemName) == mItemStrings.end())
		{
			mItemStrings.push_back(itemName);
		}

		SetDirty(false);
	}

	void SetSelectedIndex(int idx, bool triggerAction = false)
	{
		mSelectedItem = idx;
		SetValue((double)mSelectedItem / (mItemStrings.size() - 1));
		SetDirty(triggerAction);
	}

	int SelectedIndex()
	{
		return mSelectedItem;
	}

private:
	int mSelectedItem = 0;
	std::vector<std::string> mItemStrings;
	std::string mLabel = "";
	bool mIsRectMouseOver = false;


	void Draw(IGraphics& g) override
	{
		//OutputDebugString(std::format("VAL: {}\n", GetValue()).c_str());
		mSelectedItem = GetValue() * (mItemStrings.size() - 1);

		IText t = IText();
		t.mSize = 20.0f;
		t.mAlign = EAlign::Near;
		t.mFGColor = COLOR_WHITE;

		IRECT innerRect = mRECT;

		if (!mLabel.empty())
		{
			g.MeasureText(t, mLabel.c_str(), innerRect);
			g.DrawText(t, mLabel.c_str(), mRECT);
		}

		innerRect = mRECT.GetFromRight(mRECT.W() - innerRect.W() - 10);

		t.mFGColor = ui_control_accent;

		g.FillRoundRect(bgcol_dark, innerRect);
		g.DrawRoundRect(ui_control_accent, innerRect);

		IRECT textBounds = innerRect.GetPadded(-10, -5, 0, -5).GetFromLeft(innerRect.W() - 30);
		IRECT b;

		std::string finalPathToDraw = "--";

		if (mItemStrings.size() > 0 && mSelectedItem >= 0)
		{
			finalPathToDraw = mItemStrings[mSelectedItem];
		}

		g.MeasureText(t, finalPathToDraw.c_str(), b);
		if (b.W() > textBounds.W())
		{
			while (b.W() > textBounds.W())
			{
				finalPathToDraw.pop_back();
				g.MeasureText(t, finalPathToDraw.c_str(), b);
			}
			finalPathToDraw += "...";
		}

		g.DrawText(t, finalPathToDraw.c_str(), textBounds);

		IRECT changeRect = innerRect.GetFromRight(30);
		if (mIsRectMouseOver)
			g.FillRoundRect(ui_control_accent.WithOpacity(0.25f), innerRect);
		g.DrawText(t, "...", changeRect.GetHShifted(10));
		g.DrawLine(ui_control_accent, innerRect.R - 25, innerRect.B - 5, innerRect.R - 25, innerRect.T + 5);
	}

	void OnMouseOver(float x, float y, const IMouseMod& mod) override
	{
		mIsRectMouseOver = false;

		if (mRECT.Contains(x, y))
			mIsRectMouseOver = true;

		SetDirty(false);
	}

	void OnMouseOut() override
	{
		mIsRectMouseOver = false;
		SetDirty(false);
	}

	void OnMouseDown(float x, float y, const IMouseMod& mod) override
	{
		if (mIsRectMouseOver)
		{
			if (!mItemStrings.empty())
			{
				IPopupMenu menu = IPopupMenu("Items", {});
				for (int i = 0; i < mItemStrings.size(); i++)
				{
					menu.AddItem(new IPopupMenu::Item(mItemStrings[i].c_str()));
				}
				GetUI()->CreatePopupMenu(*this, menu, mRECT);
			}
		}
	}

	void OnPopupMenuSelection(IPopupMenu* pSelectedMenu, int valIdx) override
	{
		if (pSelectedMenu)
		{
			mSelectedItem = pSelectedMenu->GetChosenItemIdx();
			SetValue((double)mSelectedItem / (mItemStrings.size() - 1));
			SetDirty(true);
		}
	}
};