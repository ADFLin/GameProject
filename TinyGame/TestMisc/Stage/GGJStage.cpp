#include "GGJStage.h"
#include "StageRegister.h"

namespace GGJ
{

	int Utility::RandomValueForProperty(Random& rand , ValueProperty prop)
	{
		int result= 1;
		switch (prop)
		{
		case ValueProperty::PrimeNumber:
			do
			{
				result = 1 + rand.nextInt() % 99;
			}
			while ( !Utility::IsPrime(result)  );
			break;
		case ValueProperty::DividedBy4:
			result = rand.nextInt() % 95 + 4;
			result = 4 * (result / 4);
			break;
		case ValueProperty::DSumIsBiggerThan10:
			while (true)
			{
				int DO = 1 + rand.nextInt() % 9;
				int DT = rand.nextInt() % 10;
				result = 10 * DT + DO;
				if (DO + DT > 10)
					break;
			}
			break;
		case ValueProperty::DOIsBiggerThanDT:
			while (true)
			{
				int DO = 1 + rand.nextInt() % 9;
				int DT = rand.nextInt() % 10;
				result = 10 * DT + DO;
				if (DO > DT)
					break;
			}
			break;

		}

		return result;
	}

	int Utility::ValuePropertyFlags(int value)
	{
		int result = 0;
		if (IsPrime(value))
			result |= 1 << ValueProperty::PrimeNumber;
		if( (value % 4) == 0 )
			result |= 1 << ValueProperty::DividedBy4;
		int DT = value / 10;
		int DO = value % 10;
		if (DO + DT > 10)
			result |= 1 << ValueProperty::DSumIsBiggerThan10;
		if (DO > DT)
			result |= 1 << ValueProperty::DOIsBiggerThanDT;

		return result;
	}

	bool Utility::IsPrime(int value)
	{
		if( value <= 1 )
			return false;

		for( int i = 3 ; i < value ; i+=2 )
		{
			if ((value % i) == 0)
				return false;
		}
		return true;
	}

	int* Utility::MakeRandSeq(Random& rand, int num, int start, int buf[])
	{
		int* result = buf;
		for (int i = 0; i < num; ++i)
		{
			result[i] = start + i;
		}
		for (int i = 0; i < num; ++i)
		{
			for (int j = 0; j < num; ++j)
			{
				int temp = result[i];
				int idx = rand.nextInt() % num;
				result[i] = result[idx];
				result[idx] = temp;
			}
		}
		return result;
	}

	bool* Utility::MakeRandBool(Random& rand , int num , int numTrue , bool buf[])
	{
		bool* result = buf;
		for (int i = 0; i < num; ++i)
		{
			result[i] = (i < numTrue);
		}
		for (int i = 0; i < num; ++i)
		{
			for (int j = 0; j < num; ++j)
			{
				bool temp = result[i];
				int idx = rand.nextInt() % num;
				result[i] = result[idx];
				result[idx] = temp;
			}
		}
		return result;
	}


	uint8* Utility::MakeRandBool(Random& rand, int num, int numTrue, uint8 buf[])
	{
		uint8* result = buf;
		for( int i = 0; i < num; ++i )
		{
			result[i] = (i < numTrue) ? 1 : 0;
		}
		for( int i = 0; i < num; ++i )
		{
			for( int j = 0; j < num; ++j )
			{
				uint8 temp = result[i];
				int idx = rand.nextInt() % num;
				result[i] = result[idx];
				result[idx] = temp;
			}
		}
		return result;
	}

	WorldCondition::WorldCondition()
	{
		walls[0].name = WallName::Bed;
	}

	int WorldCondition::getObjectNum(ObjectId id)
	{
		for( int i = 0 ; i < (int)ObjectId::NumCondObject ; ++i )
		{
			if (objects[i].id == id)
				return objects[i].num;
		}
		return 0;
	}

	int WorldCondition::getTopFireLightingNum()
	{
		int result = 0;
		for( int i = 0 ; i < 4 ; ++i )
		{
			if (bTopFireLighting[i])
				++result;
		}
		return result;
	}

	int WorldCondition::getRelDirIndex(int index , CondDir dir , bool bFaceWall)
	{
		assert(0 <= (int)dir && (int)dir < 4);
		int result = index + (int)dir;
		if ( bFaceWall == false )
			result += 2;
		return result % 4;
	}

