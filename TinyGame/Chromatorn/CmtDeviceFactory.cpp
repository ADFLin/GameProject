#include "CmtPCH.h"
#include "CmtDeviceFactory.h"

#include "CmtDeviceID.h"
#include "CmtWorld.h"
#include "CmtLightTrace.h"

namespace Chromatron
{
	static DeviceInfo gDeviceInfo[ DC_DEVICE_NUM ];
	static void initDeviceInfo();

	static struct DcInfoInitHelper
	{
		DcInfoInitHelper()
		{
			initDeviceInfo();
		}
	}gHelper;


	Device* DeviceFactory::create( DeviceId id , Dir dir, Color color )
	{
		assert( 0 <= id && id < DC_DEVICE_NUM );
		return new Device( getInfo( id ) , dir , color );
	}

	DeviceInfo& DeviceFactory::getInfo( DeviceId id )
	{
		return gDeviceInfo[ id ];
	}

	void DeviceFactory::destroy( Device* dc )
	{
		delete dc;
	}

	struct DeviceFun
	{
		static Dir calcIncidentDir( Device const& dc , LightTrace const& light )
		{
			return light.getDir().inverse() - dc.getDir();
		}

		static Color calcDopplerColor( Color color , bool beSameDir )
		{
			if ( beSameDir )
				return Color( ( color >> 1 ) | ( ( color & COLOR_R ) << 2 ) );
			else
				return Color( ( ( color & ( COLOR_G | COLOR_R ) ) << 1 ) | ( ( color & COLOR_B ) >> 2 ) );
		}
		template< int ID >
		static void effect( Device& dc , World& world , LightTrace const& light ){}
		template< int ID >
		static void update( Device& dc , World& world ){}
		template< int ID >
		static bool check( Device& dc , World& world ){ return true; }
	};

	template<>
	void DeviceFun::update< DC_LIGHTSOURCE >( Device& dc , World& world )
	{
		world.addLight( dc.getPos() , dc.getColor() , dc.getDir() );
	}

	template<>
	void DeviceFun::effect< DC_PINWHEEL >( Device& dc , World& world , LightTrace const& light )
	{
		world.addLight( dc.getPos() , light.getColor() , light.getDir() );
	}

	template<>
	bool DeviceFun::check< DC_PINWHEEL >( Device& dc , World& world )
	{
		Color color = COLOR_NULL;
		Tile const& mapData = world.getMapData( dc.getPos() );

		Color testColor = dc.getColor();
		Color invColor = COLOR_W & ( ~testColor );

		for(int i = 0 ; i < 4 ;++i )
		{
			Dir dir = Dir::ValueNoCheck( i );
			Color c1 = mapData.getLightPathColor( dir );
			Color c2 = mapData.getLightPathColor( dir.inverse() );
			if ( ( c1 & invColor ) || ( c2 & invColor ) )
				return false;

			color |= c1 & c2;
		}
		return color == dc.getColor();
	}

	template<>
	void DeviceFun::effect< DC_SMIRROR >( Device& dc , World& world , LightTrace const& light )
	{
		Dir dir = calcIncidentDir( dc, light );

		if ( dir >=2 && dir <=6 ) 
			return;
		//int dout=2*m_dir-arcdir;
		dir = dc.getDir() - dir;

		world.addLight( dc.getPos() ,light.getColor(),dir);
	}

	template<>
	void DeviceFun::effect< DC_CONDUITS >( Device& dc , World& world , LightTrace const& light )
	{
		Dir dir = calcIncidentDir( dc, light );
		if( dir == 0 || dir == 4 )
		{
			world.addLight( dc.getPos() , light.getColor(), light.getDir() );
		}
	}

	template<>
	void DeviceFun::effect< DC_SPECTROSCOPE >( Device& dc , World& world , LightTrace const& light )
	{
		Dir dir = calcIncidentDir( dc, light );

		if ( dir == 2 || dir == 6 ) 
			return;

		dir = dc.getDir() - dir;

		//if ( dir != light.getDir().inverse() )
		world.addLight( dc.getPos() , light.getColor(), dir );

		world.addLight( dc.getPos() , light.getColor(), light.getDir() );
	}

