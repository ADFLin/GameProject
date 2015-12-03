#include "ProjectPCH.h"
#include "AbilityProp.h"

//#include "TActorTemplate.h"
//#include "shareDef.h"
#include <algorithm>

template < class T1 , class T2>
static void OpVal( T1& v1 , T2  v2 , unsigned op )
{
	switch ( op )
	{
	case PF_VOP_ADD: v1 = T2(v1) + v2 ;  break;
	case PF_VOP_MUL: v1 = T2(v1) * v2 ;  break;
	case PF_VOP_SUB: v1 = T2(v1) - v2 ;  break;
	case PF_VOP_DIV: v1 = T2(v1) / v2 ;  break;
	}
}

void SAbilityPropData::modifyPropVal( PropModifyInfo& info )
{
	float fVal = getPropValue( info.prop );

	unsigned op = info.flag & PF_VALUE_OPERAOTR;

	OpVal( fVal , info.val , op );
	setPropVal( info.prop , fVal );
}


void SAbilityPropData::setPropVal( PropType prop , PropVal fVal )
{
#define PROP_CASE( mToolProp , propVal )\
	case mToolProp : propVal = fVal; break;

	switch( prop )
	{
	case PROP_HP: HP = fVal ; break;
	case PROP_MP: MP = fVal ; break;
	case PROP_MAX_HP : MaxHP = fVal; break;
	case PROP_MAX_MP : MaxMP = fVal; break;
	case PROP_STR: STR = fVal  ; break;
	case PROP_INT: INT = fVal  ; break;
	case PROP_END: END = fVal  ; break;
	case PROP_DEX: DEX = fVal  ; break;
	case PROP_MAT : MAT = fVal; break;
	case PROP_SAT : SAT = fVal; break;
	case PROP_DT  : DT  = fVal; break;
	PROP_CASE( PROP_VIEW_DIST , viewDist )
	PROP_CASE( PROP_AT_RANGE  , ATRange )
	PROP_CASE( PROP_AT_SPEED , ATSpeed )
	PROP_CASE( PROP_MV_SPEED , MVSpeed )
	PROP_CASE( PROP_KEXP , KExp )
	PROP_CASE( PROP_JUMP_HEIGHT , JumpHeight )
	PROP_CASE( PROP_LEVEL , level )
	}

#undef PROP_CASE
}

PropVal SAbilityPropData::getPropValue( PropType prop ) const
{

#define PROP_CASE( mToolProp , propVal )\
	case mToolProp :  return propVal;

	switch( prop )
	{
	case PROP_HP:  return HP;
	case PROP_MP:  return MP;
	case PROP_MAX_HP : return MaxHP;
	case PROP_MAX_MP : return MaxMP;
	case PROP_STR: return STR;
	case PROP_INT: return INT;
	case PROP_END: return END;
	case PROP_DEX: return DEX;
	case PROP_MAT : return MAT;
	case PROP_SAT : return SAT;
	case PROP_DT : return DT;
	PROP_CASE( PROP_VIEW_DIST , viewDist )
	PROP_CASE( PROP_AT_RANGE  , ATRange )
	PROP_CASE( PROP_AT_SPEED , ATSpeed )
	PROP_CASE( PROP_MV_SPEED , MVSpeed )
	PROP_CASE( PROP_KEXP , KExp )
	PROP_CASE( PROP_JUMP_HEIGHT , JumpHeight )
	PROP_CASE( PROP_LEVEL , level )
	}

	assert("Can't get Prop");
	return 0;

#undef PROP_CASE
}

SAbilityPropData::SAbilityPropData( int level , int MaxHP , int MaxMP ,int STR ,int INT ,int DEX ,int END , int KExp ) 
	:level( level ), MaxHP( MaxHP ), MaxMP( MaxMP )
	,STR(STR),INT(INT),DEX(DEX),END(END)
{
	ATSpeed  = 2.0f;
	viewDist = 800.0f;
	ATRange  = 100.0f;
	MVSpeed  = 300.0f;
	JumpHeight = 100.0f;
	MAT = 0;
	SAT = 0;
	DT  = 0;
	HP = MaxHP;
	MP = MaxMP;
}