	int WorldCondition::getRelDirInvIndex(int index, CondDir dir, bool bFaceWall)
	{
		assert(0 <= (int)dir && (int)dir < 4);
		int result = index;
		if( bFaceWall == false )
			result += 2;
		return ( result - (int)dir + 4 ) % 4;
	}

	bool WorldCondition::isTopFireLighting(WallName nearWallName , CondDir dir , bool bFaceWall)
	{
		int idx = getWallIndex(nearWallName);
		return bTopFireLighting[ getRelDirIndex( idx , dir , bFaceWall ) ];
	}

	bool WorldCondition::isTopFireLighting(int idx)
	{
		return bTopFireLighting[idx];
	}

	ColorId WorldCondition::getObjectColor(ObjectId id)
	{
		for (int i = 0; i < (int)ObjectId::NumCondObject; ++i)
		{
			if (objects[i].id == id)
				return objects[i].color;
		}
		return ColorId::White;
	}

	bool WorldCondition::checkWallCond(WallName a , WallName b , CondDir dir)
	{
		int idxA = getWallIndex(a);
		int idxB = getWallIndex(b);
		return getRelDirWallIndex(idxA, dir) == idxB;
	}

	int WorldCondition::getWallIndex(WallName name)
	{
		for(int i=0;i<4;++i)
		{
			if (walls[i].name == name)
				return i;
		}
		return -1;
	}

	WallName WorldCondition::getRelDirWall(WallName name , CondDir dir)
	{
		int idx = getWallIndex(name);
		idx = getRelDirWallIndex(idx, dir);
		return walls[idx].name;
	}

	void WorldCondition::generate(Random& rand)
	{
		int wallId[3];
		Utility::MakeRandSeq(rand, 3, 1, wallId);
		for (int i = 0; i < 4; ++i)
		{
			if (i == 0)
			{
				walls[i].name = WallName::Bed;
			}
			else
			{
				walls[i].name = (WallName)wallId[i - 1];
			}
			walls[i].color = (ColorId)(rand.nextInt() % (int)ColorId::Num);
		}

		for (int i = 0; i < (int)ObjectId::NumCondObject; ++i)
		{
			addObject((ObjectId)i, 1 + rand.nextInt() % (MaxCondObjectNum - 1), ColorId::White);
		}

		addObject(ObjectId::MugCube, 1, ColorId::White);
		addObject(ObjectId::Lantern, 1, ColorId::White);
		addObject(ObjectId::Door, 1, (ColorId)(rand.nextInt() % (int)ColorId::Num));
		addObject(ObjectId::MagicLight, 1, (ColorId)(rand.nextInt() % (int)ColorId::Num));

		indexWallHaveLight = rand.nextInt() % 4;
		int valuePropReq = rand.nextInt() % (int)ValueProperty::NumProp;

		valueForNumberWall = Utility::RandomValueForProperty( rand , (ValueProperty)valuePropReq );
		valuePropertyFlags = Utility::ValuePropertyFlags(valueForNumberWall);

		for (int i = 0; i < 4; ++i)
		{
			bTopFireLighting[i] = rand.nextInt() % 2 == 1;
		}
		bTopFireLighting[ rand.nextInt() % 4 ] = true;
	}

	static CondDir WallDirCond_dirMap[2] = { CondDir::Left , CondDir::Right };

	void WallDirCondExpression::generate(Random& rand)
	{
		mIdxContent = rand.nextInt() % 2;

		int wallId[(int)WallName::Num];
		Utility::MakeRandSeq(rand, (int)WallName::Num, 0, wallId );

		if (mIdxContent == 0 )
		{
			mElements.resize(3);
			mElements[0].set( WallName(wallId[0]) );
			mElements[1].set( WallName(wallId[1]) );
			mElements[2].set( WallDirCond_dirMap[ rand.nextInt() % 2 ] );
		}
		else
		{
			mElements.resize(5);
			mElements[0].set( WallName(wallId[0]) );
			mElements[1].set( WallName(wallId[1]) );
			mElements[2].set( WallName(wallId[2]) );
			mElements[3].set( WallName(wallId[3]) );
			mElements[4].set( WallDirCond_dirMap[rand.nextInt() % 2] );
		}
	}

