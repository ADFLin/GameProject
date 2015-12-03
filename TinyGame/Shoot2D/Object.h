#ifndef Object_H
#define Object_H

#include "Commonfwd.h"
#include <list>
#include <vector>

namespace Shoot2D
{
	bool isNeedUpdate(ObjStats stats);

	class Entity
	{
	public:
		virtual void onSpawn() = 0;
		virtual void onDestroy() = 0;
	private:
		friend class World;
	};

	class GameObject
	{
	public:
		virtual void preUpdate() = 0;
		virtual void postUpdate() = 0;

	};

	class Object
	{
	public:
		Object(ModelId id,Vec2D const& pos = Vec2D(0,0));

		void setAction(Action* paction);

		Vec2D const& getPos() const { return mPos; }
		void  setPos(Vec2D const& pos){ mPos = pos;  }
		void  shiftPos( Vec2D const& pos ){ mPos += pos; }
		Vec2D const& getVelocity() const { return mVel; }
		void  setVelocity(Vec2D val) 
		{ 
			mVel = val; 
			updateDir();
		}

		size_t   getColID() const { return mColID; }
		void     setColID(size_t val) { mColID = val; }

		ObjTeam  getTeam(){ return m_Team; }
		ModelId  getModelId() const { return mModelId; }
		ObjStats getStats(){ return m_Stats; }
		void     setTeam(ObjTeam team){ m_Team = team; }
		void     setStats( ObjStats s ){ m_Stats = s; }
		unsigned getColFlag() const { return colFlag; }
		void     setColFlag(unsigned val) { colFlag = val; }
		int      getDir(){ return curDir; }
		void     updateDir();
		int      getFrameCount() const { return frame; }

		virtual void update(long time);
		virtual void processCollision(Object& obj);

	protected:
		//base
		Vec2D mPos;
		Vec2D mVel;
		//manger
		ObjStats m_Stats;
		//collision System
		unsigned colFlag;
		size_t   mColID;


		ObjTeam  m_Team;
		int m_HP;

		int curDir;
		int nextDir;
		int frame;

		ModelId mModelId;
		Action* m_Action;
	};

	ObjModel const& getModel( Object& obj );
	Vec2D getCenterPos( Object& obj );
	void  setCenterPos( Object& obj , Vec2D const& pos);


	class ObjectGroup : public Object
	{
	public:
		void update(long time)
		{

		}
		std::vector< Object* > objList;
	};

	class Vehicle : public Object
	{
	public:
		Vehicle(ModelId id,Vec2D const& pos);
		~Vehicle();

		Weapon* weapon;
		Weapon* getWeapon() const { return weapon;}
		void setWeapon(Weapon* w);
		void update(long time);
		bool tryFire();

	};

	class Bullet : public Object
	{
	public:
		Bullet( ModelId id , Object* owner , Vec2D& difPos = Vec2D(0,0) );
	};

	class Ghost :  public Object
	{
	public:
		Ghost( ModelId id , Vec2D& pos);
	};

}//namespace Shoot2D

#endif //Object_H