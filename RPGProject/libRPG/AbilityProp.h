#ifndef AbilityProp_h__
#define AbilityProp_h__

#include "common.h"
#include "CEntity.h"

#include "TAblilityProp.h"
#include <list>

class PropModifyCallBack
{
public:
	virtual void process( PropModifyInfo& info , bool beD ) = 0;
};



struct Var
{
	void setValue( float val ){ floatVal = val; }
	void setValue( int   val ){ intVal = val; }

	int   getInt(){ return intVal; }
	float getFloat(){ return floatVal; }

	union
	{
		int    intVal;
		float  floatVal;
	};
};

class AbilityProp
{
public:
	AbilityProp( SAbilityPropData& restData );

	void  updateFrame( PropModifyCallBack& removeCallback );

	void setHP(int val);
	void setMP(int val);

	void updatePropValue( PropType prop );
	void setBasePropValue( PropType prop , float val )
	{
		assert ( prop != PROP_HP && prop != PROP_MP );
		baseProp.setPropVal( prop , val );
	}
	PropVal getPropValue( PropType prop ) const
	{
		return baseProp.getPropValue( prop ) + addProp.getPropValue( prop );
	}
	PropVal getAdditionPropValue( PropType prop ) const
	{
		return addProp.getPropValue( prop );
	}
	void    addPropModify( PropModifyInfo& info );
	float   computeAdditionalVal( PropType prop );
	bool    removePropModify( PropModifyInfo& info  );

	void    resetPropVal( PropType prop );
	void    resetAllPropVal();


	int  getHP() const { return getPropValue( PROP_HP ); }
	int  getMP() const { return getPropValue( PROP_MP ); }

	void addStateBit( ActorState state )
	{
		m_stateBit |= state;
	}
	unsigned getStateBit(){ return m_stateBit; }
	void     removeStateBit(ActorState state ){ m_stateBit &= (~state); }

private:
	//unsigned m_roleID;
	SAbilityPropData& mRestData;

	SAbilityPropData baseProp;
	SAbilityPropData addProp;

	typedef std::list< PropModifyInfo > PropModifyList;
	typedef PropModifyList::iterator PropModifyIter;
	PropModifyList m_PMList;


	unsigned      m_stateBit;
};

#endif // AbilityProp_h__