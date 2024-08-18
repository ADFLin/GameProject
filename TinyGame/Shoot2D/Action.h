#ifndef Action_H
#define Action_H

#include "CommonFwd.h"
#include <vector>

namespace Shoot2D
{
	class Action
	{
	public:
		int flag;
		virtual ~Action(){}
		virtual void update( Object& obj , long time )
		{

		}
		//void* operator new ( size_t size ){}
		//void  operator delete( void* ptr ){}
	};

	class SampleAI : public Action
	{
	public:
		SampleAI(Action* m = nullptr)
		{
			move = m;
		}

		~SampleAI()
		{
			delete move;
		}

		void update(Object& obj , long time );
		Action* move;
	};

	class Move : public Action
	{
	public:
		Move(float speed)
			:m_speed(speed)
		{
		}
		virtual void update(Object& obj , long time ) = 0;
		float getSpeed() const { return m_speed; }
	protected:
		float m_speed;
	};

	class CircleMove : public Move
	{
	public:
		CircleMove( Vec2D const& pos ,float r ,float speed , bool clockwise = true);

		virtual void update(Object& obj , long time );

	public:
		bool  isClockwise;
		float radius;
		Vec2D org;
	};


	class PathMove : public Action
	{
	public:
		enum
		{
			STOP  = 0,
			CYCLE ,
			GO_FRONT,
			GO_BACK ,
		};
		PathMove()
			:nextIndex(-1)
		{
			speed = 40;
		}

		void update(Object& obj , long time );
		void computeNextMove(Object& obj);
		void addPos(Vec2D const& pos)
		{  path.push_back(pos); }

		void setNextIndex();
		int   flag;  
		float moveTime;
		float speed;
		Vec2D moveSpeed;
		int   nextIndex;
		std::vector< Vec2D > path;
	};


	class Dying : public Action
	{
	public:
		Dying(long total)
		{
			totalTime = total;
			tickTime = 0;
		}

		void update(Object& obj , long time );

		long totalTime;
		long tickTime;
	};

	class Follow : public Action
	{
	public:
		Follow( float s , Object* obj )
			:speed(s)
			,destObj(obj)
		{ }

		void update( Object& obj , long time);
		float    speed;
		Object*  destObj;
	};


}//namespace Shoot2D

#endif Action_H