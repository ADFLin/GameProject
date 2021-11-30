#ifndef BubbleScene_h__
#define BubbleScene_h__

#include "BubbleLevel.h"
#include "RenderLayer.h"

class IGraphics2D;
class ActionTrigger;
struct ActionPort;

namespace Bubble
{
	enum BubbleActionKey
	{
		ACT_BB_ROTATE_LEFT ,
		ACT_BB_ROTATE_RIGHT ,
		ACT_BB_SHOOT ,
		ACT_BB_MOUSE_ROTATE ,
	};

	class Scene : public RenderLayer
	{
	public:
		Scene( LevelListener* listener );

		struct SceneBubble;
		typedef std::list< SceneBubble* > BubbleList;

		Level& getLevel(){ return mLevel; }
		void   tick();
		void   updateFrame( int frame );

		void   shoot();
		void   rotateRight( float delta );
		void   render( IGraphics2D& g );

		void   fireAction(ActionPort port, ActionTrigger& tigger );

		void onUpdateShootBubble( Level::Bubble* bubble , unsigned result );
		void onRemoveShootBubble( Level::Bubble* bubble );
		void onPopCell( BubbleCell& cell , int index );
		void onAddFallCell( BubbleCell& cell , int index );


		long       mTimeCount;
		BubbleList mShootList;
		BubbleList mFallList;
		float      mFallBubbleAcc;

		void render(IGraphics2D& g , Vector2 const& pos );
		void renderBubbleList(IGraphics2D& g , Vector2 const& pos , BubbleList& bList );
		void renderBubble(IGraphics2D& g , Vector2 const& pos , int color );
		void renderDbg(IGraphics2D& g , Vector2 const& pos , int index );
		void renderBackground(IGraphics2D& g , Vector2 const& pos );
		void renderLauncher(IGraphics2D& g , Vector2 const& pos );


		Level     mLevel;
	};

}

#endif // BubbleScene_h__