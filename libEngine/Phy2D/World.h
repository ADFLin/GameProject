#ifndef World_h__B608C3CE_6A5F_419B_ABFF_13BECEDB94CA
#define World_h__B608C3CE_6A5F_419B_ABFF_13BECEDB94CA

#include "Phy2D/Base.h"

#include "Phy2D/Collision.h"

#include "IntrList.h"
#include "FrameAllocator.h"

namespace Phy2D
{
	struct BodyInfo;

	class World
	{
	public:
		World()
			:mAllocator( 1024 )
		{
			mGrivaty = Vec2f(0,-9.8);
		}
		
		CORE_API CollideObject* createCollideObject( Shape* shape );
		CORE_API void           destroyCollideObject( CollideObject* obj );
		CORE_API RigidBody*     createRigidBody( Shape* shape , BodyInfo const& info );
		CORE_API void           destroyRigidBody( RigidBody* body );


		CORE_API void           simulate( float dt );

		CORE_API void           clearnup( bool beDelete );

		CollisionManager mColManager;

		typedef IntrList< 
			CollideObject , 
			MemberHook< CollideObject , &CollideObject::hook > , 
			PointerType
		>  ColObjectList;

		typedef IntrList< 
			RigidBody, 
			MemberHook< CollideObject, &CollideObject::hook > , 
			PointerType
		>  RigidBodyList;
		ColObjectList  mColObjects;
		RigidBodyList  mRigidBodies;
		Vec2f          mGrivaty;
		FrameAllocator mAllocator;

	};





}

#endif // World_h__B608C3CE_6A5F_419B_ABFF_13BECEDB94CA
