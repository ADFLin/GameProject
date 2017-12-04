#ifndef FBirdStage_h__
#define FBirdStage_h__

#include "StageBase.h"
#include "FBLevel.h"

#include "RenderGL/GLCommon.h"
#include "RenderGL/GLDrawUtility.h"


namespace FlappyBird
{
	using RenderGL::RHITexture2D;
	using RenderGL::DrawUtiltiy;

	class TrainManager;
	class TrainData;

	namespace TextureID
	{
		enum 
		{
			Bird ,
			Pipe ,
			Background_A,
			Background_B,
			Ground ,
			Number,

			Count,
		};
	};

	class LevelStage : public StageBase
	{
		typedef StageBase BaseClass;
	public:
		LevelStage();
		~LevelStage();

		enum
		{
			UI_SAVE_TRAIN_DATA = BaseClass::NEXT_UI_ID,
			UI_LOAD_TRAIN_DATA ,
			UI_TOGGLE_FAST_TICK ,
			UI_USE_POOL_DATA ,
			NEXT_UI_ID ,
		};

		virtual bool onInit();
		virtual void onUpdate(long time);

		GameLevel& getLevel() { return mLevel; }

		void onRender( float dFrame );



		void restart();
		void notifyGameOver( GameLevel& level);
		CollisionResponse notifyBridCollsion(BirdEntity& bird, ColObject& obj);

		void tick();

		void updateFrame( int frame )
		{

		}

		bool onMouse( MouseMsg const& msg );
		bool onKey(unsigned key, bool isDown);
		bool onWidgetEvent( int event , int id , GWidget* ui );



		
	protected:

		void removeTrainData();

		bool mbFastTick = false;
		bool mbTrainMode = true;
		float topFitness = 0;


		Vector2 convertToScreen(Vector2 const& pos);

		void drawScreenHole(IGraphics2D& g, Vec2i const& pos, Vec2i const& size);
		void drawBird(IGraphics2D& g, BirdEntity& bird);
		void drawPipe(IGraphics2D& g, ColObject const& obj);
		void drawNumber(IGraphics2D& g, int number, float width );

		float getTextureSizeRatio( int id )
		{
			return float( mTextures[id].getSizeX() ) / mTextures[id].getSizeY();
		}



		bool loadResource();

		RHITexture2D mTextures[ TextureID::Count ];
		bool mbDebugDraw = false;

		void drawTexture(int id, Vector2 const& pos, Vector2 const& size, Vector2 const& pivot);
		void drawTexture(int id, Vector2 const& pos, Vector2 const& size, Vector2 const& pivot, Vec2i const& framePos , Vec2i const& frameDim );
		void drawTexture(int id, Vector2 const& pos, Vector2 const& size, Vector2 const& pivot, Vector2 const& texPos, Vector2 const& texSize);

		int blockType = 0;
		int backgroundType = 0;

		int  mMaxScore;
		int  mScore;
		
		int  mTimerProduce;
		float moveDistance;

		
		std::unique_ptr< TrainData >    mTrainData;
		std::unique_ptr< TrainManager > mTrainManager;

		GameLevel mLevel;
		BirdEntity mBird;
	};


}//namespace FlappyBird

#endif // FBirdStage_h__
