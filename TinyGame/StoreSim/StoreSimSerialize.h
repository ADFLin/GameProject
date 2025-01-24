#pragma once
#ifndef StoreSimSerialize_H_FC694783_8C34_4ED1_B737_EF80FF885FBC
#define StoreSimSerialize_H_FC694783_8C34_4ED1_B737_EF80FF885FBC

#include "StoreSimCore.h"
#include "Serialize/DataStream.h"

namespace StoreSim
{
	namespace ELevelSaveVersion
	{
		enum
		{
			InitVersion = 0,

			//-----------------------
			LastVersionPlusOne,
			LastVersion = LastVersionPlusOne - 1,
		};
	}

	class IArchive
	{
	public:
		IArchive(IStreamSerializer& serializer, bool bLoading)
			:mSerializer(serializer)
			,mbLoading(bLoading)
		{

		}

		bool isLoading() { return mbLoading; }
		bool isSaving() { return !mbLoading; }


		template< typename T >
		IArchive& operator & (T& value)
		{
			if (isLoading())
			{
				mSerializer >> value;
			}
			else
			{
				mSerializer << value;
			}
			return *this;
		}


		bool mbLoading;
		IStreamSerializer& mSerializer;
	};


}
#endif // StoreSimSerialize_H_FC694783_8C34_4ED1_B737_EF80FF885FBC
