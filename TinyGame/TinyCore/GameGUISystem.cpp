#include "TinyGamePCH.h"
#include "GameGUISystem.h"

#include "RenderUtility.h"
#include "RHI/RHIGraphics2D.h"

#include "StageBase.h"
#include "TaskBase.h"

// The old Conservative Policy (GUILayerPolicy) has been removed as the Aggressive Policy now handles overlaps dynamically.

// Aggressive Policy: Elements sorted ONLY by type-phase globally.
// All shapes across all widgets → layer 0..N
// All texture rects across all widgets → layer 100000..100000+N
// All text across all widgets → layer 200000..200000+N
// This allows BatchedRender's stable_sort + stateIndex secondary sort
// to merge same-state elements across different widgets.
// Draw calls = O(NumDistinctStates) ≈ 2-4 typically (no-texture shapes, font-texture text, etc.)
//
// IMPORTANT: This policy sacrifices strict per-widget z-ordering between different
// element types. For example, a button's text and background from widget A could
// interleave with widget B's elements. This is acceptable for flat UI layouts
// (like MainMenu's card grid) where widgets don't overlap, but should NOT be
// used for overlapping UI (e.g., modal dialogs on top of other widgets).
class GUIAggressiveLayerPolicy : public RHIGraphics2D::LayerPolicy
{
public:
	static constexpr int32 PHASE_SHAPE      = 0;
	static constexpr int32 PHASE_TEXTURE    = 1;
	static constexpr int32 PHASE_TEXT       = 2;
	static constexpr int32 NUM_PHASES       = 3;
	static constexpr int32 PHASE_RANGE      = 100000;

	// Each type-phase gets a sub-counter to preserve submission order within the phase.
	// This ensures overlapping shapes still render in correct order relative to each other.
	int32 mPhaseCounters[NUM_PHASES] = {};
	int32 mBaseLayer = 0;

	void reset()
	{
		for (int i = 0; i < NUM_PHASES; ++i)
			mPhaseCounters[i] = 0;
		mBaseLayer = 0;
	}

	int32 getLayer(Render::RenderBatchedElement const& element, RHIGraphics2D::RenderState const& state) override
	{
		int32 phase;
		switch (element.type)
		{
		case Render::RenderBatchedElement::Text:
		case Render::RenderBatchedElement::ColoredText:
			phase = PHASE_TEXT;
			break;
		case Render::RenderBatchedElement::TextureRect:
			phase = PHASE_TEXTURE;
			break;
		default:
			phase = PHASE_SHAPE;
			break;
		}
		return mBaseLayer * (NUM_PHASES * PHASE_RANGE) + phase * PHASE_RANGE + mPhaseCounters[phase]++;
	}
};
GUIAggressiveLayerPolicy g_GUIAggressiveLayerPolicy;

GUISystem::GUISystem()
{
	mGuiDelegate  = nullptr;
	mHideWidgets  = false;
	mbSkipCharEvent = false;
	mbSkipKeyEvent  = false;
	mbSkipMouseEvent = false;
}

bool GUISystem::initialize(IGUIDelegate&  guiDelegate)
{
	mGuiDelegate = &guiDelegate;
	return true;
}

void GUISystem::finalize()
{

}

Vec2i GUISystem::calcScreenCenterPos( Vec2i const& size )
{
	Vec2i pos;
	pos.x = ( Global::GetScreenSize().x - size.x ) / 2;
	pos.y = ( Global::GetScreenSize().y - size.y ) / 2;
	return pos;
}

void GUISystem::addTask( TaskBase* task , bool beGlobal /*= false */ )
{
	mGuiDelegate->addGUITask(task, beGlobal);
}

void GUISystem::sendMessage( int event , int id , GWidget* ui )
{
	mGuiDelegate->dispatchWidgetEvent(event, id, ui);
}

void GUISystem::removeHotkey( GWidget* ui )
{
	for( HotkeyList::iterator iter = mHotKeys.begin();
		iter != mHotKeys.end() ; )
	{
		if ( iter->ui == ui )
		{
			iter = mHotKeys.erase( iter );
		}
		else
		{
			++iter;
		}
	}
}

void GUISystem::addHotkey( GWidget* ui , ControlAction key )
{
	HotkeyInfo info;
	info.ui  = ui;
	info.keyAction = key;
	mHotKeys.push_back( info );
}

void GUISystem::scanHotkey( InputControl& inputConrol )
{
	//for ( HotkeyList::iterator iter = mHotKeys.begin();
	//	iter != mHotKeys.end() ; ++iter )
	//{
	//	iter->ui->markDestroyDefferred();
	//	if ( inputConrol.checkKey( 0 , iter->keyAction ) )
	//	{
	//		iter->ui->onHotkey( iter->keyAction );
	//	}
	//}

	//mUIManager.cleanupDestroyUI();

	//for ( HotkeyList::iterator iter = mHotKeys.begin();
	//	iter != mHotKeys.end() ; ++iter )
	//{
	//	iter->ui->unmarkDestroyDefferred();
	//}
}

