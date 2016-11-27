#include "StageBase.h"

#include "GameGlobal.h"
#include "DrawEngine.h"
#include "GameGUISystem.h"

#include "THolder.h"
#include "CppVersion.h"
#include "Random.h"

#include <string>

namespace GGJ
{
	typedef std::string String;

	class Random
	{
	public:
		int nextInt(){ return ::rand();}
	};
	enum ObjectId
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

	enum CondDir
	{
		Front = 0,
		Right = 1,
		Back  = 2,
		Left  = 3,
		Top    = 4,
		Bottom = 5,
	};

	namespace WallName
	{
		enum Enum
		{
			Bed,
			Number,
			Door,
			Clock,

			Num ,
		};
	}

	enum ColorId
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
		
		static int getRandomValueForProperty( Random& rand , ValueProperty prop );
		static int getValuePropertyFlag( int value );
		static bool IsPrime( int value );
		static int* makeRandSeq( Random& rand, int num, int start, int buf[] );
		static bool* makeRandBool(Random& rand , int num , int numTrue , bool buf[] );
	};

	struct WallInfo
	{
		WallName::Enum name;
		ColorId color;
	};

	struct ObjectInfo
	{
		ObjectId id;
		ColorId color;
		int num;
	};



	class WorldCondition
	{
	public:
		WallInfo walls[4];
		int indexWallHaveLight;
		int valueForNumberWall;
		int valuePropertyFlag;

		std::vector<ObjectInfo> objects;

		static const int MaxCondObjectNum = 10;

		bool bTopFireLighting[4];

		WorldCondition();

		int getObjectNum( ObjectId id );

		int getTopFireLightingNum();

		int getRelDirIndex( int index , CondDir dir , bool bFaceWall );

		bool isTopFireLighting( WallName::Enum nearWallName , CondDir dir , bool bFaceWall );

		bool isTopFireLighting(int idx);

		ColorId getObjectColor(ObjectId id);

		int getRelDirWallIndex( int idx , CondDir dir )
		{
			return getRelDirIndex(idx, dir, false);
		}

		bool checkVaild( WallName::Enum a , WallName::Enum b , CondDir dir );

		int getWallIndex(WallName::Enum name);
		WallName::Enum getRelDirWall( WallName::Enum name , CondDir dir );

		void generate(Random& rand);

		void addObject( ObjectId id , int num , ColorId color )
		{
			ObjectInfo info;
			info.id = id;
			info.num = num;
			info.color = ColorId::White;
			objects.push_back(info);
		}
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
			WallName::Enum wall;
			int      value;
			int      meta;
			CondDir  dir;
		};
		

		void setWall( WallName::Enum wallName )
		{
			type = eWallName;
			wall = wallName;
		}
		void setObject( ObjectId id )
		{
			type = eObject;
			obj = id;
		}

		static String toString( ColorId id );
		static String toString( ObjectId id );
		static String toString( int value );
		static String toString( CondExprElement ele );
		static String toString( WallName::Enum name );
		static String toString( CondDir dir );
	};


	class CondExpression
	{
	public:
		std::vector< CondExprElement > elements;
		int IdxContent;

		String toString( int idx )
		{
			return CondExprElement::toString( elements[idx]);
		}

		virtual bool testVaild( WorldCondition& worldCond){ return false; }
		virtual void generate(Random& rand){}
		virtual void generateVaild(Random& rand, WorldCondition& worldCond) { }
		virtual String getContent(){ return ""; }



	};

	class WallDirCondExpression : public CondExpression
	{
	public:
		bool testVaild( WorldCondition& worldCond) override;
		void generate(Random& rand) override;
		void generateVaild(Random& rand ,  WorldCondition& worldCond ) override;
		String getContent() override;

	};

	class TopLightCondExpression : public CondExpression
	{
	public:
		bool testVaild(WorldCondition& worldCond) override;
		void generate(Random& rand) override;
		void generateVaild(Random& rand, WorldCondition& worldCond) override;
		String getContent() override;
	};

	class WallColorCondExpression : public CondExpression
	{
	public:
		bool testVaild(WorldCondition& worldCond) override;
		void generate(Random& rand) override;
		void generateVaild(Random& rand, WorldCondition& worldCond) override;
		String getContent() override;
	};
	
	class ObjectNumCondExpression : public CondExpression
	{
	public:
		bool testVaild(WorldCondition& worldCond) override;
		void generate(Random& rand) override;
		void generateVaild(Random& rand, WorldCondition& worldCond) override;
		String getContent() override;
	};

	class ObjectColorCondExpression : public CondExpression
	{
		
	public:
		bool testVaild(WorldCondition& worldCond) override;
		void generate(Random& rand) override;
		void generateVaild(Random& rand, WorldCondition& worldCond) override;
		String getContent() override;
	};


	class WallNumberValueCondExpression : public CondExpression
	{
	public:
		bool testVaild(WorldCondition& worldCond) override;
		void generate(Random& rand) override;
		void generateVaild(Random& rand, WorldCondition& worldCond) override;
		String getContent() override;

	};

	class Condition
	{
	public:
		static const int TotalExprNum = 6;
		std::vector< CondExpression* > exprList;

		~Condition();

		void cleanup();

		ObjectId targetId;

		bool bVaild;

		int getExprissionNum() { return exprList.size(); }

		String getTarget() { return CondExprElement::toString(targetId); }
		String getContent(int idxExpr)
		{
			return exprList[idxExpr]->getContent();
		}
		bool testVaild( WorldCondition& worldCond , int idxExpr )
		{
			return exprList[idxExpr]->testVaild(worldCond);
		}
		CondExpression* CreateExpression( int idx );

		void generateVaild(Random& rand, WorldCondition& worldCond, int numExpr);

		void generateRandom(Random& rand, WorldCondition& worldCond, int numExpr, int numInvaild);
	};

	class ConditionTable
	{
	public:
		void cleanup();

		void generate( Random& rand , WorldCondition& worldCond , int numSel , int numExpr , int numVaild );
		bool isVaildObject( ObjectId id );
		Condition& getCondition(int idx) { return conditions[idx]; }
		bool isVaildCondition(int idx)
		{
			return conditions[idx].bVaild;
		}

		int numConditionVaild;
		int numSelection;
		std::vector< Condition > conditions;
	};


	class TestStage : public StageBase
	{
		typedef StageBase BaseClass;
	public:

		WorldCondition worldCond;
		ConditionTable condTable;

		Random rand;
		TestStage(){}

		virtual bool onInit();

		virtual void onEnd()
		{

			//::Global::getDrawEngine()->stopOpenGL();
		}

		virtual void onUpdate( long time )
		{
			BaseClass::onUpdate( time );

			int frame = time / gDefaultTickTime;
			for( int i = 0 ; i < frame ; ++i )
				tick();

			updateFrame( frame );
		}

		void onRender( float dFrame );

		void restart();


		void tick()
		{

		}

		void updateFrame( int frame )
		{

		}

		bool onMouse( MouseMsg const& msg )
		{
			if ( !BaseClass::onMouse( msg ) )
				return false;
			return true;
		}

		bool onKey( unsigned key , bool isDown )
		{
			if ( !isDown )
				return false;

			switch( key )
			{
			case 'R': restart(); break;
			}
			return false;
		}

	protected:

	};
}