	void WallDirCondExpression::generate(Random& rand , WorldCondition& worldCond)
	{
		mIdxContent = rand.nextInt() % 2;
		if (mIdxContent == 0)
		{
			mElements.resize(3);
			mElements[0].set(WallName(rand.nextInt() % 4) );
			mElements[2].set(WallDirCond_dirMap[rand.nextInt() % 2]);
			mElements[1].set( worldCond.getRelDirWall( mElements[0] , mElements[2] ) );
		}
		else
		{
			mElements.resize(5);
			int idx = rand.nextInt() % 4;
			CondDir dir = WallDirCond_dirMap[rand.nextInt() % 2];

			mElements[4].set(dir);
			for( int i = 0 ; i < 4 ; ++i )
			{
				mElements[i].set( worldCond.getWallName(idx) );
				idx = worldCond.getRelDirWallIndex(idx, dir);
			}
		}
	}

	bool WallDirCondExpression::test(WorldCondition& worldCond)
	{
		if (mIdxContent == 0)
		{
			return worldCond.checkWallCond( mElements[0] , mElements[1] , mElements[2] );
		}
		else
		{
			CondDir dir = mElements[4];
			int curIdx = worldCond.getWallIndex(mElements[0]);
			for (int i = 1; i < 4; ++i)
			{
				int nextIdx = worldCond.getRelDirWallIndex( curIdx , dir );
				if ( nextIdx != worldCond.getWallIndex( mElements[i]) )
					return false;
				curIdx = nextIdx;
			}
			return true;
		}
	}

	String WallDirCondExpression::getContent()
	{
		if (mIdxContent==0)
		{
			return toString(0) + "在" + toString(1) + "的" + toString(2) + "邊";
		}

		return toString(0) + "開始往" + toString(4) + "順序是" + 
			   toString(1) + "、" + toString(2) + "、" + toString(3);
	}

	static CondDir TopLightCond_dirMap[] = { CondDir::Front, CondDir::Back, CondDir::Left, CondDir::Right };



	void TopLightCondExpression::generate(Random& rand)
	{
		mIdxContent = rand.nextInt() % 2;

		if (mIdxContent == 0)
		{
			mElements.resize(3);
			mElements[0].setFaceFront( rand.nextInt() % 2 );
			mElements[1].set( WallName( rand.nextInt() % 4 ) );
			mElements[2].set( TopLightCond_dirMap[ rand.nextInt() % ARRAY_SIZE(TopLightCond_dirMap) ] );
		}
		else
		{
			mElements.resize(1);
			mElements[0].set( rand.nextInt() % 5 );
		}
	}

	void TopLightCondExpression::generate(Random& rand, WorldCondition& worldCond)
	{
		mIdxContent = rand.nextInt() % 2;

		if (mIdxContent == 0)
		{
			mElements.resize(3);
			mElements[0].setFaceFront(rand.nextInt() % 2);
			mElements[2].set( TopLightCond_dirMap[ rand.nextInt() % ARRAY_SIZE(TopLightCond_dirMap) ] );

			std::vector<int> idxLighting;
			for (int i = 0; i < 4; ++i )
			{
				if ( worldCond.isTopFireLighting(i) )
				{
					idxLighting.push_back(i);
				}
			}
			int idx = idxLighting[rand.nextInt() % idxLighting.size()];
			mElements[1].set( worldCond.getWallName( worldCond.getRelDirInvIndex( idx , mElements[2] , mElements[0].intValue != 0 ) ) );
			assert(worldCond.getRelDirIndex(worldCond.getWallIndex(mElements[1]), mElements[2], mElements[0].intValue != 0) == idx );
		}
		else
		{
			mElements.resize(1);
			mElements[0].set( worldCond.getTopFireLightingNum() );
		}
	}

	bool TopLightCondExpression::test(WorldCondition& worldCond)
	{
		if( mIdxContent == 0 )
		{
			return worldCond.isTopFireLighting( mElements[1], mElements[2], mElements[0].intValue != 0);
		}
		else
		{
			return mElements[0].intValue == worldCond.getTopFireLightingNum();
		}
	}

	String TopLightCondExpression::getContent()
	{
		if( mIdxContent == 0 )
		{
			return toString(0) + toString(1) + "向上看，" + toString(2) + "的燈是亮的";
		}
		else
		{
			return "天花板有" + toString(0) + "個燈亮";
		}
	}