	template<>
	void DeviceFun::effect< DC_SMIRROR_45 >( Device& dc , World& world , LightTrace const& light )
	{
		Dir dir = calcIncidentDir( dc, light );

		if ( dir > 2 && dir <= 6 ) 
			return;

		//int dout=2*m_dir-arcdir+1;
		dir = dc.getDir() - dir + Dir(1);

		world.addLight( dc.getPos() , light.getColor(),dir);
	}

	template<>
	void DeviceFun::effect< DC_PRISM >( Device& dc , World& world , LightTrace const& light )
	{
		Dir dir = calcIncidentDir( dc, light );
		Dir invDir = light.getDir().inverse();

		Color color=light.getColor();

		int const outR[] = { -1,-1, 4, 4,-1,-1, 4, 4 };
		int const outG[] = {  3, 5,-1, 5,-1,-1, 3,-1 };
		int const outB[] = {  6, 2,-1, 6,-1,-1, 2,-1 };

		if( ( color & COLOR_R ) && outR[dir] != -1 )
		{
			Dir outDir = invDir + Dir( outR[dir] );
			world.addLight( dc.getPos() , COLOR_R , outDir );
		}
		if( ( color & COLOR_G ) && outG[dir] != -1 )
		{
			Dir outDir = invDir + Dir( outG[dir] );
			world.addLight( dc.getPos() , COLOR_G , outDir );
		}
		if( ( color & COLOR_B ) && outB[dir] != -1 )
		{
			Dir outDir = invDir + Dir( outB[dir] );
			world.addLight( dc.getPos() , COLOR_B , outDir );
		}
	}

	template<>
	void DeviceFun::effect< DC_FILTER >( Device& dc , World& world , LightTrace const& light )
	{
		Dir dir = calcIncidentDir( dc, light );

		if ( dir!= 0 && dir != 4 ) 
			return;

		Color color = Color( light.getColor() & dc.getColor() );
		if (  color )
		{
			world.addLight( dc.getPos() , color , light.getDir() );
		}
	}

	template<>
	void DeviceFun::effect< DC_DOPPLER >( Device& dc , World& world , LightTrace const& light )
	{
		Dir dir = calcIncidentDir( dc, light );

		if ( dir != 0 && dir != 4 )
			return;

		Color color =light.getColor();
		Color outColor = calcDopplerColor( color , dir == Dir(0) );

		world.addLight( dc.getPos() , outColor,light.getDir() );
	}

	class QTangterEffectSolver : public World::SyncProcessor
	{
		typedef World::LightList LightList;
	public:
		QTangterEffectSolver( World& w ):mWorld(w){}
		~QTangterEffectSolver(){}

		static int const QStatsUp   =  1;
		static int const QStatsDown = -1;

		void solve( Vec2D const& pos , LightTrace const& light )
		{
			Color color = light.getColor();

			bool prevMode = mWorld.isSyncMode();
			mWorld.setSyncMode( true );

			bool keep = true;
			if ( keep && ( color & COLOR_R ) )
				keep = solveColor( pos , light.getDir() , COLOR_R );
			if ( keep && ( color & COLOR_G ) )
				keep = solveColor( pos , light.getDir() , COLOR_G );
			if ( keep && ( color & COLOR_B ) )
				keep = solveColor( pos , light.getDir() , COLOR_B );

			mWorld.setSyncMode( prevMode );
		}

		bool solveColor( Vec2D const& pos , Dir const& dir , Color color )
		{
			if ( mWorld.getMapData( pos ).getReceivedLightColor( dir.inverse() ) & color )
				return true;

			mTransmitLights.clear();
			mCheckLights.clear();

			mTransmitLights.push_back( LightTrace( pos , color , dir + Dir(2) , QStatsUp ) );
			mTransmitLights.push_back( LightTrace( pos , color , dir - Dir(2) , QStatsDown ) );

			if ( mWorld.transmitLightSync( *this , mTransmitLights ) != TSS_OK )
				return false;

			return true;
		}

