#ifndef THeighField_h__
#define THeighField_h__

#include "common.h"

class THeighField;

struct TerBrush
{
	float     strength; // 0 - 100
	float     radius;
	float     falloff;
	bool      beInv;
	int       texLayer;

};



struct BrushObj
{
	BrushObj( FnScene& scene  , TerBrush& brush , int s );
	void updateRadius( TerBrush& brush );
	void updatePos(  Vec3D const& pos , THeighField&  field , TerBrush& brush );
private:
	void computeHeight( int gemoID , THeighField&  field );
	FnObject obj;
	int segment;
	int sCircle;
	int bCircle;
};


class TerEffect
{
public:
	struct CData
	{
		void* val;
		int   index;
		int   sx;
		int   sy;
		int   ix;
		int   iy;
	};
	virtual void fireMethod( THeighField& field , TerBrush& brush , Vec3D const& lPos ) = 0;
	virtual bool process( CData& data , TerBrush&  brush ,  float dist  ){ return false; }

	
	static float computeFactor( TerBrush& brush , float dist );
};

class TerLiftEffect : public TerEffect
{
protected:
	virtual void fireMethod( THeighField& field , TerBrush& brush , Vec3D const& lPos );
	virtual bool process( CData& data , TerBrush&  brush ,  float dist  );

	float   incHeight;
};

class TerSmoothEffect : public TerEffect
{
protected:
	virtual void fireMethod( THeighField& field , TerBrush& brush , Vec3D const& lPos );
	virtual bool process( CData& data , TerBrush&  brush ,  float dist  );

	float*  saveData;
	int     numData;
	float   incHeight;
};

class TerAvgHeightEffect : public TerEffect
{
protected:
	virtual void fireMethod( THeighField& field , TerBrush& brush , Vec3D const& lPos );
	virtual bool process( CData& data , TerBrush&  brush ,  float dist  );

	float   incHeight;
};

class TerTextureEffect : public TerEffect
{
protected:
	virtual void fireMethod( THeighField& field , TerBrush& brush , Vec3D const& lPos );
	virtual bool process( CData& data , TerBrush&  brush ,  float dist  );

	int     incVal;
};


class THeighField
{
public:
	THeighField( FnScene& scene , int sx , int sy , float cellSize );

	void         paint( Vec3D const& wPos , TerBrush& brush , TerEffect& effect );
	Vec3D        transToLocalPos( Vec3D const& wPos );
	FnObject&    getObject(){ return m_hostObj; }
	FnMaterial&  getMaterial(){ return m_mat; }
	bool         getHeight( Vec3D const& wPos , float* heigh );


	void         paintTexMethod( Vec3D const& lPos , TerBrush& brush , TerEffect& effect );
	void         paintVtxMethod( Vec3D const& lPos , TerBrush& brush , TerEffect& effect );

	float*       copyHeightData();

	int          getCellSizeX(){ return sizeX; }
	int          getCellSizeY(){ return sizeY; }

	struct  TextureLayer
	{
		TString  filePath;
		float    scale;
		int      texID;

	};

private:
	int          addTextureLayer( char const* texName );
	void         rebulid();

	int          getVertexLen(){ return 3 + 6; }
	
	static int const MaxTexLayerNum = 5; 

	FnObject   m_hostObj;
	FnMaterial m_mat;
	int        texSizeFactor;

	int        m_gemoID;
	int        numTex;
	float*     vertex;
	float      m_cellLength;
	int        sizeX;
	int        sizeY;
};


#endif // THeighField_h__