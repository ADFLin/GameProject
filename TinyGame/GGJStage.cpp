#include "GGJStage.h"

namespace GGJ
{

	int Utility::getRandomValueForProperty(Random& rand , ValueProperty prop)
	{
		int result= 1;
		switch (prop)
		{
		case ValueProperty::PrimeNumber:
			do
			{
				result = 1 + rand.nextInt() % 99;
			}
			while (Utility::IsPrime(result));
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

	int Utility::getValuePropertyFlag(int value)
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
		for( int i = 2 ; i < value ; ++i )
		{
			if ((value % i) == 0)
				return false;
		}
		return true;
	}

	int* Utility::makeRandSeq(Random& rand, int num, int start, int buf[])
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

	bool* Utility::makeRandBool(Random& rand , int num , int numTrue , bool buf[])
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
		int result = index + (int)dir;
		if ( bFaceWall == false )
			result += 2;
		return result % 4;
	}

	bool WorldCondition::isTopFireLighting(WallName::Enum nearWallName , CondDir dir , bool bFaceWall)
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

	bool WorldCondition::checkVaild(WallName::Enum a , WallName::Enum b , CondDir dir)
	{
		int idxA = getWallIndex(a);
		int idxB = getWallIndex(b);
		return getRelDirWallIndex(idxA, dir) == idxB;
	}

	int WorldCondition::getWallIndex(WallName::Enum name)
	{
		for(int i=0;i<4;++i)
		{
			if (walls[i].name == name)
				return i;
		}
		return -1;
	}

	WallName::Enum WorldCondition::getRelDirWall(WallName::Enum name , CondDir dir)
	{
		int idx = getWallIndex(name);
		idx = getRelDirWallIndex(idx, dir);
		return walls[idx].name;
	}

