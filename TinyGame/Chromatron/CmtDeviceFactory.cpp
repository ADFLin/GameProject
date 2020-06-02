#include "CmtPCH.h"
#include "CmtDeviceFactory.h"

#include "CmtDeviceID.h"
#include "CmtWorld.h"
#include "CmtLightTrace.h"

#include "BitUtility.h"

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


	Device* DeviceFactory::Create( DeviceId id , Dir dir, Color color )
	{
		assert( 0 <= id && id < DC_DEVICE_NUM );
		return new Device( GetInfo( id ) , dir , color );
	}

	DeviceInfo const& DeviceFactory::GetInfo( DeviceId id )
	{
		return gDeviceInfo[ id ];
	}

	void DeviceFactory::Destroy( Device* dc )
	{
		delete dc;
	}

	static int const QSpinUp = 1;
	static int const QSpinDown = -1;

	struct DeviceFun
	{
		static Dir IncidentDir( Device const& dc , LightTrace const& light )
		{
			return light.getDir().inverse() - dc.getDir();
		}

		static Color DopplerColor( Color color , bool beSameDir )
		{
			if ( beSameDir )
				return Color( ( color >> 1 ) | ( ( color & COLOR_R ) << 2 ) );
			else
				return Color( ( ( color & ( COLOR_G | COLOR_R ) ) << 1 ) | ( ( color & COLOR_B ) >> 2 ) );
		}

		static Dir Rotate(Dir dir , bool bCW)
		{
			if( bCW )
				return dir + Dir::ValueChecked(1);
			else
				return dir - Dir::ValueChecked(1);
		}

		template< DeviceTag ID >
		static void Effect( Device& dc , WorldUpdateContext& context , LightTrace const& light ){}
		template< DeviceTag ID >
		static void Update( Device& dc , WorldUpdateContext& context ){}
		template< DeviceTag ID >
		static bool CheckFinish( Device& dc , WorldUpdateContext& context ){ return true; }
	};

	template<>
	void DeviceFun::Update< DC_LIGHTSOURCE >( Device& dc , WorldUpdateContext& context )
	{
		context.addEffectedLight( dc.getPos() , dc.getColor() , dc.getDir() );
	}

	template<>
	void DeviceFun::Effect< DC_PINWHEEL >( Device& dc , WorldUpdateContext& context , LightTrace const& light )
	{
		context.addEffectedLight( dc.getPos() , light.getColor() , light.getDir() );
	}

	template<>
	bool DeviceFun::CheckFinish< DC_PINWHEEL >( Device& dc , WorldUpdateContext& context )
	{
		Color color = COLOR_NULL;
		Tile const& tile = context.getTile( dc.getPos() );

		Color testColor = dc.getColor();
		Color invColor = COLOR_W & ( ~testColor );

		for(int i = 0 ; i < 4 ;++i )
		{
			Dir dir = Dir::ValueChecked( i );
			Color c1 = tile.getLightPathColor( dir );
			Color c2 = tile.getLightPathColor( dir.inverse() );
			if ( ( c1 & invColor ) || ( c2 & invColor ) )
				return false;

			color |= c1 & c2;
		}
		return color == dc.getColor();
	}

	template<>
	void DeviceFun::Effect< DC_SMIRROR >( Device& dc , WorldUpdateContext& context , LightTrace const& light )
	{
		Dir dir = IncidentDir( dc, light );

		if ( dir >=2 && dir <=6 ) 
			return;
		//int dout=2*m_dir-arcdir;
		dir = dc.getDir() - dir;

		context.addEffectedLight( dc.getPos() ,light.getColor(),dir);
	}

	template<>
	void DeviceFun::Effect< DC_CONDUITS >( Device& dc , WorldUpdateContext& context , LightTrace const& light )
	{
		Dir dir = IncidentDir( dc, light );
		if( dir == 0 || dir == 4 )
		{
			context.addEffectedLight( dc.getPos() , light.getColor(), light.getDir() );
		}
	}

	template<>
	void DeviceFun::Effect< DC_SPECTROSCOPE >( Device& dc , WorldUpdateContext& context , LightTrace const& light )
	{
		Dir dir = IncidentDir( dc, light );

		if ( dir == 2 || dir == 6 ) 
			return;

		dir = dc.getDir() - dir;

		//if ( dir != light.getDir().inverse() )
		context.addEffectedLight( dc.getPos() , light.getColor(), dir );

		context.addEffectedLight( dc.getPos() , light.getColor(), light.getDir() );
	}

	template<>
	void DeviceFun::Effect< DC_SMIRROR_45 >( Device& dc , WorldUpdateContext& context , LightTrace const& light )
	{
		Dir dir = IncidentDir( dc, light );

		if ( dir > 2 && dir <= 6 ) 
			return;

		//int dout=2*m_dir-arcdir+1;
		dir = dc.getDir() - dir + Dir(1);

		context.addEffectedLight( dc.getPos() , light.getColor(),dir);
	}

	template<>
	void DeviceFun::Effect< DC_PRISM >( Device& dc , WorldUpdateContext& context , LightTrace const& light )
	{
		Dir dir = IncidentDir( dc, light );
		Dir invDir = light.getDir().inverse();

		Color color=light.getColor();

		int const outR[] = { -1,-1, 4, 4,-1,-1, 4, 4 };
		int const outG[] = {  3, 5,-1, 5,-1,-1, 3,-1 };
		int const outB[] = {  6, 2,-1, 6,-1,-1, 2,-1 };

		if( ( color & COLOR_R ) && outR[dir] != -1 )
		{
			Dir outDir = invDir + Dir( outR[dir] );
			context.addEffectedLight( dc.getPos() , COLOR_R , outDir );
		}
		if( ( color & COLOR_G ) && outG[dir] != -1 )
		{
			Dir outDir = invDir + Dir( outG[dir] );
			context.addEffectedLight( dc.getPos() , COLOR_G , outDir );
		}
		if( ( color & COLOR_B ) && outB[dir] != -1 )
		{
			Dir outDir = invDir + Dir( outB[dir] );
			context.addEffectedLight( dc.getPos() , COLOR_B , outDir );
		}
	}

	template<>
	void DeviceFun::Effect< DC_FILTER >( Device& dc , WorldUpdateContext& context , LightTrace const& light )
	{
		Dir dir = IncidentDir( dc, light );

		if ( dir!= 0 && dir != 4 ) 
			return;

		Color color = Color( light.getColor() & dc.getColor() );
		if (  color )
		{
			context.addEffectedLight( dc.getPos() , color , light.getDir() );
		}
	}

	template<>
	void DeviceFun::Effect< DC_DOPPLER >( Device& dc , WorldUpdateContext& context , LightTrace const& light )
	{
		Dir dir = IncidentDir( dc, light );

		if ( dir != 0 && dir != 4 )
			return;

		Color color = light.getColor();
		Color outColor = DopplerColor( color , dir == Dir::ValueChecked(0) );

		context.addEffectedLight( dc.getPos() , outColor,light.getDir() );
	}


	template<>
	void DeviceFun::Effect< DC_SPINSPLITTER >(Device& dc, WorldUpdateContext& context, LightTrace const& light)
	{
		Dir dir = IncidentDir(dc, light);

		if( (dir % 2) == 1 )
			return;

		if ( light.getParam() == 0 )
		{
			context.addEffectedLight(dc.getPos(), light.getColor(), light.getDir());
		}
		else
		{
			bool bCW = light.getParam() == QSpinUp;
			if( dir == 2 || dir == 6 )
				bCW = !bCW;

			context.addEffectedLight(dc.getPos(), light.getColor(), Rotate(light.getDir(), bCW));
		}
	}

	class QTangterEffectSolver : public LightSyncProcessor
	{
	public:
		QTangterEffectSolver( WorldUpdateContext& context ):mContext( context ){}
		~QTangterEffectSolver()= default;

		void solve( Vec2i const& pos , LightTrace const& light )
		{
			Color color = light.getColor();

			bool keep = true;
			if ( keep && ( color & COLOR_R ) )
				keep = solveSingleColor( pos , light.getDir() , COLOR_R );
			if ( keep && ( color & COLOR_G ) )
				keep = solveSingleColor( pos , light.getDir() , COLOR_G );
			if ( keep && ( color & COLOR_B ) )
				keep = solveSingleColor( pos , light.getDir() , COLOR_B );
		}

		bool solveSingleColor( Vec2i const& pos , Dir const& dir , Color color )
		{
			assert( FBitUtility::CountSet((uint8)color) == 1 );

			Color receivedColor = mContext.getTile(pos).getReceivedLightColor(dir.inverse());
			//if ( !(receivedColor & color ) )
			{
				mTransmitLights.clear();
				mCheckLights.clear();

				mTransmitLights.emplace_back(pos, color, dir + Dir::ValueChecked(2), QSpinUp);
				mTransmitLights.emplace_back(pos, color, dir - Dir::ValueChecked(2), QSpinDown);

				if( mContext.transmitLightSync(*this, mTransmitLights) != ETransmitStatus::OK )
					return false;
			}

			return true;
		}


		enum EDeivcePassStep
		{
			PrevSpinSplitter,
			SpinSplitter,
			PostSpinSplitter,
			Doppler,
			PosetDoppler,
		};

		bool prevEffectDevice( Device& dc  , LightTrace const& light , int passStep ) override
		{
			bool result = false;

			switch( dc.getId() )
			{
			case DC_DOPPLER:
				if ( passStep == EDeivcePassStep::Doppler )
				{
					Dir dir = DeviceFun::IncidentDir( dc , light );
					if ( dir == 0 || dir == 4 )
					{
						bool isSameDir = ( dir == 0 );
						applyDopplerEffect( light , !isSameDir );
					}
					result = true;
				}
				break;
			case DC_SPINSPLITTER:
				if( passStep == EDeivcePassStep::SpinSplitter )
				{
					Dir dir = DeviceFun::IncidentDir(dc, light);
					if( (dir % 2) == 0 )
					{
						bool bCW = (light.getParam() == QSpinUp);
						if( dir == 2 || dir == 6)
						   bCW = !bCW;
						applySpinSplitterEffect(light, (light.getParam() == QSpinUp ) ? QSpinDown : QSpinUp , bCW );
					}
					result = true;
				}
				break;
			default:
				if ( passStep == EDeivcePassStep::PrevSpinSplitter )
				{
					result = true;
				}
			}


			if ( result )
			{
				if ( dc.getFlag() & DFB_REMOVE_QSTATE )
				{
					mContext.setSyncMode( false );
					mContext.setLightParam( 0 );
				}
				else
				{
					mContext.setSyncMode( true );
					mContext.setLightParam( light.getParam() );
				}
			}

			return result;
		}

		bool prevAddLight( Vec2i const& pos , Color color , Dir dir , int param , int age ) override
		{
			if ( param != QSpinUp && param != QSpinDown )
			{
				for( LightTrace const& light : mCheckLightsNotQState )
				{
					if( light.getStartPos() == pos &&
					    light.getColor() == color &&
					    light.getDir() == dir &&
					    light.getParam() == param )
					{
						return false;
					}	
				}

				LightTrace light( pos , color , dir , param );
				mCheckLightsNotQState.push_back( light );
				return true;
			}

			int  paramAnti = ( param != QSpinUp ) ? QSpinUp : QSpinDown;
			bool haveFound = false;
			bool haveAnti  = false;

			for( LightTrace const& light : mTransmitLights )
			{
				if ( light.getParam() == paramAnti )
				{
					haveAnti = true;
					break;
				}
			}

			for( LightTrace const& light : mCheckLights )
			{
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

		void applyDopplerEffect( LightTrace const& light , bool beSameDir )
		{
			for( auto& lightOther : mTransmitLights )
			{
				if ( lightOther.getParam() != light.getParam() )
				{
					Color outColor = DeviceFun::DopplerColor(lightOther.getColor() , beSameDir );
					lightOther.setColor( outColor );
				}
			}
		}

		void applySpinSplitterEffect(LightTrace const& light, int QState , bool bCW )
		{
			for( auto& lightOther : mTransmitLights )
			{
				if( lightOther.getParam() == QState )
				{
					Dir dir = DeviceFun::Rotate(lightOther.getDir(), bCW);
					lightOther.changeDir(dir);
				}
			}
		}


		WorldUpdateContext& mContext;
		LightList  mTransmitLights;
		LightList  mCheckLights;
		LightList  mCheckLightsNotQState;
	};

	template<>
	void DeviceFun::Effect< DC_QTANGLER >( Device& dc , WorldUpdateContext& context , LightTrace const& light )
	{
		Dir dir = IncidentDir( dc, light );	
		if ( dir != 0 ) 
			return;

		QTangterEffectSolver solver( context );
		solver.solve( dc.getPos() , light );
	}


	template<>
	void DeviceFun::Effect< DC_TELEPORTER >( Device& dc , WorldUpdateContext& context , LightTrace const& light )
	{
		Dir dir = light.getDir();
		Vec2i pos = dc.getPos();

		World& world = context.getWorld();
		for(;;)
		{
			Device* pDC = world.goNextDevice(dir, pos);
			if ( pDC == nullptr )
				break;

			if ( pDC->getId() == DC_TELEPORTER )
			{
				context.addEffectedLight( pDC->getPos() , light.getColor(), dir );
				return;
			}
		}
	}

	template<>
	void DeviceFun::Effect< DC_MULTIFILTER >( Device& dc , WorldUpdateContext& context , LightTrace const& light )
	{
		Dir dir = IncidentDir( dc, light );
		int select = dir % 4;
		static Color const filterColor[] = { COLOR_W , COLOR_R , COLOR_G , COLOR_B };

		Color color = Color( light.getColor() & filterColor[select] );
		if (color) 
			context.addEffectedLight( dc.getPos() ,  color, light.getDir());

	}

	template<>
	void DeviceFun::Effect< DC_TWISTER >( Device& dc , WorldUpdateContext& context , LightTrace const& light )
	{
		Dir turnDir = ( dc.getFlag() & DFB_CLOCKWISE ) ?Dir(-2):Dir(2);
		Dir outDir(light.getDir() + turnDir);

		context.addEffectedLight( dc.getPos() , light.getColor(), outDir );
	}

	template<>
	void DeviceFun::Effect< DC_DUALREFLECTOR >( Device& dc , WorldUpdateContext& context , LightTrace const& light )
	{
		Dir dir = IncidentDir( dc, light );
		if ( dir == 2 || dir == 6 ) 
			return;

		dir = dc.getDir() - dir;
		context.addEffectedLight( dc.getPos() , light.getColor(), dir );
	}

	template<>
	void DeviceFun::Effect< DC_STARBURST >( Device& dc , WorldUpdateContext& context , LightTrace const& light )
	{
		for(int i= (light.getDir()+1)%2 ; i < NumDir ; i+=2 )
			context.addEffectedLight( dc.getPos() , light.getColor(), Dir::ValueChecked( i ) );
	}

	template<>
	void DeviceFun::Effect< DC_COMPLEMENTOR >( Device& dc , WorldUpdateContext& context , LightTrace const& light )
	{
		Dir dir = IncidentDir( dc, light );

		if ( dir % 2 == 1 ) 
			return;

		Tile& tile = context.getTile( dc.getPos() );

		Color color = Complementary( tile.getReceivedLightColor( light.getDir().inverse() ) );

		Color prevColor = tile.getEmittedLightColor( light.getDir() );
		if ( color != prevColor )
		{
			if ( !prevColor )
			{
				context.addEffectedLight( dc.getPos() , color, light.getDir() );
				//dc.removeFlagBit( FB_BLOCK_EFFECT );
			}
			else if ( dc.getFlag() & DFB_LAZY_EFFECT )
			{
				dc.getFlag().addBits( DFB_SHUTDOWN );
				context.notifyStatus( ETransmitStatus::LogicError );
			}
			else
			{
				dc.getFlag().addBits( DFB_LAZY_EFFECT );
				Dir invDir = light.getDir().inverse();
				tile.setLazyLightColor( invDir , tile.getReceivedLightColor( invDir ) );
				context.notifyStatus( ETransmitStatus::RecalcRequired );
			}

		}
	}

	template<>
	void DeviceFun::Effect< DC_QUADBENDER >( Device& dc , WorldUpdateContext& context , LightTrace const& light )
	{
		Dir dir = IncidentDir( dc, light );
		//int dout=2*m_dir-arcdir+1;
		dir = dc.getDir() - dir + Dir(1);
		context.addEffectedLight( dc.getPos() , light.getColor() , dir );
	}

	template<>
	void DeviceFun::Effect< DC_LOGICGATE_AND >( Device& dc , WorldUpdateContext& context , LightTrace const& light )
	{
		Dir dir = IncidentDir( dc , light );
		if ( dir != 1 && dir != 7 )
			return;

		Tile& data = context.getTile( dc.getPos() );

		Color colorT = data.getReceivedLightColor( dc.getDir()+Dir(1) );
		Color colorD = data.getReceivedLightColor( dc.getDir()+Dir(-1) );

		Color color = Color( colorT & colorD );
		if ( color )
		{
			context.addEffectedLight( dc.getPos() , color , dc.getDir().inverse() );
		}
	}

	template<>
	void DeviceFun::Effect< DC_LOGICGATE_AND_PRIMARY >( Device& dc , WorldUpdateContext& context , LightTrace const& light )
	{
		Dir dir = IncidentDir( dc , light );
		if ( dir != 1 && dir != 7 )
			return;

		Tile& data = context.getTile( dc.getPos() );

		Color colorT = data.getReceivedLightColor( dc.getDir()+Dir(1) );
		Color colorD = data.getReceivedLightColor( dc.getDir()+Dir(-1) );

		Color color = Color( colorT & colorD );
		if ( color == COLOR_R || color == COLOR_G || color == COLOR_B )
		{
			context.addEffectedLight( dc.getPos() , color , dc.getDir().inverse() );
		}
	}

	template<>
	void DeviceFun::Effect< DC_LOGICGATE_OR >( Device& dc , WorldUpdateContext& context , LightTrace const& light )
	{
		Dir dir = IncidentDir( dc , light );
		if ( dir != 1 && dir != 7 )
			return;

		Tile& data = context.getTile( dc.getPos() );

		Color colorT = data.getReceivedLightColor( dc.getDir() + Dir(1) );
		Color colorD = data.getReceivedLightColor( dc.getDir() + Dir(-1) );

		Color color = Color( colorT | colorD );
		if ( color )
		{
			context.addEffectedLight( dc.getPos() , color , dc.getDir().inverse() );
		}
	}

#define BEGIN_DC_INFO()\
	void initDeviceInfo(){ \

#define DC_INFO( id , flag ) \
	gDeviceInfo[ id ] = DeviceInfo( id , flag , &DeviceFun::Effect< id > , &DeviceFun::Update< id > , &DeviceFun::CheckFinish< id > );

#define END_DC_INFO() }

	BEGIN_DC_INFO()
		DC_INFO( DC_LIGHTSOURCE  , DFB_STATIC | DFB_USE_LOCKED_COLOR )
		DC_INFO( DC_PINWHEEL     , DFB_STATIC | DFB_UNROTATABLE | DFB_DRAW_FIRST | DFB_USE_LOCKED_COLOR )
		DC_INFO( DC_TWISTER      , DFB_UNROTATABLE )
		DC_INFO( DC_SMIRROR      , 0 )
		DC_INFO( DC_SPECTROSCOPE , DFB_DRAW_FIRST )
		DC_INFO( DC_SMIRROR_45   , 0 )
		DC_INFO( DC_PRISM        , DFB_DRAW_FIRST )
		DC_INFO( DC_FILTER       , DFB_USE_LOCKED_COLOR )
		DC_INFO( DC_DOPPLER      , 0 )
		DC_INFO( DC_QTANGLER     , DFB_REMOVE_QSTATE )
		DC_INFO( DC_TELEPORTER   , DFB_UNROTATABLE )
		DC_INFO( DC_MULTIFILTER  , 0 )
		DC_INFO( DC_DUALREFLECTOR, DFB_DRAW_FIRST )
		DC_INFO( DC_CONDUITS     , DFB_STATIC | DFB_DRAW_FIRST )
		DC_INFO( DC_STARBURST    , DFB_REMOVE_QSTATE | DFB_UNROTATABLE )
		DC_INFO( DC_COMPLEMENTOR , DFB_REMOVE_QSTATE )
		DC_INFO( DC_QUADBENDER   , DFB_DRAW_FIRST )
		DC_INFO( DC_LOGICGATE_AND, DFB_REMOVE_QSTATE )
		DC_INFO( DC_LOGICGATE_AND_PRIMARY , DFB_REMOVE_QSTATE )
		DC_INFO( DC_LOGICGATE_OR , DFB_REMOVE_QSTATE )
		DC_INFO( DC_SPINSPLITTER      , DFB_DRAW_FIRST )
	END_DC_INFO()


#undef BEGIN_DC_INFO
#undef DC_INFO
#undef END_DC_INFO


}//namespace Chromatron
