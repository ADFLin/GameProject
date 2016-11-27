#ifndef CFVertexDecl_h__
#define CFVertexDecl_h__

namespace CFly
{
	typedef uint32 VertexType;

	enum VertexElementSemantic
	{
		CFV_XYZ          = 1,
		CFV_XYZW         = 2,
		CFV_XYZRHW       = 3,
		CFV_BLENDWEIGHT  = BIT(2),
		CFV_BLENDINDICES = BIT(3),
		CFV_NORMAL       = BIT(4),
		CFV_TANGENT      = BIT(5),
		CFV_BINORMAL     = BIT(6),
		CFV_PSIZE        = BIT(7),
		CFV_COLOR        = BIT(8),
		CFV_COLOR2       = BIT(9),
		CFV_TEXCOORD     = BIT(10),
	};

#define CFVF_TEXCOUNT_MASK       ( 0x00070000 )
#define CFVF_TEXCOUNT_OFFSET     (4 * 4)
#define CFVF_TEXSIZE_MASK        0x3
#define CFVF_TEXSIZE_OFFSET( IDX ) ( ( ( IDX ) * 2 ) + 16 + 3 ) 
#define CFVF_BLENDWEIGHT_MASK    0x00003000
#define CFVF_BLENDWEIGHT_OFFSET  ( 4 * 3 )
#define CFVF_TEXCOUNT_GET( TYPE )        ( ( ( ( TYPE ) & CFVF_TEXCOUNT_MASK ) >> CFVF_TEXCOUNT_OFFSET ) + 1 )
#define CFVF_TEXSIZE_GET( TYPE , IDX )   ( ( ( ( TYPE ) >> CFVF_TEXSIZE_OFFSET( IDX ) ) & CFVF_TEXSIZE_MASK  ) + 1 )
#define CFVF_BLENDWEIGHTSIZE_GET( TYPE ) ( ( ( ( TYPE ) & CFVF_BLENDWEIGHT_MASK ) >> CFVF_BLENDWEIGHT_OFFSET ) + 1 )

#define CFVF_COLOR_FMT_BYTE      0x00004000
#define CFVF_COLOR_ALPHA         0x00008000
#define CFVF_BLENDWEIGHTSIZE( SIZE ) ( CFV_BLENDWEIGHT | ( ( ( SIZE ) - 1 ) << CFVF_BLENDWEIGHT_OFFSET ) )
#define CFVF_TEXCOUNT( COUNT )       ( CFV_TEXCOORD | ( ( ( COUNT ) - 1 ) << CFVF_TEXCOUNT_OFFSET ) )
#define CFVF_TEXSIZE( IDX , SIZE )   ( ( ( SIZE ) - 1 ) << CFVF_TEXSIZE_OFFSET( IDX ) )
#define CFVF_TEX1( T0 ) \
	( CFVF_TEXCOUNT( 1 ) | CFVF_TEXSIZE( 0 , T0 ) )
#define CFVF_TEX2( T0 , T1 ) \
	( CFVF_TEXCOUNT( 2 ) | CFVF_TEXSIZE( 0 , T0 ) | CFVF_TEXSIZE( 1 , T1 ) )
#define CFVF_TEX3( T0 , T1 , T2 ) \
	( CFVF_TEXCOUNT( 3 ) | CFVF_TEXSIZE( 0 , T0 ) | CFVF_TEXSIZE( 1 , T1 ) | CFVF_TEXSIZE( 2 , T2 ) )
#define CFVF_TEX4( T0 , T1 , T2 , T3 ) \
	( CFVF_TEXCOUNT( 4 ) | CFVF_TEXSIZE( 0 , T0 ) | CFVF_TEXSIZE( 1 , T1 ) | CFVF_TEXSIZE( 2 , T2 ) | CFVF_TEXSIZE( 3 , T3 ) )
#define CFVF_TEX5( T0 , T1 , T2 , T3 , T4 ) \
	( CFVF_TEXCOUNT( 5 ) | CFVF_TEXSIZE( 0 , T0 ) | CFVF_TEXSIZE( 1 , T1 ) | CFVF_TEXSIZE( 2 , T2 ) | CFVF_TEXSIZE( 3 , T3 ) | CFVF_TEXSIZE( 4 , T4 ) )
#define CFVF_TEX6( T0 , T1 , T2 , T3 , T4 , T5 ) \
	( CFVF_TEXCOUNT( 6 ) | CFVF_TEXSIZE( 0 , T0 ) | CFVF_TEXSIZE( 1 , T1 ) | CFVF_TEXSIZE( 2 , T2 ) | CFVF_TEXSIZE( 3 , T3 ) | CFVF_TEXSIZE( 4 , T4 ) | CFVF_TEXSIZE( 5 , T5 ))
#define CFVF_TEX7( T0 , T1 , T2 , T3 , T4 , T5 , T6 ) \
	( CFVF_TEXCOUNT( 7 ) | CFVF_TEXSIZE( 0 , T0 ) | CFVF_TEXSIZE( 1 , T1 ) | CFVF_TEXSIZE( 2 , T2 ) | CFVF_TEXSIZE( 3 , T3 ) | CFVF_TEXSIZE( 4 , T4 ) | CFVF_TEXSIZE( 5 , T5 ) | CFVF_TEXSIZE( 6 , T6 ) )


#define CFVF_POSITION_MASK     ( 0x3 )
#define CFVF_VERTEX_MASK       ( CFVF_POSITION_MASK | CFV_BLENDINDICES | CFV_BLENDWEIGHT )
#define CFVF_BLEND_WEIGHT_MASK ( CFV_BLENDINDICES | CFV_BLENDWEIGHT )
#define CFVF_GEOM_MASK         ( CFV_NORMAL | CFV_TANGENT | CFV_BINORMAL )
#define CFVF_COLOR_TEX_MASK    ( CFV_COLOR | CFV_COLOR2 | CFV_TEXCOORD )