GWidget* GUISystem::showMessageBox( int id , char const* msg , EMessageButton::Type buttonType)
{
	Vec2i pos = calcScreenCenterPos( GMsgBox::BoxSize );
	GMsgBox* box = new GMsgBox( id , pos , NULL , buttonType);
	box->setTitle( msg );
	box->setAlpha( 0.8f );
	addWidget( box );
	doModel(box);
	return box;
}

int GUISystem::getModalId()
{
	GWidget* ui = getManager().getModalWidget();
	if ( ui )
		return ui->getID();
	return UI_ANY;
}

void GUISystem::updateFrame( int frame , long tickTime )
{
	struct UpdateFrameVisitor
	{
		void operator()( GWidget* w )
		{
			w->updateFrame( frame );
		}
		int frame;
	} visitor;

	visitor.frame = frame;
	getManager().visitWidgets( visitor );
	mTweener.update( frame * tickTime );
}

MsgReply GUISystem::procMouseMsg(MouseMsg const& msg)
{
	if( mHideWidgets || mbSkipMouseEvent )
		return MsgReply::Unhandled();
	return getManager().procMouseMsg(msg);
}

MsgReply GUISystem::procKeyMsg(KeyMsg const& msg)
{
	if( mHideWidgets || mbSkipKeyEvent )
		return MsgReply::Unhandled();
	return getManager().procKeyMsg(msg);
}

MsgReply GUISystem::procCharMsg(unsigned code)
{
	if( mHideWidgets || mbSkipCharEvent )
		return MsgReply::Unhandled();
	return getManager().procCharMsg(code);
}

void GUISystem::addWidget( GWidget* widget, WidgetLayoutLayer* layer)
{
	widget->layer = layer;
	mUIManager.addWidget( widget );
}

void GUISystem::cleanupWidget(bool bForceCleanup , bool bPersistentIncluded)
{
	mUIManager.cleanupWidgets(bPersistentIncluded);
	mTweener.clear();
	if( bForceCleanup )
		mUIManager.cleanupPaddingKillWidgets();
}

void GUISystem::doModel(GWidget* ui)
{
	mUIManager.startModal(ui);
}

void GUISystem::render(IGraphics2D& g)
{
	if ( mHideWidgets )
		return;

	RHIGraphics2D* gRHI = nullptr;
	if (g.isUseRHI())
	{
		gRHI = &g.getImpl<RHIGraphics2D>();
	}

	if (gRHI)
	{
		g_GUIAggressiveLayerPolicy.reset();
		gRHI->setLayerPolicy(&g_GUIAggressiveLayerPolicy);
	}

	struct RenderedWidgetInfo
	{
		Vec2i pos;
		Vec2i size;
		bool  bIsLayered;
		int32 baseLayer;
		
		bool isIntersect(RenderedWidgetInfo const& other) const
		{
			if (bIsLayered || other.bIsLayered)
				return true; // Conservative fallback for transformed widgets

			return !(pos.x + size.x <= other.pos.x || pos.x >= other.pos.x + other.size.x ||
					 pos.y + size.y <= other.pos.y || pos.y >= other.pos.y + other.size.y);
		}
	};
	TArray<RenderedWidgetInfo> renderedWidgets;

	for (auto ui = mUIManager.createTopWidgetIterator(); ui; ++ui)
	{
		if (!ui->isShow())
			continue;

		int32 baseLayer = 0;
		if (gRHI)
		{
			Vec2i pos = ui->getWorldPos();
			Vec2i size = ui->getSize();
			bool bIsLayered = (ui->layer != nullptr);

			RenderedWidgetInfo currentWidget = { pos, size, bIsLayered, 0 };

			for (auto const& w : renderedWidgets)
			{
				if (currentWidget.isIntersect(w))
				{
					if (w.baseLayer >= baseLayer)
						baseLayer = w.baseLayer + 1;
				}
			}
			currentWidget.baseLayer = baseLayer;
			renderedWidgets.push_back(currentWidget);

			g_GUIAggressiveLayerPolicy.mBaseLayer = baseLayer;
		}

		if (ui->layer)
		{
			if (gRHI)
			{
				gRHI->pushXForm();
				gRHI->transformXForm(ui->layer->worldToScreen, true);
				ui->renderAll();
				gRHI->popXForm();
			}
			else
			{
				ui->renderAll();
			}
		}
		else
		{
			ui->renderAll();
		}
	}

	if (gRHI)
	{
		gRHI->setLayerPolicy(nullptr);
	}
}


void GWidget::doRenderAll()
{
	prevRender();
	render();
	postRender();

	bool const bClipTest = haveChildClipTest();

	for (auto child = createChildrenIterator(); child; ++child)
	{
		if (!child->isShow())
			continue;
		if (bClipTest && !child->clipTest())
			continue;

		child->renderAll();
	}
	postRenderChildren();
}

void GUISystem::update()
{
	mUIManager.update();
}

GWidget* GUISystem::findTopWidget(int id)
{
	for( auto child = mUIManager.createTopWidgetIterator() ; child ; ++child )
	{
		if ( child->getID() == id )
			return &(*child);
	}
	return nullptr;
}

