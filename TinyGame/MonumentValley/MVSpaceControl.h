#ifndef MVSpaceControl_h__
#define MVSpaceControl_h__

#include "MVCommon.h"
#include "MVObject.h"

#include "IntrList.h"

#include <vector>

namespace MV
{
	typedef TIntrList< 
		ObjectGroup , 
		MemberHook< ObjectGroup , &ObjectGroup::spaceHook > , 
		PointerType 
	> ObjectGroupList;

	enum ModifyType
	{
		SNT_ROTATOR = 0,
		SNT_PARALLAX_ROTATOR = 1,
		SNT_TRANSlATOR = 2 ,
	};

	class  ISpaceModifier
	{
	public:
		ISpaceModifier()
		{
			isUse = false;
			bModifyChildren = true;
		}
		virtual ~ISpaceModifier(){}
		virtual ModifyType getType() const = 0;

		virtual bool isGroup() = 0;
		virtual void prevModify( World& world ) = 0;
		virtual bool canModify( World& world , float factor ) = 0;
		virtual void modify( World& world , float factor ) = 0;
		virtual void postModify( World& world ) = 0;

		virtual void updateValue( float factor ) = 0;
		virtual void prevRender() = 0;
		virtual void postRender() = 0;

		bool  isUse;
		bool  bModifyChildren;

	};

	class  IWorldModifier : public ISpaceModifier
	{
	public:
		virtual bool isGroup(){ return false; }




	};

	class  IGroupModifier : public ISpaceModifier
	{
	public:
		virtual ~IGroupModifier(){}
		virtual bool isGroup(){ return true; }

		virtual void prevModify( World& world );
		virtual void postModify( World& world );

		void addGroup( ObjectGroup& group )
		{
			assert( !group.spaceHook.isLinked() );
			mGourps.push_back( &group );
			group.node = this;
		}
		void removeGroup( ObjectGroup& group )
		{
			assert( group.node == this );
			group.spaceHook.unlink();
			group.node = NULL;
		}

		ObjectGroupList mGourps;
	};

	struct IRotator : public IGroupModifier
	{
	public:
		virtual ModifyType getType() const { return SNT_ROTATOR; }
		virtual bool canModify( World& world , float factor );
		virtual void modify( World& world , float factor );

		void rotate_R( ObjectGroup& group , int factor , bool modifyChildren );
		Dir   mDir;
		Vec3i mPos;
	};

	class IParallaxRotator  : public IWorldModifier
	{
	public:
		virtual void prevModify( World& world );
		virtual void modify( World& world , float factor );
		virtual void postModify( World& world );

		int mIndex;
	};

	class ITranslator : public IGroupModifier
	{
	public:
		ITranslator()
		{
			maxFactor = 1;
		}


		virtual void modify( World& world , float factor );
		virtual bool canModify( World& world , float factor );

		void translate_R( ObjectGroup& group , Vec3i const& offset , bool modifyChildren );


		Vec3i prevPos;
		Vec3i org;
		Vec3i diff;
		int   maxFactor;
	};


	struct SpaceControllor
	{
	public:
		void clearNode(){ mInfo.clear(); }
		void addNode( ISpaceModifier& modifier , float scale );
		void prevModify( World& world );
		bool modify( World& world , float factor );
		void updateValue( float factor );

		struct Info
		{
			ISpaceModifier* modifier;
			float           factorScale;
		};
		typedef std::vector< Info > InfoVec;
		InfoVec mInfo;

	};



}//namespace MV

#endif // MVSpaceControl_h__