	void WallColorCondExpression::generate(Random& rand)
	{
		//IdxContent = rand.Next() % 2;
		mIdxContent = 0;
		if (mIdxContent == 0)
		{
			mElements.resize(3);
			mElements[0].setFaceFront(rand.nextInt() % 2);
			mElements[1].set( WallName( rand.nextInt() % 4 ) );
			mElements[2].set( ColorId( rand.nextInt() % (int)ColorId::Num ) );
		}
		else
		{
			mElements.resize(5);
			mElements[0].setFaceFront(rand.nextInt() % 2);
			mElements[1].set(WallName(rand.nextInt() % 4));
			mElements[2].set(ColorId(rand.nextInt() % (int)ColorId::Num));
			if( (rand.nextInt() % 2) == 0 )
				mElements[3].set(CondDir::Left);
			else
				mElements[3].set(CondDir::Right);

			mElements[4].set( 1 + (rand.nextInt() % 3) );
		}
	}

	void WallColorCondExpression::generate(Random& rand, WorldCondition& worldCond)
	{
		//IdxContent = rand.Next() % 2;
		mIdxContent = 0;
		if (mIdxContent == 0)
		{
			mElements.resize(3);

			mElements[0].setFaceFront(rand.nextInt() % 2);
			mElements[1].set(WallName(rand.nextInt() % 4));
			int idx = worldCond.getWallIndex(mElements[1]);
			idx = worldCond.getRelDirIndex(idx, CondDir::Front, mElements[0].intValue != 0);
			mElements[2].set( worldCond.getWallColor(idx) );
		}
		else
		{
			mElements.resize(3);
			mElements[0].setFaceFront(rand.nextInt() % 2);
			mElements[1].set( WallName( rand.nextInt() % 4 ) );

			int idx = worldCond.getWallIndex(mElements[1]);
			idx = worldCond.getRelDirIndex(idx, CondDir::Front, mElements[0].intValue != 0);

			if ( (rand.nextInt() % 2 ) == 0 )
				mElements[3].set(CondDir::Left);
			else
				mElements[3].set(CondDir::Right);

			mElements[4].set( 1 + (rand.nextInt() % 3) );
			mElements[2].set( worldCond.getWallColor(idx) );
		}
	}

	bool WallColorCondExpression::test(WorldCondition& worldCond)
	{
		if (mIdxContent == 0)
		{
			int idx = worldCond.getWallIndex(mElements[1]);
			if (mElements[0].intValue == 0)
			{
				idx = (idx + 2) % 4;
			}
			return worldCond.getWallColor(idx) == mElements[2];
		}

		return false;
	}

	String WallColorCondExpression::getContent()
	{
		if (mIdxContent == 0)
		{
			return toString( 0 ) + toString( 1 ) + "那面牆，顏色是" + toString( 2 );
		}

		return toString( 0 ) + toString( 1 ) + "向" + toString( 3 ) + "的第" + toString( 4 ) + "面牆的顏色是" + toString( 2 );
	}


	void ObjectNumCondExpression::generate(Random& rand)
	{
		mElements.resize(2);
		mElements[0].set( ObjectId( rand.nextInt() % (int)ObjectId::NumCondObject ) );
		mElements[1].set( rand.nextInt() % 10 );
	}

	void ObjectNumCondExpression::generate(Random& rand, WorldCondition& worldCond)
	{
		mElements.resize(2);
		mElements[0].set(ObjectId(rand.nextInt() % (int)ObjectId::NumCondObject));
		mElements[1].set( worldCond.getObjectNum(mElements[0]) );
	}

	bool ObjectNumCondExpression::test(WorldCondition& worldCond)
	{
		return (int)mElements[1] == worldCond.getObjectNum(mElements[0]);
	}

	String ObjectNumCondExpression::getContent()
	{
		return "整個場景有" + toString( 1 ) + "個" + toString( 0 );
	}

	ObjectId ObjectColorCond_objectMap[] = { ObjectId::Door, ObjectId::MagicLight };


	void ObjectColorCondExpression::generate(Random& rand)
	{
		mElements.resize(2);
		mElements[0].set( rand.nextInt() % ARRAY_SIZE(ObjectColorCond_objectMap) );
		mElements[1].set( ColorId( rand.nextInt() % (int)ColorId::Num ) );
	}

	void ObjectColorCondExpression::generate(Random& rand, WorldCondition& worldCond)
	{
		mElements.resize(2);
		mElements[0].set( rand.nextInt() % ARRAY_SIZE(ObjectColorCond_objectMap) );
		mElements[1].set( worldCond.getObjectColor(ObjectColorCond_objectMap[ (int)mElements[0] ]) );
	}

	bool ObjectColorCondExpression::test(WorldCondition& worldCond)
	{
		return worldCond.getObjectColor(ObjectColorCond_objectMap[(int)mElements[0]]) == mElements[1];
	}

