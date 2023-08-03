#include "RichBase.h"

#include "RichLevel.h"
#include "RichEditInterface.h"

#include "DrawEngine.h"
#include "DataStructure/IntrList.h"
#include "Tween.h"
#include "RHI/TextureAtlas.h"

namespace Rich
{
	class AnimSetupHelper;


	struct RenderCoord
	{
		Vector2 scan;
		Vector2 screen;
		Vec2i map;
	};

	enum ViewDir
	{
		eNorth ,
		eEast  ,
		eSouth ,
		eWest  ,
	};



	struct RenderView
	{
		void calcCoordFromMap( RenderCoord& coord , Vec2i const& mapPos );
		void calcCoordFromScan( RenderCoord& coord, Vector2 const& scanPos);

		Vector2 convScanToMapPos(Vector2 const& scanPos) const;
		Vector2 convScreenToMapPos( Vec2i const& sPos ) const;
		Vector2 convMapToScreenPos( Vec2i const& mapPos ) const;
		Vector2 convMapToScreenPos(Vector2 const& mapPos) const;
		Vector2 convScanToScreenPos(Vector2 const& scanPos) const;
		Vector2 convScreenToScanPos(Vector2 const& screenPos) const;
		Vector2 convMapToScanPos(Vector2 const& mapPos) const;
		Vector2 calcMapToScreenOffset( float dx , float dy );
		Vector2 calcScanToScreenOffset(Vector2 const& scanOffset);
		void update();

		Vector2 titleSize;
		Vector2 scanTileSize;
		Vector2 screenOffset = Vector2::Zero();
		Vector2 screenSize;
		float   zoom = 1.0f;
		ViewDir dir = ViewDir::eNorth;
	};

	class AnimEventHandler
	{
	public:
		virtual ~AnimEventHandler() = default;
		virtual void handleAnimFinish() {}
	};
	class Animation
	{
	public:
		Animation(){ mNext = nullptr; }
		virtual void setup( AnimSetupHelper& helper ){}
		virtual bool update( long time ) = 0;
		virtual void skip(){}
		virtual void render(IGraphics2D& g){}
		virtual void renderDebug(IGraphics2D& g){}
	private:
		friend class Scene;
		friend class WaitAnimation;
		AnimEventHandler* mHandler = nullptr;
		Animation* mNext;
	};

	class WaitAnimation : public IYieldInstruction
		                , public AnimEventHandler
	{
	public:
		WaitAnimation(Animation* anim)
		{
			mAnim = anim;
		}

		virtual void handleAnimFinish() 
		{
			Coroutines::Resume(mYieldHandle);
		}

		void startYield(YieldContextHandle handle) override
		{
			mAnim->mHandler = this;
			mYieldHandle = handle;
		}
		Animation* mAnim;
		YieldContextHandle mYieldHandle;
	};

	typedef Tween::GroupTweener< long > Tweener;
	class AnimSetupHelper
	{
	public:
		Tweener& getTweener(){ return mTweener; }

		virtual Vec2i calcScreenPos(MapCoord const& pos) = 0;
	private:
		Tweener mTweener;

	};

	class RenderComp : public IComponent
	{
	public:
		virtual void render(IGraphics2D& g ){}
		virtual void renderDebug(IGraphics2D& g) {}

		virtual void onUnlink(){ hook.unlink(); }
		virtual void update( long time ){}
		virtual void destroy(){ delete this; }

		Vector2  pos;
		LinkHook hook;
	};

	class ActorRenderComp : public RenderComp
	{
	public:
		ActorRenderComp()
		{
			bUpdatePos = true;
		}
		virtual void renderDebug(IGraphics2D& g);
		virtual void update( long time );
		bool bUpdatePos;
	};

	class PlayerRenderComp : public ActorRenderComp
	{
	public:
		virtual void renderDebug(IGraphics2D& g );

	};

	struct RenderParams
	{
		RenderView* view = nullptr;
		bool bDrawDebug = false;
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

		void reset()
		{
			mTileIdMap.clear();
			cleanupAnim();
		}

		void update( long time );

		Level& getLevel(){ assert( mLevel ); return *mLevel; }
		
		void initiliazeTiles();

		void render(IGraphics2D& g , RenderParams const& renderParams );

		void render(IGraphics2D& g , RenderView& view );

		void renderDebug(IGraphics2D &g);

		bool initializeRHI();
		void releaseRHI();


		void addAnim( Animation* anim , bool bNewGroup = true);


		void drawNumber(IGraphics2D& g, Vector2 const& pos, float width, int number, Vector2 const& pivot = Vector2::Zero());
		Vec2i calcScreenPos(MapCoord const& pos);

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


		struct AnimGroup
		{
			Animation* cur;
			Animation* last;

			void clearnup()
			{
				Animation* anim = cur;
				while (anim)
				{
					Animation* next = anim->mNext;
					delete anim;
					anim = next;
				}

				cur = nullptr;
				last = nullptr;
			}
		};
		bool upateAnim(AnimGroup& group, long time);
		void cleanupAnim()
		{
			for (auto& group : mPlayingAnims)
			{
				group.clearnup();
			}
			mPlayingAnims.clear();
		}
		TArray< AnimGroup > mPlayingAnims;

		typedef TIntrList< 
			RenderComp , 
			MemberHook< RenderComp , &RenderComp::hook > , 
			PointerType 
		> RenderCompList;


		Render::TextureAtlas mTextureAtlas;

		bool       mbSkipMoveAnim;
		bool       mbAlwaySkipMoveAnim;

		RenderView mView;

		RenderCompList mRenderList;
		Level*         mLevel = nullptr;


		struct ImageInfo
		{
			Vector2 size;
			Vector2 uvPos;
			Vector2 uvSize;
		};

		TArray< ImageInfo > mImageInfos;


		struct TileImagInfo
		{
			int id;
			Vector2 size;
			Vector2 center;
			int maskId;
		};

		TArray<TileImagInfo> mTileImages;

		struct ActorImageInfo
		{
			int id;
			Vector2 size;
			Vector2 center;
		};
		TArray<ActorImageInfo> mActorImages;

		int mNumberId;

		void updateAreaTile(Area* area);


		struct MapCoordHasher
		{
			uint32 operator()(MapCoord const& v) const
			{
				uint32 result = HashValue(v.x);
				result = HashCombine(result, v.y);
				return result;
			}
		};

		struct TileId
		{
			union
			{
				struct
				{
					uint8 type;
					uint8 params[3];
				};
				uint32 value;
			};

			static TileId Make(uint8 type, uint8 a = 0, uint8 b = 0, uint8 c = 0)
			{
				TileId result;
				result.type = type;
				result.params[0] = a;
				result.params[1] = b;
				result.params[2] = c;
				return result;
			}
		};

		std::unordered_map< MapCoord, TileId, MapCoordHasher > mTileIdMap;
	};

}//namespace Rich