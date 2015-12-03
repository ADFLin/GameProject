#ifndef wxPropIO_h__
#define wxPropIO_h__

#include "common.h"

#include <wx/wx.h>
#include <wx/propgrid/propgrid.h>
#include <wx/propgrid/propdev.h>
#include <wx/propgrid/advprops.h>


#include "TPorpLoader.h"

WX_PG_DECLARE_VALUE_TYPE_VOIDP( Vec3D )
WX_PG_DECLARE_PROPERTY( wxVec3DProp ,const Vec3D& , Vec3D(0,0,0) )

class wxVec3DPropClass : public wxPGPropertyWithChildren
{
	WX_PG_DECLARE_PROPERTY_CLASS()
public:
	wxVec3DPropClass( const wxString& label,const wxString& name,const Vec3D& val = Vec3D(0,0,0));
	virtual ~wxVec3DPropClass ();

	WX_PG_DECLARE_PARENTAL_TYPE_METHODS()
	WX_PG_DECLARE_PARENTAL_METHODS()

protected:
	Vec3D    m_value;
};


class wxVType
{
public:
	wxVType( wxPGProperty& prop )
		:m_prop( prop )
		,m_vari( prop.DoGetValue() )
	{
	}

	template < class T>
	void setVal( T  val ){ m_prop.DoSetValue( val ); }
	void setVal( Vec3D& val ){ m_prop.DoSetValue( ( void*)&val ); }
	void setVal( bool  val ){ m_prop.DoSetValue( (int)val ); }
	void setVal( unsigned  val ){ m_prop.DoSetValue( (int)val ); }
	void setVal( String const& val )
	{
		wxString str( val.c_str() );
		m_prop.DoSetValue( str ); 
	}
	
	void getVal( String& val){ val = m_vari.GetString().c_str(); }
	void getVal( float&  val){ val = m_vari.GetDouble(); }
	void getVal( bool& val ){ val = m_vari.GetBool(); }
	void getVal( int& val ){ val = m_vari.GetLong(); }
	void getVal( short& val ){ val = m_vari.GetLong(); }
	void getVal( unsigned& val ){ val = m_vari.GetLong(); }
	void getVal( Vec3D& val )
	{
		float* ptr = (float*) m_vari.GetVoidPtr();
		val.setValue( ptr[0] , ptr[1] , ptr[2] );
	}


	wxPGProperty& m_prop;
	wxPGVariant   m_vari;
};


using namespace PropSpace;

class wxPropIO
{
public:
	typedef wxPGId  PropIDType;
	typedef wxVType VType;
	template < class T >
	static wxPGProperty* getPGProp( wxString const& name , T );

	static wxPGProperty* getPGProp( wxString const& name , short* )
	{  return wxIntProperty( name , wxPG_LABEL );  }
	static wxPGProperty* getPGProp( wxString const& name , bool* )
	{  return wxBoolProperty( name , wxPG_LABEL );  }
	static wxPGProperty* getPGProp( wxString const& name , int*  )
	{  return wxIntProperty( name , wxPG_LABEL );  }
	static wxPGProperty* getPGProp( wxString const& name , unsigned*  )
	{  return wxUIntProperty( name , wxPG_LABEL );  }
	static wxPGProperty* getPGProp( wxString const& name , float*  )
	{  return wxFloatProperty( name , wxPG_LABEL );  }
	static wxPGProperty* getPGProp( wxString const& name , String* )
	{  return wxStringProperty( name , wxPG_LABEL );  }
	static wxPGProperty* getPGProp( wxString const& name , Vec3D* )
	{  return wxVec3DProp( name , wxPG_LABEL );  }

	void getVal( wxPGId& id , TPropBase<VType>* prop );
	void setVal( wxPGId& id , TPropBase<VType>* prop );

	template < class T >
	wxPGId  append( wxString const& name , T* )
	{
		return m_propGrid->Append( getPGProp( name , (T*)0 ) );
	}

	template < class T >
	wxPGId  append( wxString const& name , NormalTypeTrait< T > )
	{
		return m_propGrid->Append( getPGProp( name , (T*)0 ) );
	}

	template < class T >
	wxPGId  append( wxString const& name , EnumTypeTrait< T > )
	{
		wxArrayString strArray;
		wxArrayInt    intArray;
		size_t size;
		EnumTypeData* data = GET_PROP_ENUM_DATA( T , size ); 
		for( int i = 0 ; i < size ; ++i )
		{
			strArray.push_back( data[i].str );
			intArray.push_back( data[i].val );
		}

		return m_propGrid->Append( 
			wxEnumProperty( name , wxPG_LABEL , strArray , intArray  ) );
	}

	template < class T >
	wxPGId  appendEnumFlag( wxString const& name  , T* )
	{
		wxPGChoices constants;

		size_t size;
		EnumTypeData* data = GET_PROP_ENUM_DATA( T , size ); 
		for( int i = 0 ; i < size ; ++i )
		{
			constants.Add( data[i].str , data[i].val );
		}

		return m_propGrid->Append( 
			wxFlagsProperty(name, wxPG_LABEL, constants, 0 ) );
	}

	void addClass( char const* name )
	{
		m_propGrid->AppendCategory( name );
	}

	void clearProp()
	{
		m_propGrid->Clear();
	}


	wxPropertyGrid* m_propGrid;
};

#endif // wxPropIO_h__