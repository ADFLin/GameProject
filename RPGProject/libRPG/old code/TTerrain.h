#ifndef TTerrain_h__
#define TTerrain_h__

class TTerrain : public TPhyEntity
{
	DESCRIBLE_ENTITY_CLASS( TTerrain , TPhyEntity )
	TTerrain( )
	{
		m_entityType |= ET_TERRAIN ;
	}

};
#endif // TTerrain_h__