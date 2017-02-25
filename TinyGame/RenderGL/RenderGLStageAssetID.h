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
	op(Lightning)\
	op(POMTitle)\

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
	op( CircleN , "Texture/rocks_NM_height.tga")\
	\
	op(Metel , "Model/Metel.tga")\
    op(ElephantSkin , "Model/ELPHSKIN.png")\
	op(MarioD, "Model/marioD.png")\
	op(MarioS, "Model/marioS.png")\
	\
	op(Lightning00, "Model/Lightning/c001_00.tga")\
	op(Lightning01, "Model/Lightning/c001_01.tga")\
	op(Lightning02, "Model/Lightning/c001_02.tga")\
	op(Lightning03, "Model/Lightning/c001_03.tga")\
	op(Lightning04, "Model/Lightning/c001_04.tga")\
	op(Lightning05, "Model/Lightning/c001_05.tga")\
	op(Lightning06, "Model/Lightning/c001_06.tga")\
	op(Lightning07, "Model/Lightning/c001_07.tga")\
	op(Lightning08, "Model/Lightning/c001_08.tga")\
	op(Lightning09, "Model/Lightning/c001_09.tga")\
	op(Lightning10, "Model/Lightning/c001_10.tga")\
	op(Lightning11, "Model/Lightning/c001_11.tga")\
	op(Lightning12, "Model/Lightning/c001_12.tga")\
	op(Lightning13, "Model/Lightning/c001_13.tga")\
	op(Lightning14, "Model/Lightning/c001_14.tga")\
	op(Lightning15, "Model/Lightning/c001_15.tga")\
	op(Lightning16, "Model/Lightning/c001_16.tga")\
	op(Lightning17, "Model/Lightning/c001_17.tga")\
	op(LightningW0, "Model/Lightning/w001_00.tga")\
	op(LightningW1, "Model/Lightning/w001_01.tga")\
	\
	op(Vanille00 , "Model/Vanille/c595_00.tga")\
	op(Vanille01 , "Model/Vanille/c595_01.tga")\
	op(Vanille02 , "Model/Vanille/c595_02.tga")\
	op(Vanille03 , "Model/Vanille/c595_03.tga")\
	op(Vanille04 , "Model/Vanille/c595_04.tga")\
	op(Vanille05 , "Model/Vanille/c595_05.tga")\
	op(Vanille06 , "Model/Vanille/c595_06.tga")\

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
	op(Havoc , "Model/havoc.obj")\
    op(Dragon , "Model/Dragon.obj")\
	op(Teapot , "Model/teapot.obj")\
	op(Elephant, "Model/elephant.obj")\
	op(Mario, "Model/Mario.obj")\
	op(Lightning, "Model/lightning.obj")\
    op(Vanille , "Model/Vanille.obj")\
	op(Skeleton, "Model/Skeleton.obj")\


#define EnumOp( NAME , PATH ) NAME ,
			MESH_LIST(EnumOp)
#undef EnumOp
		};
	}
}
