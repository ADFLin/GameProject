#ifndef MVLevel_h__
#define MVLevel_h__

#include "MVCommon.h"
#include "MVWorld.h"
#include "MVSpaceControl.h"

class DataSerializer;

namespace MV
{
	class IModifyCreator
	{
	public:
		virtual IRotator*    createRotator() = 0;
		//virtual ITranslator* createTranslator() = 0;
	};

	class Level
	{
	public:
		~Level();

		void save(DataSerializer& sr);
		void load(DataSerializer& sr);

		void cleanup( bool beDestroy );

		SpaceControllor* createControllor();
		MeshObject*      createMesh( 
			Vec3f const& pos , int idMesh , 
			Vec3f const& rotation , ObjectGroup* group = NULL );
		MeshObject*      createEmptyMesh( ObjectGroup* group = NULL );
		void             destroyMeshByIndex( int idx );

		IRotator*        createRotator( Vec3i const& pos , Dir dir );

		typedef std::vector< ISpaceModifier* >   ModifierVec;
		ModifierVec    mModifiers;
		typedef std::vector< SpaceControllor* > SpaceCtrlorVec;
		SpaceCtrlorVec  mSpaceCtrlors;
		typedef std::vector< MeshObject* >      MeshVec;
		MeshVec         mMeshVec;
		World           mWorld;
		IModifyCreator* mCreator;
	};

}//namespace MV



#endif // MVLevel_h__