	String ObjectColorCondExpression::getContent()
	{
		if ( (int)mElements[0] == 0)
		{
			return "門的顏色是" + toString(1);
		}
		else
		{
			return "魔法陣的蠟燭火焰顏色是" + toString(1);	
		}
	}


	void WallNumberValueCondExpression::generate(Random& rand)
	{
		mElements.resize(1);
		mElements[0].set( rand.nextInt() % (int)ValueProperty::NumProp );
	}

	void WallNumberValueCondExpression::generate(Random& rand, WorldCondition& worldCond)
	{
		std::vector<int> props;
		for (int i = 0; i < (int)ValueProperty::NumProp; ++i )
		{
			if ( ( (1 << i) & worldCond.valuePropertyFlags ) != 0 )
				props.push_back(i);
		}
		mElements.resize(1);
		mElements[0].set( props[ rand.nextInt() % props.size() ] );
	}

	bool WallNumberValueCondExpression::test(WorldCondition& worldCond)
	{
		return !!( ( 1 << (int)mElements[0] ) & worldCond.valuePropertyFlags );
	}

	String WallNumberValueCondExpression::getContent()
	{
		switch( (int)mElements[0] )
		{
		case ValueProperty::PrimeNumber: return "在牆上的數字是質數";
		case ValueProperty::DividedBy4: return "在牆上的數字是四的倍數";
		case ValueProperty::DSumIsBiggerThan10: return "在牆上的數字，十位數字加個位數字超過10";
		case ValueProperty::DOIsBiggerThanDT: return "在牆上的數字，個位數字比十位數字大";
		}
		return "Error Content";
	}


	Condition::~Condition()
	{
		cleanup();
	}

	void Condition::cleanup()
	{
		for(int i = 0 ; i < mExprList.size() ; ++i )
		{
			delete mExprList[i];
		}
		mExprList.clear();
	}

	static const int TotalExprNum = 6;
	CondExpression* CreateExpression(int idx)
	{
		switch( idx )
		{
		case 0: return new WallDirCondExpression();
		case 1: return new WallColorCondExpression();
		case 2: return new ObjectNumCondExpression();
		case 3: return new ObjectColorCondExpression();
		case 4: return new TopLightCondExpression();
		case 5: return new WallNumberValueCondExpression();
		}
		assert(0);
		return nullptr;
	}

	void GenerateRandExprssion(Random& rand, int numExpr , CondExpression* outExpr[] )
	{
		assert(numExpr <= TotalExprNum);
		bool bufUseExprMap[TotalExprNum];
		bool* useExprMap = Utility::MakeRandBool(rand, TotalExprNum, numExpr, bufUseExprMap);

		int idx = 0;
		for( int i = 0; i < TotalExprNum; ++i )
		{
			if( useExprMap[i] == false )
				continue;

			outExpr[idx] = CreateExpression(i);
			assert(outExpr[idx]);
			++idx;
		}
	}

	void Condition::generate(Random& rand, WorldCondition& worldCond, int numExpr)
	{
		cleanup();
		mExprList.resize(numExpr);
		GenerateRandExprssion(rand, numExpr, &mExprList[0]);
		for( int i = 0 ; i < numExpr; ++i )
		{
			CondExpression* expr = mExprList[i];
			expr->generate(rand, worldCond);
			assert(expr->test(worldCond));
		}
		bValid = true;
	}

	void Condition::generateRandom(Random& rand, WorldCondition& worldCond, int numExpr, int numIn)
	{
		assert(numExpr <= TotalExprNum);
		cleanup();
		mExprList.resize(numExpr);
		GenerateRandExprssion(rand, numExpr, &mExprList[0]);

		TArrayHolder< bool > bufInMap(new bool[numExpr]);
		bool* inMap = Utility::MakeRandBool(rand, numExpr, numIn, bufInMap.get());
		for( int i = 0; i < numExpr; ++i )
		{
			CondExpression* expr = mExprList[i];
			if( inMap[i] )
			{
				do
				{
					expr->generate(rand);
				} 
				while( expr->test(worldCond) );
			}
			else
			{
				expr->generate(rand, worldCond);
				assert(expr->test(worldCond));
			}
		}

		bValid = false;
	}