		virtual bool prevEffectDevice( Device& dc  , LightTrace const& light , int pass )
		{
			bool result = true;

			switch( dc.getId() )
			{
			case DC_DOPPLER:
				if ( pass == 1 )
				{
					Dir dir = DeviceFun::calcIncidentDir( dc , light );
					if ( dir == 0 || dir == 4 )
					{
						bool isSameDir = ( dir == 0 );
						applyQuantumEffect( light , !isSameDir );
					}
				}
				else
				{
					result = false;
				}
				break;
			}

			if ( result )
			{
				if ( dc.getFlag() & DFB_REMOVE_QE )
				{
					mWorld.setSyncMode( false );
					mWorld.setLightParam( 0 );
				}
				else
				{
					mWorld.setSyncMode( true );
					mWorld.setLightParam( light.getParam() );
				}
			}

			return result;
		}

		bool prevAddLight( Vec2D const& pos , Color color , Dir dir , int param , int age )
		{
			if ( param == 0 )
			{
				for ( LightList::iterator iter = mCheckLights.begin();
					iter != mCheckLights.end() ; ++iter )
				{
					if ( iter->getStartPos() == pos &&
						iter->getColor()    == color &&
						iter->getDir()      == dir &&
						iter->getParam()    == param )
					{
						return false;
					}
				}

				LightTrace light( pos , color , dir , param );
				mCheckLights.push_back( light );
				return true;
			}

			int  paramAnti = ( param != QStatsUp ) ? QStatsUp : QStatsDown;
			bool haveFound = false;
			bool haveAnti  = false;

			LightList&  lightList = mTransmitLights;
			for( LightList::iterator iter = lightList.begin();
				iter != lightList.end();++iter )
			{
				LightTrace const& light = *iter;
				if ( light.getParam() == paramAnti )
				{
					haveAnti = true;
					break;
				}
			}

			for ( LightList::iterator iter = mCheckLights.begin();
				iter != mCheckLights.end() ; ++iter )
			{
				LightTrace const& light = *iter;

				if ( light.getStartPos() == pos &&
					light.getColor()    == color &&
					light.getDir()      == dir &&
					light.getParam()    == param )
				{
					haveFound = true;
					break;
				}
			}
			int const MaxTransmitLightAge = 500;

			if ( haveFound )
			{
				if ( age > MaxTransmitLightAge )
					return false;
				return haveAnti;
			}
			LightTrace light( pos , color , dir , param );
			mCheckLights.push_back( light );

			return true;
		}

		void applyQuantumEffect( LightTrace const& light , bool beSameDir )
		{
			LightList&  lightList = mTransmitLights;
			for( LightList::iterator iter = lightList.begin();
				iter != lightList.end();++iter )
			{
				if ( iter->getParam() != light.getParam() )
				{
					Color outColor = DeviceFun::calcDopplerColor( iter->getColor() , beSameDir );
					iter->setColor( outColor );
				}
			}
		}

		LightList  mTransmitLights;
		LightList  mCheckLights;
		Vec2D      mPos;
		Dir        mDir;
		Color      mColor;
		World&     mWorld;
	};

	template<>
	void DeviceFun::effect< DC_QTANGLER >( Device& dc , World& world , LightTrace const& light )
	{
		Dir dir = calcIncidentDir( dc, light );	
		if ( dir != 0 ) 
			return;

		QTangterEffectSolver solver( world );
		solver.solve( dc.getPos() , light );
	}

	class QRotatorEffectSolver : public World::SyncProcessor
	{
	public:
		QRotatorEffectSolver( World& w ):mWorld(w){}

		void solve( Vec2D const& pos , LightTrace const& light )
		{
			bool keep = true;
			bool prevMode = mWorld.isSyncMode();


			mWorld.setSyncMode( prevMode );
		}



		World& mWorld;
	};

	template<>
	void DeviceFun::effect< DC_QUANROTATOR >( Device& dc , World& world , LightTrace const& light )
	{
		Dir dir = calcIncidentDir( dc, light );	
		if ( dir != 0 ) 
			return;

		//QRotatorEffectSolver solver( world );
		//solver.solve( dc.getPos() , light );
	}