PropModifyInfo::PropModifyInfo()
{
	time  = 0;
	state = AS_NORMAL;
}

AbilityProp::AbilityProp( SAbilityPropData& restData ) 
:mRestData( restData )
{
	resetAllPropVal();
}

void AbilityProp::addPropModify( PropModifyInfo& info )
{
	if ( info.time <= 0.0f )
	{
		baseProp.modifyPropVal( info );

		if ( info.prop == PROP_HP || 
			 info.prop == PROP_MP )
			 updatePropValue( info.prop);
	}
	else
	{
		m_PMList.push_back( info );
		m_PMList.back().time += g_GlobalVal.curtime;
		updatePropValue( info.prop );
	}
}

float AbilityProp::computeAdditionalVal( PropType prop )
{
	float baseVal = baseProp.getPropValue( prop );

	float modfiyVal = baseVal;

	for ( PropModifyList::iterator iter = m_PMList.begin();
		iter != m_PMList.end() ; ++iter )
	{
		if ( iter->prop == prop )
		{
			OpVal( modfiyVal , iter->val , iter->flag );
		}
	}
	return modfiyVal - baseVal;
}

void AbilityProp::resetAllPropVal()
{
	//TRoleData* data = TRoleManager::instance().getRoleData( m_roleID );
	baseProp = mRestData;

	for ( int i = 0 ; i < PROP_TYPE_NUM ; ++i )
		addProp.setPropVal( PropType(i) , 0 );
}

void AbilityProp::resetPropVal( PropType prop )
{
	float fVal = mRestData.getPropValue( prop );

	baseProp.setPropVal( prop , fVal );
	addProp.setPropVal( prop , 0 );
}

void AbilityProp::updateFrame( PropModifyCallBack& callback )
{
	int ch = 0;

	for( PropModifyIter iter = m_PMList.begin();
		iter!= m_PMList.end() ; )
	{
		if ( iter->time  < g_GlobalVal.curtime )
		{
			ch |= ( 1 <<iter->prop );
			callback.process( *iter , true );
			iter = m_PMList.erase( iter );
		}
		else
		{
			callback.process( *iter , false );
			++iter;
		}
	}

	for ( int i = 0 ; i < PROP_TYPE_NUM ; ++i )
	{
		if ( ch & (1 << i ) )
			updatePropValue( PropType(i) );
	}
}

void AbilityProp::updatePropValue( PropType prop )
{
	float fVal;
	switch( prop )
	{
	case PROP_HP:
		fVal = TMin( baseProp.getPropValue(PROP_HP) , getPropValue( PROP_MAX_HP) );
		baseProp.setPropVal( PROP_HP , fVal );
		break;
	case PROP_MP:
		fVal = TMin( baseProp.getPropValue(PROP_MP) , getPropValue( PROP_MAX_MP) );
		baseProp.setPropVal( PROP_MP , fVal );
		break;
	case PROP_MAX_HP:
		fVal = computeAdditionalVal( prop );
		addProp.setPropVal( prop , fVal);
		updatePropValue( PROP_HP );
		break;
	case PROP_MAX_MP:
		fVal = computeAdditionalVal( prop );
		addProp.setPropVal( prop , fVal);
		updatePropValue( PROP_MP );
		break;
	default:
		fVal = computeAdditionalVal( prop );
		addProp.setPropVal( prop , fVal);
	}
}

void AbilityProp::setHP( int val )
{
	baseProp.HP = TMin( val , int( getPropValue( PROP_MAX_HP )) );
}

void AbilityProp::setMP( int val )
{
	baseProp.MP = TMin( val , int( getPropValue( PROP_MAX_HP )) );
}
