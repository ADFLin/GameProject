#ifndef MVLevel_h__
#define MVLevel_h__

#include "MVCommon.h"
#include "MVWorld.h"
#include "MVSpaceControl.h"

#include "DataStructure/Array.h"

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

		void save(IStreamSerializer& sr);
		void load(IStreamSerializer& sr);

		void cleanup( bool beDestroy );

		SpaceControllor* createControllor();
		MeshObject*      createMesh( 
			Vec3f const& pos , int idMesh , 
			Vec3f const& rotation , ObjectGroup* group = nullptr );
		MeshObject*      createEmptyMesh( ObjectGroup* group = nullptr );
		void             destroyMeshByIndex( int idx );

		IRotator*        createRotator( Vec3i const& pos , Dir dir );

		typedef TArray< ISpaceModifier* >   ModifierVec;
		ModifierVec    mModifiers;
		typedef TArray< SpaceControllor* > SpaceCtrlorVec;
		SpaceCtrlorVec  mSpaceCtrlors;
		typedef TArray< MeshObject* >      MeshVec;
		MeshVec         mMeshVec;
		World           mWorld;
		IModifyCreator* mCreator;
	};

}//namespace MV



#endif // MVLevel_h__