	template<>
	void DeviceFun::effect< DC_TELEPORTER >( Device& dc , World& world , LightTrace const& light )
	{
		Dir dir = light.getDir();
		Vec2D pos = dc.getPos();

		Device* pDC;
		while( pDC = world.goNextDevice( dir , pos ) )
		{
			if ( pDC->getId() == DC_TELEPORTER )
			{
				world.addLight( pDC->getPos() , light.getColor(), dir );
				return;
			}
		}
	}

	template<>
	void DeviceFun::effect< DC_MULTIFILTER >( Device& dc , World& world , LightTrace const& light )
	{
		Dir dir = calcIncidentDir( dc, light );
		int select = dir % 4;
		static Color const filterColor[] = { COLOR_W , COLOR_R , COLOR_G , COLOR_B };

		Color color = Color( light.getColor() & filterColor[select] );
		if (color) 
			world.addLight( dc.getPos() ,  color, light.getDir());

	}

	template<>
	void DeviceFun::effect< DC_TWISTER >( Device& dc , World& world , LightTrace const& light )
	{
		Dir turnDir = ( dc.getFlag() & DFB_CLOCKWISE ) ?Dir(-2):Dir(2);
		Dir outDir(light.getDir() + turnDir);

		world.addLight( dc.getPos() , light.getColor(), outDir );
	}

	template<>
	void DeviceFun::effect< DC_DUALREFLECTOR >( Device& dc , World& world , LightTrace const& light )
	{
		Dir dir = calcIncidentDir( dc, light );
		if ( dir == 2 || dir == 6 ) 
			return;

		dir = dc.getDir() - dir;
		world.addLight( dc.getPos() , light.getColor(), dir );
	}

	template<>
	void DeviceFun::effect< DC_STARBURST >( Device& dc , World& world , LightTrace const& light )
	{
		for(int i= (light.getDir()+1)%2 ; i < NumDir ; i+=2 )
			world.addLight( dc.getPos() , light.getColor(), Dir::ValueNoCheck( i ) );
	}

	template<>
	void DeviceFun::effect< DC_COMPLEMENTOR >( Device& dc , World& world , LightTrace const& light )
	{
		Dir dir = calcIncidentDir( dc, light );

		if ( dir % 2 == 1 ) 
			return;

		Tile& data = world.getMapData( dc.getPos() );

		Color color = complementary( data.getReceivedLightColor( light.getDir().inverse() ) );

		Color prevColor = data.getEmittedLightColor( light.getDir() );
		if ( color != prevColor )
		{
			if ( !prevColor )
			{
				world.addLight( dc.getPos() , color, light.getDir() );
				//dc.removeFlagBit( FB_BLOCK_EFFECT );
			}
			else if ( dc.getFlag() & DFB_LAZY_EFFECT )
			{
				dc.getFlag().addBits( DFB_SHOT_DOWN );
				world.notifyStatus( TSS_LOGIC_ERROR );
			}
			else
			{
				dc.getFlag().addBits( DFB_LAZY_EFFECT );
				Dir invDir = light.getDir().inverse();
				data.setLazyLightColor( invDir , data.getReceivedLightColor( invDir ) );
				world.notifyStatus( TSS_RECALC );
			}

		}
	}

	template<>
	void DeviceFun::effect< DC_QUADBENDER >( Device& dc , World& world , LightTrace const& light )
	{
		Dir dir = calcIncidentDir( dc, light );
		//int dout=2*m_dir-arcdir+1;
		dir = dc.getDir() - dir + Dir(1);
		world.addLight( dc.getPos() , light.getColor() , dir );
	}