	void WorldCondition::generate(Random& rand)
	{
		int bufWallId[3];
		int* wallId = Utility::makeRandSeq(rand, 3, 1, bufWallId);
		for (int i = 0; i < 4; ++i)
		{
			if (i == 0)
			{
				walls[i].name = WallName::Bed;
			}
			else
			{
				walls[i].name = (WallName::Enum)wallId[i - 1];
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

		valueForNumberWall = Utility::getRandomValueForProperty( rand , (ValueProperty)valuePropReq);
		valuePropertyFlag = Utility::getValuePropertyFlag(valueForNumberWall);

		for (int i = 0; i < 4; ++i)
		{
			bTopFireLighting[i] = rand.nextInt() % 2 == 1;
		}
		bTopFireLighting[ rand.nextInt() % 4 ] = true;
	}

	static CondDir WallDirCond_dirMap[2] = { CondDir::Left , CondDir::Right };

	bool WallDirCondExpression::testVaild(WorldCondition& worldCond)
	{
		if (IdxContent == 0)
		{
			return worldCond.checkVaild( elements[0].wall , elements[1].wall , elements[2].dir );
		}
		else
		{
			CondDir dir = (CondDir)elements[4].meta;
			int curIdx = worldCond.getWallIndex(elements[0].wall);
			for (int i = 1; i < 4; ++i)
			{
				int nextIdx = worldCond.getRelDirWallIndex( curIdx , dir );
				if (nextIdx != worldCond.getWallIndex(elements[0].wall))
					return false;
			}
			return true;
		}
	}

	void WallDirCondExpression::generate(Random& rand)
	{
		IdxContent = rand.nextInt() % 2;

		int bufWallId[ WallName::Num ];
		int* walllId = Utility::makeRandSeq( rand , WallName::Num , 0 , bufWallId );

		if (IdxContent == 0 )
		{
			elements.resize(3);
			elements[0].setWall( WallName::Enum(walllId[0]) );
			elements[1].setWall( WallName::Enum(walllId[1]) );

			elements[2].type = CondExprElement::eDir;
			elements[2].meta = (int)WallDirCond_dirMap[ rand.nextInt() % 2 ];
		}
		else
		{
			elements.resize(5);
			elements[0].setWall( WallName::Enum(walllId[0]) );
			elements[1].setWall( WallName::Enum(walllId[1]) );
			elements[2].setWall( WallName::Enum(walllId[2]) );
			elements[3].setWall( WallName::Enum(walllId[3]) );

			elements[4].type = CondExprElement::eDir;
			elements[4].meta = (int)WallDirCond_dirMap[rand.nextInt() % 2];
		}
	}

	void WallDirCondExpression::generateVaild(Random& rand , WorldCondition& worldCond)
	{
		IdxContent = rand.nextInt() % 2;
		if (IdxContent == 0)
		{
			elements.resize(3);
			elements[0].type = CondExprElement::eWallName;
			elements[0].meta = rand.nextInt()%4;

			elements[2].type = CondExprElement::eDir;
			elements[2].meta = (int)WallDirCond_dirMap[rand.nextInt() % 2];

			elements[1].type = CondExprElement::eWallName;
			elements[1].meta = (int)worldCond.getRelDirWall( (WallName::Enum)elements[0].meta , (CondDir)elements[2].meta );
		}
		else
		{
			elements.resize(5);
			int idx = rand.nextInt() % 4;
			CondDir dir = WallDirCond_dirMap[rand.nextInt() % 2];

			elements[4].type = CondExprElement::eDir;
			elements[4].meta = (int)dir;

			for( int i = 0 ; i < 4 ; ++i )
			{
				elements[i].type = CondExprElement::eWallName;
				elements[i].meta = (int)worldCond.walls[idx].name;
				idx = worldCond.getRelDirWallIndex(idx, dir);
			}
		}
	}

	String WallDirCondExpression::getContent()
	{
		if (IdxContent==0)
		{
			return toString(0) + "在" + toString(1) + "的" + toString(2) + "邊";
		}

		return toString(0) + "開始往" + toString(4) + "順序是"
			+ toString(1) + "、"
			+ toString(2) + "、"
			+ toString(3);
	}

	static CondDir TopLightCond_dirMap[] = { CondDir::Front, CondDir::Back, CondDir::Left, CondDir::Right };

	bool TopLightCondExpression::testVaild(WorldCondition& worldCond)
	{
		if (IdxContent == 0)
		{
			return worldCond.isTopFireLighting((WallName::Enum)elements[1].meta, (CondDir)elements[0].meta, elements[2].meta != 0);
		}
		else 
		{
			return elements[0].meta == worldCond.getTopFireLightingNum();
		}
	}

	void TopLightCondExpression::generate(Random& rand)
	{
		IdxContent = rand.nextInt() % 2;

		if (IdxContent == 0)
		{
			elements.resize(3);
			elements[0].type = CondExprElement::eFaceFront;
			elements[0].meta = rand.nextInt() % 2;

			elements[1].type = CondExprElement::eWallName;
			elements[1].meta = rand.nextInt() % 4;

			elements[2].type = CondExprElement::eDir;
			elements[2].meta = (int)TopLightCond_dirMap[ rand.nextInt() % ARRAY_SIZE(TopLightCond_dirMap) ];
		}
		else
		{
			elements.resize(1);
			elements[0].type = CondExprElement::eIntValue;
			elements[0].meta = rand.nextInt() % 5;
		}
	}

	void TopLightCondExpression::generateVaild(Random& rand, WorldCondition& worldCond)
	{
		IdxContent = rand.nextInt() % 2;

		if (IdxContent == 0)
		{
			elements.resize(3);
			elements[0].type = CondExprElement::eFaceFront;
			elements[0].meta = rand.nextInt() % 2;

			elements[2].type = CondExprElement::eDir;
			elements[2].meta = (int)TopLightCond_dirMap[ rand.nextInt() % ARRAY_SIZE(TopLightCond_dirMap) ];

			std::vector<int> idxLighting;
			for (int i = 0; i < 4; ++i )
			{
				if ( worldCond.isTopFireLighting(i) )
				{
					idxLighting.push_back(i);
				}
			}
			int idx = idxLighting[rand.nextInt() % idxLighting.size()];
			if (idx == 0)
				idx += 2;
			elements[1].type = CondExprElement::eWallName;
			elements[1].meta = (int)worldCond.walls[ ( idx - elements[2].meta + 4 ) % 4 ].name;

		}
		else
		{
			elements.resize(1);
			elements[0].type = CondExprElement::eIntValue;
			elements[0].meta = worldCond.getTopFireLightingNum();
		}
	}

	String TopLightCondExpression::getContent()
	{
		if (IdxContent == 0)
		{
			return toString( 0 ) + toString( 1 ) + "向上看，燈亮的是" + toString( 2 );
		}
		else
		{
			return "天花板有" + toString( 0 ) + "個燈亮";
		}
	}


	bool WallColorCondExpression::testVaild(WorldCondition& worldCond)
	{
		if (IdxContent == 0)
		{
			int idx = worldCond.getWallIndex((WallName::Enum)elements[1].meta);
			if (elements[0].meta == 0)
			{
				idx = (idx + 2) % 4;
			}
			return elements[2].meta == (int)worldCond.walls[idx].color;
		}

		return false;
	}

	void WallColorCondExpression::generate(Random& rand)
	{
		//IdxContent = rand.Next() % 2;
		IdxContent = 0;
		if (IdxContent == 0)
		{
			elements.resize(3);
			elements[0].type = CondExprElement::eFaceFront;
			elements[0].meta = rand.nextInt() % 2;

			elements[1].type = CondExprElement::eWallName;
			elements[1].meta = rand.nextInt() % 4;

			elements[2].type = CondExprElement::eColor;
			elements[2].meta = rand.nextInt() % (int)ColorId::Num;
		}
		else
		{
			elements.resize(5);
			elements[0].type = CondExprElement::eFaceFront;
			elements[0].meta = rand.nextInt() % 2;

			elements[1].type = CondExprElement::eWallName;
			elements[1].meta = rand.nextInt() % 4;

			elements[2].type = CondExprElement::eColor;
			elements[2].meta = rand.nextInt() % (int)ColorId::Num;

			elements[3].type = CondExprElement::eDir;
			if ( (rand.nextInt() % 2 ) == 0 )
				elements[3].meta = (int)CondDir::Left;
			else
				elements[3].meta = (int)CondDir::Right;

			elements[4].type = CondExprElement::eIntValue;
			elements[4].meta = 1 + (rand.nextInt() % 3);
		}
	}

	void WallColorCondExpression::generateVaild(Random& rand, WorldCondition& worldCond)
	{
		//IdxContent = rand.Next() % 2;
		IdxContent = 0;
		if (IdxContent == 0)
		{
			elements.resize(3);
			elements[0].type = CondExprElement::eFaceFront;
			elements[0].meta = rand.nextInt() % 2;

			elements[1].type = CondExprElement::eWallName;
			elements[1].meta = rand.nextInt() % 4;

			int idx = worldCond.getWallIndex((WallName::Enum)elements[1].meta);
			idx = worldCond.getRelDirIndex(idx, CondDir::Front, elements[0].meta != 0);
			elements[2].type = CondExprElement::eColor;
			elements[2].meta = (int)worldCond.walls[idx].color;
		}
		else
		{
			elements.resize(3);
			elements[0].type = CondExprElement::eFaceFront;
			elements[0].meta = rand.nextInt() % 2;

			elements[1].type = CondExprElement::eWallName;
			elements[1].meta = rand.nextInt() % 4;


			int idx = worldCond.getWallIndex((WallName::Enum)elements[1].meta);
			idx = worldCond.getRelDirIndex(idx, CondDir::Front, elements[0].meta != 0);

			elements[3].type = CondExprElement::eDir;
			if ( (rand.nextInt() % 2 ) == 0 )
				elements[3].meta = (int)CondDir::Left;
			else
				elements[3].meta = (int)CondDir::Right;

			elements[4].type = CondExprElement::eIntValue;
			elements[4].meta = 1 + (rand.nextInt() % 3);

			elements[2].type = CondExprElement::eColor;
			elements[2].meta = (int)worldCond.walls[idx].color;
		}
	}

	String WallColorCondExpression::getContent()
	{
		if (IdxContent == 0)
		{
			return toString( 0 ) + toString( 1 ) + "那面牆，顏色是" + toString( 2 );
		}

		return toString( 0 ) + toString( 1 ) + "向" + toString( 3 ) + "的第" + toString( 4 ) + toString( 2 );
	}


	bool ObjectNumCondExpression::testVaild(WorldCondition& worldCond)
	{
		return elements[1].meta == worldCond.getObjectNum((ObjectId)elements[0].meta);
	}

	void ObjectNumCondExpression::generate(Random& rand)
	{
		elements.resize(2);
		elements[0].type = CondExprElement::eObject;
		elements[0].meta = rand.nextInt() % (int)ObjectId::NumCondObject;

		elements[1].type = CondExprElement::eIntValue;
		elements[1].meta = rand.nextInt() % 10;
	}

	void ObjectNumCondExpression::generateVaild(Random& rand, WorldCondition& worldCond)
	{
		elements.resize(2);
		elements[0].type = CondExprElement::eObject;
		elements[0].meta = rand.nextInt() % (int)ObjectId::NumCondObject;

		elements[1].type = CondExprElement::eIntValue;
		elements[1].meta = worldCond.getObjectNum((ObjectId)elements[0].meta);
	}

	String ObjectNumCondExpression::getContent()
	{
		return "整個場景有" + toString( 1 ) + "個" + toString( 0 );
	}

	ObjectId ObjectColorCond_objectMap[] = { ObjectId::Door, ObjectId::MagicLight };


	bool ObjectColorCondExpression::testVaild(WorldCondition& worldCond)
	{
		return elements[1].meta == (int)worldCond.getObjectColor(ObjectColorCond_objectMap[elements[0].meta]);
	}

	void ObjectColorCondExpression::generate(Random& rand)
	{
		elements.resize(2);
		elements[0].type = CondExprElement::eIntValue;
		elements[0].meta = rand.nextInt() % ARRAY_SIZE(ObjectColorCond_objectMap);

		elements[1].type = CondExprElement::eColor;
		elements[1].meta = rand.nextInt() % (int)ColorId::Num;
	}

	void ObjectColorCondExpression::generateVaild(Random& rand, WorldCondition& worldCond)
	{
		elements.resize(2);
		elements[0].type = CondExprElement::eIntValue;
		elements[0].meta = rand.nextInt() % ARRAY_SIZE(ObjectColorCond_objectMap);

		elements[1].type = CondExprElement::eColor;
		elements[1].meta = (int)worldCond.getObjectColor(ObjectColorCond_objectMap[elements[0].meta]);
	}

	String ObjectColorCondExpression::getContent()
	{
		if (elements[0].meta == 0)
		{
			return "魔法陣的蠟燭火焰顏色是" +  toString( 1 );
		}
		else
		{
			return "門的顏色是" + toString( 1 );
		}
	}


	bool WallNumberValueCondExpression::testVaild(WorldCondition& worldCond)
	{
		return ( ( 1 << elements[0].meta ) & worldCond.valuePropertyFlag ) != 0;
	}

	void WallNumberValueCondExpression::generate(Random& rand)
	{
		elements.resize(1);
		elements[0].type = CondExprElement::eIntValue;
		elements[0].meta = rand.nextInt() % (int)ValueProperty::NumProp;
	}

	void WallNumberValueCondExpression::generateVaild(Random& rand, WorldCondition& worldCond)
	{
		std::vector<int> props;
		for (int i = 0; i < (int)ValueProperty::NumProp; ++i )
		{
			if ( ( (1 << i) & worldCond.valuePropertyFlag ) != 0 )
				props.push_back(i);
		}
		elements.resize(1);
		elements[0].type = CondExprElement::eIntValue;
		elements[0].meta = props[ rand.nextInt() % props.size() ];
	}

	String WallNumberValueCondExpression::getContent()
	{
		switch( (ValueProperty)elements[0].meta )
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
		for(int i = 0 ; i < exprList.size() ; ++i )
		{
			delete exprList[i];
		}
		exprList.clear();
	}

	CondExpression* Condition::CreateExpression(int idx)
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
		return nullptr;
	}

	void Condition::generateVaild(Random& rand, WorldCondition& worldCond, int numExpr)
	{
		cleanup();

		int idx = 0;
		exprList.resize(numExpr);
		bool bufUseExprMap[ TotalExprNum ];
		bool* useExprMap = Utility::makeRandBool(rand, TotalExprNum, numExpr , bufUseExprMap ); 
		for( int i = 0 ; i < TotalExprNum ; ++i )
		{
			if (useExprMap[i] == false)
				continue;

			CondExpression* expr = CreateExpression(i);
			if (expr!=nullptr)
			{
				expr->generateVaild(rand, worldCond);
			}
			exprList[idx] = expr;
			++idx;
		}
		bVaild = true;
	}

	void Condition::generateRandom(Random& rand, WorldCondition& worldCond, int numExpr, int numInvaild)
	{
		bool bufInvaildMap[TotalExprNum];
		bool* invaildMap = Utility::makeRandBool(rand, TotalExprNum, numInvaild , bufInvaildMap );
		bool bufUseMap[TotalExprNum];
		bool* useExprMap = Utility::makeRandBool(rand, TotalExprNum, numExpr , bufUseMap );

		int idx = 0;
		exprList.resize( numExpr );
		for (int i = 0; i < TotalExprNum; ++i)
		{
			if (useExprMap[i] == false)
				continue;

			CondExpression* expr = CreateExpression(i);
			if (expr != nullptr)
			{
				if ( invaildMap[i] )
				{
					do
					{
						expr->generate(rand);
					}
					while (expr->testVaild(worldCond) == true);
				}
				else
				{
					expr->generateVaild( rand , worldCond );
				}
			}
			exprList[idx] = expr;
			++idx;
		}

		bVaild = false;
	}


	void ConditionTable::generate(Random& rand , WorldCondition& worldCond , int numSel , int numExpr , int numVaild)
	{
		numSelection = numSel;
		numConditionVaild = numVaild;

		conditions.resize( numSelection );

		int const MaxObjectId = 6;
		int bufObjectIdMap[MaxObjectId];
		int* objectIdMap = Utility::makeRandSeq( rand , MaxObjectId , 0 , bufObjectIdMap );
		TArrayHolder< bool > bufVaildMap( new bool[ numSelection ] );
		bool* vaildMap = Utility::makeRandBool( rand , numSelection , numConditionVaild , bufVaildMap.get() );
		for( int i = 0 ; i < numSelection ; ++i )
		{
			if ( vaildMap[i] )
			{
				conditions[i].generateVaild( rand , worldCond , numExpr );
			}
			else
			{
				int numInvaild = 1;
				int additionInvaild = numExpr - 1;
				if (additionInvaild > 0)
					numInvaild += rand.nextInt() % additionInvaild;
				conditions[i].generateRandom(rand, worldCond, numExpr, numInvaild );
			}

			conditions[i].targetId = (ObjectId)objectIdMap[i];
		}
	}

	bool ConditionTable::isVaildObject(ObjectId id)
	{
		for( int i = 0 ; i < numSelection ; ++i )
		{
			if (conditions[i].targetId == id )
			{
				return conditions[i].bVaild;
			}
		}
		return false;
	}

	void ConditionTable::cleanup()
	{
		conditions.clear();
		numConditionVaild = 0;
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
		FixString<128> temp;
		temp.format("%d" , value);
		return temp.c_str();
	}

	String CondExprElement::toString(CondExprElement ele)
	{
		switch( ele.type )
		{
		case CondExprElement::eDir: return toString( (CondDir)ele.meta );
		case CondExprElement::eWallName: return toString( (WallName::Enum)ele.meta );
		case CondExprElement::eColor: return toString((ColorId)ele.meta);
		case CondExprElement::eObject: return toString((ObjectId)ele.meta);
		case CondExprElement::eIntValue: return toString( ele.meta );
		case CondExprElement::eFaceFront:
			if (ele.meta == 0)
				return "背對";
			return "面向";
		}

		return "Error Cond Element";
	}

	String CondExprElement::toString(WallName::Enum name)
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


	bool TestStage::onInit()
	{
		//if ( !::Global::getDrawEngine()->startOpenGL() )
		//return false;
		::srand(2);

		::Global::GUI().cleanupWidget();
		restart();
		return true;
	}

	void TestStage::restart()
	{
		worldCond.generate( rand );
		condTable.generate( rand , worldCond ,  3 , 6 , 1 );
	}

	void TestStage::onRender(float dFrame)
	{
		Graphics2D& g = Global::getGraphics2D();

		Vec2i pos( 50 , 10 );
		for(int i = 0 ; i < condTable.numSelection ; ++i )
		{
			Condition& cond = condTable.getCondition(i);
			FixString<128> str;
			str.format("Cond %d %s" , i , ( cond.bVaild ) ? "True" : "False" );
			g.drawText( pos , str.c_str() );
			pos.y += 15;

			for( int n = 0 ; n < cond.getExprissionNum() ; ++n )
			{
				String str = cond.getContent( n );
				str += "(";
				str += cond.exprList[n]->testVaild( worldCond ) ? "True" : "False";
				str += ")";
				g.drawText( pos , str.c_str() );
				pos.y += 15;
			}
		}
	}

}//namespace GGJ