	enum EnumVertexType
	{
		CFVT_UNKONWN    = 0,
		CFVT_XYZ        = CFV_XYZ ,
		CFVT_XYZ_N      = CFV_XYZ | CFV_NORMAL,
		CFVT_XYZ_CF1    = CFV_XYZ | CFV_COLOR ,
		CFVT_XYZ_N_CF1  = CFV_XYZ | CFV_NORMAL | CFV_COLOR ,
		CFVT_XYZ_NT_CF1 = CFV_XYZ | CFV_NORMAL | CFV_TANGENT | CFV_COLOR ,

		CFVT_XYZ_CAF1    = CFVT_XYZ_CF1    | CFVF_COLOR_ALPHA ,
		CFVT_XYZ_N_CAF1  = CFVT_XYZ_N_CF1  | CFVF_COLOR_ALPHA ,
		CFVT_XYZ_NT_CAF1 = CFVT_XYZ_NT_CF1 | CFVF_COLOR_ALPHA ,

		CFVT_XYZ_CB1    = CFVT_XYZ_CF1    | CFVF_COLOR_FMT_BYTE ,
		CFVT_XYZ_N_CB1  = CFVT_XYZ_N_CF1  | CFVF_COLOR_FMT_BYTE ,
		CFVT_XYZ_NT_CB1 = CFVT_XYZ_NT_CF1 | CFVF_COLOR_FMT_BYTE ,

		CFVT_XYZ_CF2    = CFVT_XYZ_CF1    | CFV_COLOR2 ,
		CFVT_XYZ_N_CF2  = CFVT_XYZ_N_CF1  | CFV_COLOR2 ,
		CFVT_XYZ_NT_CF2 = CFVT_XYZ_NT_CF1 | CFV_COLOR2 ,
		CFVT_XYZ_CB2    = CFVT_XYZ_CB1    | CFV_COLOR2 ,
		CFVT_XYZ_N_CB2  = CFVT_XYZ_N_CB1  | CFV_COLOR2 ,
		CFVT_XYZ_NT_CB2 = CFVT_XYZ_NT_CB1 | CFV_COLOR2 ,
	};

	enum VertexTypeFormat
	{
		CFTF_FLOAT1 ,
		CFTF_FLOAT2 ,
		CFTF_FLOAT3 ,
		CFTF_FLOAT4 ,

		CFTF_BYTE4 ,
		CFTF_SHORT4 ,
		CFTF_SHORT2 ,
	};

	class VertexDecl : public RefObjectT< VertexDecl > 
	{
	public:
		typedef VertexElementSemantic Sematic;
		typedef VertexTypeFormat      Format;
		VertexDecl& addElement( uint32 src , uint32 offset , Format format , Sematic sematic , uint8 idxSematic = 0 )
		{



			return *this;
		}

		class Element
		{
			uint16  source;
			uint16  offset;
			Format  format;
			Sematic semantic;
			uint8   idxSemantic;
		};

		Element* findElement( Sematic semantic )
		{




			return nullptr;
		}

		void*  getSystemImpl(){ return mImpl; }


		
		void*    mImpl;
		unsigned mVertexTypeBit;
	};


}//namespace CFly

#endif // CFVertexDecl_h__
