#include "RichBase.h"

#include "RichLevel.h"
#include "RichEditInterface.h"

#include "DrawEngine.h"
#include "DataStructure/IntrList.h"
#include "Tween.h"


namespace Rich
{
	class AnimSetupHelper;


	struct RenderCoord
	{
		float xScan;
		float yScan;
		Vec2i screen;
		Vec2i map;
	};

	enum ViewDir
	{
		eNorth ,
		eEast  ,
		eSouth ,
		eWest  ,
	};


	/// 
	///   --------> sx  
	///  |   offset/ cx	
	///	 |        /	
	/// \|/       \   
	/// sy         \
	//             cy

	struct RenderView
	{
		void calcCoord( RenderCoord& coord , Vec2i const& mapPos );
		void calcCoord( RenderCoord& coord , float xScan , float yScan );


		Vec2i convScanToMapPos( int xScan , int yScan ) const;
		Vec2i convScanToMapPos( float xScan , float yScan ) const;
		Vec2i convScreenToMapPos( Vec2i const& sPos ) const;
		Vec2i convMapToScreenPos( Vec2i const& mapPos ) const;
		Vec2i convMapToScreenPos( float xMap , float yMap );
		Vec2i convScanToScreenPos( float xScan , float yScan ) const;
		void  convMapToScanPos( Vec2i const& mapPos , float& sx , float& sy ) const;

		Vec2i calcMapToScreenOffset( float dx , float dy );
		Vec2i calcScanToScreenOffset( float dx , float dy );
		static Vec2i calcTileOffset( int w , int h );

		Vec2i   screenOffset;
		Vec2i   screenSize;
		ViewDir dir;
	};


	class Animation
	{
	public:
		Animation(){ mNext = nullptr; }
		virtual void setup( AnimSetupHelper& helper ){}
		virtual bool update( long time ) = 0;
		virtual void skip(){}
		virtual void render( Graphics2D& g ){}
	private:
		friend class Scene;
		Animation* mNext;
	};

	typedef Tween::GroupTweener< long > Tweener;
	class AnimSetupHelper
	{
	public:
		Tweener& getTweener(){ return mTweener; }
	private:
		Tweener mTweener;
	};

	class RenderComp : public IComponent
	{
	public:
		virtual void render( Graphics2D& g ){}

		virtual void onUnlink(){ hook.unlink(); }
		virtual void update( long time ){}
		virtual void destroy(){ delete this; }

		Vector2    pos;
		LinkHook hook;
	};

	class ActorRenderComp : public RenderComp
	{
	public:
		ActorRenderComp()
		{
			bUpdatePos = true;
		}
		virtual void render( Graphics2D& g );
		virtual void update( long time );
		bool bUpdatePos;
	};

	class PlayerRenderComp : public ActorRenderComp
	{
	public:
		virtual void render( Graphics2D& g );

	};

	class Scene : public IWorldMessageListener
		        , public AnimSetupHelper
				, public IViewSelect
	{
	public:
		Scene();

		void setupLevel( Level& level )
		{
			mLevel = &level;
			mLevel->getWorld().addMsgListener( *this );
		}

		void update( long time );

		Level& getLevel(){ assert( mLevel ); return *mLevel; }

		void render( Graphics2D& g );

		bool haveAnimation(){ return mAnimCur != nullptr; }


		void render( Graphics2D& g , RenderView& view );


		void addAnim( Animation* anim );
		template< class T >
		T* createComponentT( Entity* entity )
		{
			T* comp = new T;
			mRenderList.push_back( comp );
			entity->addComponent( COMP_RENDER , comp );
			return comp;
		}

		virtual void onWorldMsg( WorldMsg const& msg );

		virtual bool calcCoord( Vec2i const& sPos , MapCoord& coord );

		Animation*    mAnimCur;
		Animation*    mAnimLast;

		typedef TIntrList< 
			RenderComp , 
			MemberHook< RenderComp , &RenderComp::hook > , 
			PointerType 
		> RenderCompList;


		bool       mbSkipMoveAnim;
		bool       mbAlwaySkipMoveAnim;

		RenderCompList mRenderList;
		Level*         mLevel;
	};

}//namespace Rich