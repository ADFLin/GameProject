#ifndef ImageScanser_h__
#define ImageScanser_h__

#define  NOMINMAX
#include <Windows.h>
#include <vector>
#include <list>
#define ERROR_SCAN_ID (-1)
struct ScanPixel
{
	COLORREF color;
	short    x , y;
};

typedef std::vector< ScanPixel > ScanPixelList;
struct ScanShape
{
	unsigned  regID;
	int       error;
	ScanPixelList pixels;
};

typedef std::list< ScanShape > ScanShapeList;



class ScanGroup
{
public:

	ScanShape* findShape( unsigned id )
	{
		for( ScanShapeList::iterator iter = shapes.begin() ; 
			 iter != shapes.end() ; ++iter )
		{
			if ( iter->regID == id )
				return &(*iter);
		}
		return NULL;
	}
	ScanShape& addShape( unsigned id , int error )
	{
		ScanShape* shape = findShape( id );
		if ( shape )
		{
			shape->error = error;
			shape->pixels.clear();
			return *shape;
		}

		shapes.push_back( ScanShape() );
		shapes.back().regID = id;
		shapes.back().error = error;
		return shapes.back();
	}

	void  addPixel( unsigned id , int x , int y , COLORREF color )
	{
		ScanShape* shape = findShape( id );
		if ( !shape )
			return;

		ScanPixel sp;
		sp.x = x;
		sp.y = y;
		sp.color = color;
		shape->pixels.push_back( sp );

	}

	HDC   hDC;
	int   errorConut;
	ScanShapeList shapes; 
};


struct ColorCmpPolicy
{

};

class ImageScanser
{
public:
	bool scanShape( HDC hDC , int x0 , int y0 , ScanShape const& shape , int maxErrorCount )
	{
		int count;
		for ( ScanPixelList::const_iterator iter = shape.pixels.begin() ; 
			  iter != shape.pixels.end() ; ++iter )
		{
			ScanPixel const& sp = *iter;
			COLORREF color = ::GetPixel( hDC , x0 + sp.x , y0 + sp.y );
			if ( !isSameColor( color , iter->color , shape.error ) )
			{
				++count;
				if( count >= maxErrorCount )
					return false;
			}
		}
		return true;
	}
	unsigned scan( int x , int y , ScanGroup const& group )
	{
		for( ScanShapeList::const_iterator iter = group.shapes.begin();
			 iter != group.shapes.end() ; ++iter )
		{
			if ( scanShape( group.hDC , x , y , *iter , group.errorConut ) )
				return iter->regID;
		}
		return ERROR_SCAN_ID;
	}

	static bool isSameColor( COLORREF color , COLORREF cmpColor , int  error )
	{
		int dR = abs( GetRValue( color ) - GetRValue( cmpColor ) );
		if ( dR > error ) return false;
		int dG = abs( GetGValue( color ) - GetGValue( cmpColor ) );
		if ( dG > error ) return false;
		int dB = abs( GetBValue( color ) - GetBValue( cmpColor ) );
		if ( dB > error ) return false;
		//if ( dR + dB + dG > 30 )
		//	return false;
		return true;
	}
};

class ScanBuilder
{
public:
	struct ShapeInfo
	{
		unsigned  regID;
		unsigned  error;
		unsigned  idxPixel;
	};


	void  importData( ShapeInfo info[] , int numShape , ScanPixel pixels[] )
	{
		mBuildGroup->shapes.clear();
		int prevIdx = 0;
		for( int i = 0 ; i < numShape ; ++i )
		{
			ScanShape& shape = mBuildGroup->addShape( info[i].regID , info[i].error );
			for( int n = prevIdx ; n < info[i].idxPixel ; ++n )
			{
				shape.pixels.push_back( pixels[n] );
			}
		}
	}

	void  exportData( ShapeInfo info[] , int& numShape , ScanPixel pixels[] )
	{
		numShape = mBuildGroup->shapes.size();
		int idxShape = 0;
		int idxPixel = 0;
		for( ScanShapeList::iterator iter = mBuildGroup->shapes.begin() ; 
			 iter != mBuildGroup->shapes.end() ; ++iter )
		{
			ScanShape& shape = *iter;
			info[ idxShape ].regID = shape.regID;
			info[ idxShape ].error = shape.error;
			for( ScanPixelList::iterator piter = shape.pixels.begin() ; 
				 piter != shape.pixels.end() ; ++piter )
			{
				pixels[ idxPixel++ ] = *piter;
			}
			info[ idxShape ].idxPixel = idxPixel;
			++idxShape;
		}
	}

	bool  saveSrcData( char const* path , char const* dataName );

	void  createShpae( unsigned id , int error )
	{
		mBuildGroup->addShape( id , error );
	}
	void  addPixel( unsigned id , int x , int y )
	{
		COLORREF color = ::GetPixel( mBuildGroup->hDC , x , y );
		mBuildGroup->addPixel( id , x - mDPx , y - mDPy , color );
	}

	void setHDC( HDC hDC )
	{
		mBuildGroup->hDC = hDC;
	}
	void setDatumPoint(  int x , int y )
	{
		mDPx = x;
		mDPy = y;
	}
	void setGroup( ScanGroup* group )
	{
		mBuildGroup = group;
	}
	int mDPx;
	int mDPy;
	ScanGroup* mBuildGroup;
};

#endif // ImageScanser_h__