	void ConditionTable::generate(Random& rand , WorldCondition& worldCond , int numSel , int numExpr , int num)
	{
		numSelection = numSel;
		numCondition = num;

		conditions.resize( numSelection );

		int const MaxObjectId = 6;
		int bufObjectIdMap[MaxObjectId];
		int* objectIdMap = Utility::MakeRandSeq( rand , MaxObjectId , 0 , bufObjectIdMap );
		TArrayHolder< bool > bufMap( new bool[ numSelection ] );
		bool* Map = Utility::MakeRandBool( rand , numSelection , numCondition , bufMap.get() );
		for( int i = 0 ; i < numSelection ; ++i )
		{
			if ( Map[i] )
			{
				conditions[i].generate( rand , worldCond , numExpr );
			}
			else
			{
				int numIn = 1;
				int additionIn = numExpr - 1;
				if (additionIn > 0)
					numIn += rand.nextInt() % additionIn;
				conditions[i].generateRandom(rand, worldCond, numExpr, numIn );
			}

			conditions[i].targetId = (ObjectId)objectIdMap[i];
		}
	}

	bool ConditionTable::isObject(ObjectId id)
	{
		for( int i = 0 ; i < numSelection ; ++i )
		{
			if (conditions[i].targetId == id )
			{
				return conditions[i].bValid;
			}
		}
		return false;
	}

	void ConditionTable::cleanup()
	{
		conditions.clear();
		numCondition = 0;
		numSelection = 0;
	}


	String CondExprElement::toString(ColorId id)
	{
		switch( id )
		{
		case ColorId::Red: return "紅";
		case ColorId::White: return "白";
		case ColorId::Yellow: return "黃";
		case ColorId::Green: return "綠";
		}
		return "Error Color";
	}

	String CondExprElement::toString(ObjectId id)
	{
		switch (id)
		{
		case ObjectId::Ball: return "球";
		case ObjectId::Candlestick: return "燭台";
		case ObjectId::Book: return "書";
		case ObjectId::Doll: return "兔娃娃";
		case ObjectId::Lantern: return "燈籠";
		case ObjectId::MugCube: return "方塊";
		}
		return "Error Object";
	}

	String CondExprElement::toString(int value)
	{
		InlineString<128> temp;
		temp.format("%d" , value);
		return temp.c_str();
	}

	String CondExprElement::toString(WallName name)
	{
		switch(name)
		{
		case WallName::Bed: return "床";
		case WallName::Door: return "門";
		case WallName::Clock: return "時鐘";
		case WallName::Number: return "數字牆";
		}
		return "Error Wall Name";
	}

	String CondExprElement::toString(CondDir dir)
	{
		switch( dir )
		{
		case CondDir::Top: return "上";
		case CondDir::Left: return "左";
		case CondDir::Right: return "右";
		case CondDir::Bottom: return "下";
		case CondDir::Front: return "前";
		case CondDir::Back: return "後";
		}

		return "Error dir";
	}


	String CondExprElement::toString()
	{
		switch( type )
		{
		case CondExprElement::eDir: return toString(dir);
		case CondExprElement::eWallName: return toString(wall);
		case CondExprElement::eColor: return toString(color);
		case CondExprElement::eObject: return toString(obj);
		case CondExprElement::eIntValue: return toString(intValue);
		case CondExprElement::eFaceFront:
			if( intValue == 0 )
				return "背對";
			return "面向";
		}

		return "Error Cond Element";
	}

	bool TestStage::onInit()
	{
		//if ( !::Global::GetDrawEngine().startOpenGL() )
		//return false;
		::srand(2);

		::Global::GUI().cleanupWidget();
		restart();
		return true;
	}

	void TestStage::restart()
	{
		worldCond.generate( rand );
		condTable.generate( rand , worldCond ,  4 , 6 , 1 );
	}

	void TestStage::onRender(float dFrame)
	{
		Graphics2D& g = Global::GetGraphics2D();

		Vec2i pos( 50 , 10 );
		for(int i = 0 ; i < condTable.numSelection ; ++i )
		{
			Condition& cond = condTable.getCondition(i);
			InlineString<128> str;
			str.format("Cond %d %s" , i , ( cond.bValid ) ? "True" : "False" );
			g.drawText( pos , str.c_str() );
			pos.y += 15;

			for( int n = 0 ; n < cond.getExpressionNum() ; ++n )
			{
				String str = cond.getExpression( n )->getContent();
				str += "(";
				str += cond.getExpression( n )->test( worldCond ) ? "True" : "False";
				str += ")";
				g.drawText( pos , str.c_str() );
				pos.y += 15;
			}
		}
	}

}//namespace GGJ


REGISTER_STAGE("GGJ Test", GGJ::TestStage, EStageGroup::GraphicsTest);