	template<>
	void DeviceFun::effect< DC_LOGICGATE_AND >( Device& dc , World& world , LightTrace const& light )
	{
		Dir dir = calcIncidentDir( dc , light );
		if ( dir != 1 && dir != 7 )
			return;

		Tile& data = world.getMapData( dc.getPos() );

		Color colorT = data.getReceivedLightColor( dc.getDir()+Dir(1) );
		Color colorD = data.getReceivedLightColor( dc.getDir()+Dir(-1) );

		Color color = Color( colorT & colorD );
		if ( color )
		{
			world.addLight( dc.getPos() , color , dc.getDir().inverse() );
		}
	}

	template<>
	void DeviceFun::effect< DC_LOGICGATE_AND_PRIMARY >( Device& dc , World& world , LightTrace const& light )
	{
		Dir dir = calcIncidentDir( dc , light );
		if ( dir != 1 && dir != 7 )
			return;

		Tile& data = world.getMapData( dc.getPos() );

		Color colorT = data.getReceivedLightColor( dc.getDir()+Dir(1) );
		Color colorD = data.getReceivedLightColor( dc.getDir()+Dir(-1) );

		Color color = Color( colorT & colorD );
		if ( color == COLOR_R || color == COLOR_G || color == COLOR_B )
		{
			world.addLight( dc.getPos() , color , dc.getDir().inverse() );
		}
	}

	template<>
	void DeviceFun::effect< DC_LOGICGATE_OR >( Device& dc , World& world , LightTrace const& light )
	{
		Dir dir = calcIncidentDir( dc , light );
		if ( dir != 1 && dir != 7 )
			return;

		Tile& data = world.getMapData( dc.getPos() );

		Color colorT = data.getReceivedLightColor( dc.getDir() + Dir(1) );
		Color colorD = data.getReceivedLightColor( dc.getDir() + Dir(-1) );

		Color color = Color( colorT | colorD );
		if ( color )
		{
			world.addLight( dc.getPos() , color , dc.getDir().inverse() );
		}
	}

#define BEGIN_DC_INFO()\
	void initDeviceInfo(){ \

#define DC_INFO( id , flag ) \
	gDeviceInfo[ id ] = DeviceInfo( id , flag , &DeviceFun::effect< id > , &DeviceFun::update< id > , &DeviceFun::check< id > );

#define END_DC_INFO() }

	BEGIN_DC_INFO()
		DC_INFO( DC_LIGHTSOURCE  , DFB_STATIC | DFB_USE_LOCKED_COLOR )
		DC_INFO( DC_PINWHEEL     , DFB_STATIC | DFB_UNROTATABLE | DFB_DRAW_FRIST | DFB_USE_LOCKED_COLOR )
		DC_INFO( DC_TWISTER      , DFB_UNROTATABLE )
		DC_INFO( DC_SMIRROR      , 0 )
		DC_INFO( DC_SPECTROSCOPE , DFB_DRAW_FRIST )
		DC_INFO( DC_SMIRROR_45   , 0 )
		DC_INFO( DC_PRISM        , DFB_DRAW_FRIST )
		DC_INFO( DC_FILTER       , DFB_USE_LOCKED_COLOR )
		DC_INFO( DC_DOPPLER      , 0 )
		DC_INFO( DC_QTANGLER     , DFB_REMOVE_QE )
		DC_INFO( DC_TELEPORTER   , DFB_UNROTATABLE )
		DC_INFO( DC_MULTIFILTER  , 0 )
		DC_INFO( DC_DUALREFLECTOR, DFB_DRAW_FRIST )
		DC_INFO( DC_CONDUITS     , DFB_STATIC | DFB_DRAW_FRIST )
		DC_INFO( DC_STARBURST    , DFB_REMOVE_QE | DFB_UNROTATABLE )
		DC_INFO( DC_COMPLEMENTOR , DFB_REMOVE_QE )
		DC_INFO( DC_QUADBENDER   , DFB_DRAW_FRIST )
		DC_INFO( DC_LOGICGATE_AND , DFB_REMOVE_QE )
		DC_INFO( DC_LOGICGATE_AND_PRIMARY , DFB_REMOVE_QE )
		DC_INFO( DC_LOGICGATE_OR  , DFB_REMOVE_QE )
		DC_INFO( DC_QUANROTATOR   , 0 )
	END_DC_INFO()


}//namespace Chromatron
