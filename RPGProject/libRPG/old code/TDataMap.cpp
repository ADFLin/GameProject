#include "TDataMap.h"

void saveDataMap( dataFileIO& file , dataMap_t& dataMap , void* ptr )
{
	if ( dataMap.baseDataMap )
		saveDataMap( file , *dataMap.baseDataMap , ptr );

	for ( int i = 0 ; i < dataMap.numDataField ; ++i )
	{
		dataFieldBase* df = dataMap.dataFieldArray[i];
		df->save( file , ptr );
	}
}

void loadDataMap( dataFileIO& file , dataMap_t& dataMap , void* ptr )
{
	if ( dataMap.baseDataMap )
		loadDataMap( file , *dataMap.baseDataMap , ptr );

	for ( int i = 0 ; i < dataMap.numDataField ; ++i )
	{
		dataFieldBase* df = dataMap.dataFieldArray[i];
		df->load( file , ptr );
	}
}