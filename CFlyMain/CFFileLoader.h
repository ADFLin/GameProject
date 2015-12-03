#ifndef CFFileLoader_h__
#define CFFileLoader_h__

#include "CFBase.h"
#include <fstream>

namespace CFly
{

	class BinaryStream
	{
	public:
		typedef std::ios::off_type OffsetType;
		typedef std::ios::seekdir  SeekOrigin;
		typedef std::ios::pos_type PosType;
		BinaryStream( std::ifstream& fs ):mFS( fs ){}
		template< class T >
		T       read(){ T out; mFS.read( (char*)&out , sizeof( out ) ); return out; }
		template< class T >
		void    read( T& value ){ mFS.read( (char*)&value , sizeof( value ) ); }

		template< class T , int N >
		void    readArray( T (&value)[ N ] ){ mFS.read( (char*)&value , sizeof( T ) * N ); }
		template< class T >
		void    readArray( T* pValue , int num ){ mFS.read( (char*)pValue , sizeof( T ) * num ); }
		int     readCString( char* buf , int maxSize );
		
		int     getLength();
		PosType getPosition(){ return mFS.tellg(); }
		void    seek( OffsetType offset , SeekOrigin origin ){ mFS.seekg( offset , origin ); }
		void    seek( PosType pos ){ mFS.seekg( pos ); }
		std::ifstream& mFS; 
	};


	class ILoadListener
	{
	public:
		virtual void onLoadObject( Object* object ){}
	};

	class IFileImport
	{
	public:
		virtual void deleteThis() = 0;
		virtual bool load( char const* path , Object* object , ILoadListener* listener ){ return false; }
		virtual bool load( char const* path , Scene* scene , ILoadListener* listener ){ return false; }
		virtual bool load( char const* path , Actor* actor , ILoadListener* listener ){ return false; }
	};

	class IFileExport
	{
	public:
		virtual void deleteThis() = 0;
		virtual bool save( char const* path , Object* object ){ return false; }
		virtual bool save( char const* path , Scene* scene  ){ return false; }
		virtual bool save( char const* path , Actor* actor ){ return false; }
	};

	class IFileLinker
	{
	public:
		virtual void deleteThis() = 0;
		virtual bool canLoad( EntityType type , char const* subFileName ) = 0;
		virtual IFileImport* createLoader() = 0;
	};
}
#endif // CFFileLoader_h__