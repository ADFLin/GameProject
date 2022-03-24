#include "StageBase.h"

#include "GameGlobal.h"
#include "GameGUISystem.h"

#include "Holder.h"
#include "Core/IntegerType.h"
#include "CppVersion.h"
#include "Random.h"

#include <string>

namespace GGJ
{
	using String = std::string;

	class Random
	{
	public:
		int nextInt(){ return ::rand();}
	};
	enum class ObjectId
	{
		Ball = 0 ,
		Candlestick, 
		Book ,
		Doll ,

		Lantern ,
		MugCube ,

		MagicLight , 
		Door ,

		NumCondObject = 4,
	};

	enum class CondDir
	{
		Front = 0,
		Right = 1,
		Back  = 2,
		Left  = 3,
		Top    = 4,
		Bottom = 5,
	};

	inline CondDir invertDir(CondDir dir)
	{
		static const CondDir invDirMap[] = { CondDir::Back , CondDir::Left , CondDir::Front , CondDir::Right , CondDir::Bottom , CondDir::Top };
		return invDirMap[(int)dir];
	}

	enum class WallName
	{
		Bed,
		Number,
		Door,
		Clock,

		Num,
	};

	enum class ColorId
	{
		Red = 0,
		Yellow,
		Green,
		White,

		Num,
	};

	enum ValueProperty
	{
		PrimeNumber = 0,
		DividedBy4 ,
		DSumIsBiggerThan10 ,
		DOIsBiggerThanDT ,


		NumProp ,
	};

	class Utility
	{
	public:
		
		static int RandomValueForProperty( Random& rand , ValueProperty prop );
		static int ValuePropertyFlags( int value );
		static bool IsPrime( int value );
		static int* MakeRandSeq( Random& rand, int num, int start, int buf[] );
		static bool* MakeRandBool(Random& rand , int num , int numTrue , bool buf[] );
		static uint8* MakeRandBool(Random& rand, int num, int numTrue, uint8 buf[] );
	};

	class WorldCondition
	{
	public:
		WorldCondition();

		WallName getWallName(int idx) const { return walls[idx].name; }
		ColorId  getWallColor(int idx) const { return walls[idx].color; }

		int    indexWallHaveLight;
		int    valueForNumberWall;
		uint32 valuePropertyFlags;

		static const int MaxCondObjectNum = 10;

		int  getObjectNum( ObjectId id );
		int  getTopFireLightingNum();
		int  getRelDirIndex( int index , CondDir dir , bool bFaceWall );
		int  getRelDirInvIndex(int index, CondDir dir, bool bFaceWall);
		bool isTopFireLighting(WallName nearWallName, CondDir dir, bool bFaceWall);
		bool isTopFireLighting(int idx);

		ColorId getObjectColor(ObjectId id);
		int getRelDirWallIndex( int idx , CondDir dir )
		{
			return getRelDirIndex(idx, dir, false);
		}

		bool checkWallCond( WallName a , WallName b , CondDir dir );
		int  getWallIndex(WallName name);
		WallName getRelDirWall( WallName name , CondDir dir );

		void generate(Random& rand);

		void addObject( ObjectId id , int num , ColorId color )
		{
			ObjectInfo info;
			info.id = id;
			info.num = num;
			info.color = color;
			objects.push_back(info);
		}

	private:

		struct ObjectInfo
		{
			ObjectId id;
			ColorId color;
			int num;
		};
		std::vector<ObjectInfo> objects;

		struct WallInfo
		{
			WallName name;
			ColorId  color;
		};
		WallInfo walls[4];
		bool bTopFireLighting[4];
	};

	struct CondExprElement
	{
		enum Type
		{
			eWallName,
			eObject,
			eColor,
			eIntValue,
			eDir,
			eFaceFront ,
		};

		Type type;
		union
		{
			ObjectId obj;
			ColorId  color;
			WallName wall;
			CondDir  dir;
			int      intValue;
		};

#define TYPE_FUN( TYPE , TYPEID , MEMBER )\
		operator TYPE() { assert(type == TYPEID); return MEMBER; }\
		void set(TYPE value)\
		{\
			type = TYPEID;\
			MEMBER = value;\
		}

		TYPE_FUN(ObjectId, eObject, obj)
		TYPE_FUN(WallName, eWallName, wall)
		TYPE_FUN(ColorId, eColor, color)
		TYPE_FUN(CondDir, eDir, dir)
		TYPE_FUN(int, eIntValue, intValue)

#undef TYPE_FUN

