#ifndef CRenderComponent_h__
#define CRenderComponent_h__

#include "common.h"
#include "CEntity.h"
	
typedef CFly::SceneNode RenderNode;
typedef CFly::Scene     RenderScene;

enum RenderEntityType
{
	RET_OBJECT_MODEL ,
	RET_ACTOR_MODEL  ,
};

class IRenderEntity
{
public:
	virtual void changeScene( RenderScene* scene ) = 0;
	virtual RenderNode* getRenderNode() = 0;
	virtual void release() = 0;
};

struct SRenderEntityCreateParams
{
	unsigned         resType;
	char const*      name;
	CFScene*         scene;
};

class CRenderComponent : public IComponent
{
public:
	CRenderComponent():IComponent( COMP_RENDER ){}

	//IRenderEntity* createEmptyRenderEntity( RenderEntityType type ) = 0;
	IRenderEntity* getRenderEntity( unsigned slot );
	IRenderEntity* createRenderEntity( unsigned slot , SRenderEntityCreateParams const& params );
	void           releaseRenderEntity( unsigned slot );

	typedef std::vector< IRenderEntity* > RenderEntityVec;
	RenderEntityVec mRenderEntities;

};

class CObjectModel : public IRenderEntity
{
public:
	CObjectModel( CFObject* object ):mCFObject( object ){}
	
	//virtual 
	RenderNode* getRenderNode();
	//virtual 
	void  changeScene( RenderScene* scene );
	void  release();

protected:
	CFObject* mCFObject;
};

#endif // CRenderComponent_h__
