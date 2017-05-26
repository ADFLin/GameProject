namespace RenderGL
{
	namespace MaterialId
	{
		enum Enum
		{
#define MATERIAL_LIST( op ) \
	op( Simple1 )\
	op( Simple2 )\
	op( Simple3 )\
	op( MetelA )\
	op( Elephant )\
	op( Mario )\
	op( Lightning )\
	op( POMTitle )\
	op( Sphere )\
	op( Havoc )\


#define EnumOp( NAME ) NAME ,
			MATERIAL_LIST(EnumOp)
#undef EnumOp
		};
	}

	namespace TextureId
	{
		enum Enum
		{
#define TEXTURE_LIST(op)\
	op( Base , "Texture/tile1.png" )\
	op( Normal , "Texture/tile1.tga" )\
	op( RocksD, "Texture/rocks.png")\
	op( RocksNH, "Texture/rocks_NM_height.tga")\
	op( CircleN , "Texture/CircleN.tga")\
	op( Star , "Texture/star.png")\
	\
	op(Metel , "Model/Metel.tga")\
	op(MarioD, "Model/marioD.png")\
	op(MarioS, "Model/marioS.png")\


#define EnumOp( NAME , PATH ) NAME ,
			TEXTURE_LIST(EnumOp)
#undef EnumOp
		};
	}

	namespace MeshId
	{
		enum Enum
		{
#define MESH_LIST(op)\
	op(Sponza, "Model/Sponza.obj")\
    op(Dragon2 , "Model/Dragon2.obj")\
	op(Teapot , "Model/teapot.obj")\
	op(Elephant, "Model/elephant.obj")\
	op(Mario, "Model/Mario.obj")\
	op(Lightning, "Model/lightning.obj")\
    op(Vanille , "Model/Vanille.obj")\
	op(Havoc, "Model/havoc.obj")\
	op(Skeleton, "Model/Skeleton.obj")\
	op(Dragon, "Model/Dragon.obj")\
	


#define EnumOp( NAME , PATH ) NAME ,
			MESH_LIST(EnumOp)
#undef EnumOp
		};
	}
}
