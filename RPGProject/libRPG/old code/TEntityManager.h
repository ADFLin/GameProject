#ifndef TEntityManager_h__
#define TEntityManager_h__

#include "common.h"
#include "TObject.h"
#include "singleton.hpp"
#include "CHandle.h"

#include <list>
#include <vector>



class EntityResultCallBack
{
public:
	~EntityResultCallBack(){}
	virtual bool  processResult( TEntityBase* entity ) = 0;
	// return true continue finding
};

class TEntityManager : public Singleton< TEntityManager >
{
public:
	typedef std::list< TEntityBase* > EntityList;
	typedef EntityList::iterator  iterator;
	
	TEntityManager();
	~TEntityManager();

	size_t     getEntityNum(){ return m_PEList.size(); }
	void       updateFrame();
	void       addEntity( TEntityBase* entity , bool needMark = false );
	void       removeEntity( TEntityBase* entity , bool beDel );
	void       removeEntity( EntityType type , bool beDel );

	
	void         findEntityInSphere( Vec3D const& pos , Float r  , EntityResultCallBack* callback );
	TPhyEntity*  findEntityInSphere( iterator& iter  , Vec3D const& pos , Float r2 );

	iterator     getIter(){ return m_PEList.begin(); }
	TEntityBase* visitEntity( iterator& iter );

	void       applyExplosion( Vec3D const& pos , float impulse , float range );

	void       debugDraw( bool beDebug );

protected:
	void       clearEntitySet( TEntityBase* entity );
	bool       m_debugShow;
	EntityList m_PEList;
	std::vector< TEntityBase*> m_deleteList;

public:
	struct EffectData
	{
		TEffectBase* effect;
		EHandle      handle;
	};
	typedef std::list< EffectData > EffectList;

	void addEffect( TEntityBase* entity , TEffectBase* effect );
	bool removeEffect( TEntityBase* entity , TEffectBase* effect );
	void clearEffect();
	void updateEffect();

protected:
	EffectList m_EffectList;

};




#endif // TEntityManager_h__