		void setFaceFront(bool bFace)
		{
			type = eFaceFront;
			intValue = (bFace) ? 1 : 0;
		}
		String toString();

		static String toString( ColorId id );
		static String toString( ObjectId id );
		static String toString( int value );
		static String toString( WallName name );
		static String toString( CondDir dir );
	};


	class CondExpression
	{
	public:
		String toString( int idx ){ return mElements[idx].toString(); }

		virtual void generate(Random& rand) = 0;
		virtual void generate(Random& rand, WorldCondition& worldCond) = 0;
		virtual bool test(WorldCondition& worldCond) = 0;
		virtual String getContent() = 0;
	protected:
		std::vector< CondExprElement > mElements;
		int mIdxContent;
	};

	class WallDirCondExpression : public CondExpression
	{
	public:
		void generate(Random& rand) override;
		void generate(Random& rand ,  WorldCondition& worldCond ) override;
		bool test(WorldCondition& worldCond) override;
		String getContent() override;

	};

	class TopLightCondExpression : public CondExpression
	{
	public:
		
		void generate(Random& rand) override;
		void generate(Random& rand, WorldCondition& worldCond) override;
		bool test(WorldCondition& worldCond) override;
		String getContent() override;
	};

	class WallColorCondExpression : public CondExpression
	{
	public:
		void generate(Random& rand) override;
		void generate(Random& rand, WorldCondition& worldCond) override;
		bool test(WorldCondition& worldCond) override;
		String getContent() override;
	};
	
	class ObjectNumCondExpression : public CondExpression
	{
	public:
		bool test(WorldCondition& worldCond) override;
		void generate(Random& rand) override;
		void generate(Random& rand, WorldCondition& worldCond) override;
		String getContent() override;
	};

	class ObjectColorCondExpression : public CondExpression
	{
	public:
		void generate(Random& rand) override;
		void generate(Random& rand, WorldCondition& worldCond) override;
		bool test(WorldCondition& worldCond) override;
		String getContent() override;
	};

	class WallNumberValueCondExpression : public CondExpression
	{
	public:
		void generate(Random& rand) override;
		void generate(Random& rand, WorldCondition& worldCond) override;
		bool test(WorldCondition& worldCond) override;
		String getContent() override;
	};


	class Condition
	{
	public:
		
		~Condition();

		int             getExpressionNum() { return mExprList.size(); }
		CondExpression* getExpression( int idx ) { return mExprList[idx]; }
		String getTarget() { return CondExprElement::toString(targetId); }

		void generate(Random& rand, WorldCondition& worldCond, int numExpr);
		void generateRandom(Random& rand, WorldCondition& worldCond, int numExpr, int numIn);

		ObjectId targetId;
		bool     bValid;

	private:
		void cleanup();
		std::vector< CondExpression* > mExprList;
	};

	class ConditionTable
	{
	public:
		void cleanup();

		void generate( Random& rand , WorldCondition& worldCond , int numSel , int numExpr , int num );
		bool isObject( ObjectId id );
		Condition& getCondition(int idx) { return conditions[idx]; }
		bool isCondition(int idx)
		{
			return conditions[idx].bValid;
		}

		int numCondition;
		int numSelection;
		std::vector< Condition > conditions;
	};


	class TestStage : public StageBase
	{
		using BaseClass = StageBase;
	public:

		WorldCondition worldCond;
		ConditionTable condTable;

		Random rand;
		TestStage(){}

		bool onInit() override;

		void onEnd() override
		{

		}

		void onUpdate( long time ) override
		{
			BaseClass::onUpdate( time );

			int frame = time / gDefaultTickTime;
			for( int i = 0 ; i < frame ; ++i )
				tick();

			updateFrame( frame );
		}

		void onRender( float dFrame ) override;

		void restart();


		void tick()
		{

		}

		void updateFrame( int frame )
		{

		}

		MsgReply onMouse(MouseMsg const& msg) override
		{
			return BaseClass::onMouse(msg);
		}

		MsgReply onKey(KeyMsg const& msg) override
		{
			if ( msg.isDown() )
			{
				switch (msg.getCode())
				{
				case EKeyCode::R: restart(); break;
				}
			}
			return BaseClass::onKey(msg);
		}

	protected:

	};
}