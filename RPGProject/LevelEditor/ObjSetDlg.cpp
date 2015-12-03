#include "ObjSetDlg.h"

#include "WorldEdit.h"


BEGIN_EVENT_TABLE( ObjSetDlg, ObjSetDlgUI )
//EVT_BUTTON( ID_ProGrid  , ObjSetFrame::OnPGButtonClick )
EVT_PG_CHANGED( ID_ProGrid , ObjSetDlg::OnPGChange )
END_EVENT_TABLE()


static wxChar const* FlyObjTypeEnumStr[]=
{
	wxT("EOT_TERRAIN"),
	wxT("FOT_STATIC_OBJ"),
	wxT("FOT_OTHER"),
	NULL
};

ObjSetDlg::ObjSetDlg( wxWindow* parent , wxWindowID id ) 
	:ObjSetDlgUI( parent , id )
{
	wxPGId pgid;

	m_ObjPropGrid->SetMarginColour(wxColour(200,255,255) );

	pgid = m_ObjPropGrid->AppendCategory( wxT("aa"));
	pgid = m_ObjPropGrid->Append( Vec3DProperty(wxT("Position"),wxPG_LABEL, Vec3D(0,0,0) ) );
	pgid = m_ObjPropGrid->Append( Vec3DProperty(wxT("frontDir"),wxPG_LABEL, Vec3D(0,0,0) ) );
	pgid = m_ObjPropGrid->Append( Vec3DProperty(wxT("upDir"),wxPG_LABEL, Vec3D(0,0,0) ) );

	pgid = m_ObjPropGrid->AppendCategory( wxT("LevelObj"));

	wxArrayInt FlyObjTypeArray;
	FlyObjTypeArray.push_back( EOT_TERRAIN );
	FlyObjTypeArray.push_back( EOT_FLY_OBJ );
	FlyObjTypeArray.push_back( EOT_OTHER );
	pgid = m_ObjPropGrid->Append( wxEnumProperty(wxT("type") , wxPG_LABEL, 
		wxArrayString( 3 , FlyObjTypeEnumStr ) , FlyObjTypeArray ) );

	m_ObjPropGrid->SetPropertyAttributeAll(wxPG_BOOL_USE_CHECKBOX,(long)1);

}

void ObjSetDlg::OnPGChange( wxPropertyGridEvent & event )
{
	wxPGId id = event.GetProperty();
	wxPGVariant val = id.GetProperty().DoGetValue();

	wxString const& name = event.GetPropertyName();

	FnObject& obj = g_WorldEdit->getObjEdit().getSelectObj();

	if ( name == wxT("Position") )
	{
		obj.SetPosition( (float*) val.GetVoidPtr() );
	}
	else if ( name == wxT("frontDir") )
	{
		obj.SetDirection( (float*) val.GetVoidPtr() , NULL );
		updateProGrid();
	}
	else if ( name == wxT("upDir") )
	{
		obj.SetDirection( NULL , (float*) val.GetVoidPtr() );
		updateProGrid();
	}
	else if ( name == wxT("type") )
	{
		int index = g_WorldEdit->getObjEdit().getSelectIndex();
		if ( index != -1 )
		{
			g_editDataVec[ index ].type = (EditObjType) val.GetLong();
		}
	}

}

void ObjSetDlg::updateProGrid()
{
	FnObject& obj = g_WorldEdit->getObjEdit().getSelectObj();
	int index = g_WorldEdit->getObjEdit().getSelectIndex();

	if ( index == -1 ) 
		return;

	wxPGId id;
	Vec3D pos, front , up;
	obj.GetPosition( pos );
	obj.GetDirection( front , up );
	id = m_ObjPropGrid->GetPropertyByName ( wxT("Position") );
	id.GetProperty().DoSetValue( (void*)pos );

	id = m_ObjPropGrid->GetPropertyByName ( wxT("frontDir") );
	id.GetProperty().DoSetValue( (void*)front );
	id = m_ObjPropGrid->GetPropertyByName ( wxT("upDir") );
	id.GetProperty().DoSetValue( (void*)up );

	EditData& data = g_editDataVec[ index ];

	m_ObjPropGrid->SetPropertyValue( wxT("type") , int( data.type) );
	Refresh();
}

WX_PG_IMPLEMENT_VALUE_TYPE_VOIDP( Vec3D , Vec3DProperty, Vec3D(0,0,0) )
WX_PG_IMPLEMENT_PROPERTY_CLASS( Vec3DProperty , wxBaseParentProperty, Vec3D , const Vec3D& , TextCtrl )

Vec3DPropertyClass::Vec3DPropertyClass( const wxString& label,
									    const wxString& name,
									    const Vec3D&  val )
	:wxPGPropertyWithChildren(label,name)
{
	wxPG_INIT_REQUIRED_TYPE(Vec3D)
	DoSetValue((void*)&val );
	AddChild( wxFloatProperty(wxT("X"),wxPG_LABEL,val.x()) );
	AddChild( wxFloatProperty(wxT("Y"),wxPG_LABEL,val.y()) );
	AddChild( wxFloatProperty(wxT("Z"),wxPG_LABEL,val.z()) );
}

Vec3DPropertyClass::~Vec3DPropertyClass() { }

void Vec3DPropertyClass::DoSetValue( wxPGVariant val )
{
	Vec3D* pObj = (Vec3D*) val.GetVoidPtr();
	m_value = *pObj;
	RefreshChildren();
}

wxPGVariant Vec3DPropertyClass::DoGetValue() const
{
	return wxPGVariant((void*)&m_value);
}

void Vec3DPropertyClass::RefreshChildren()
{
	if ( !GetCount() ) return;
	Item(0)->DoSetValue( m_value.x() );
	Item(1)->DoSetValue( m_value.y() );
	Item(2)->DoSetValue( m_value.z() );

}

void Vec3DPropertyClass::ChildChanged( wxPGProperty* p )
{
	switch ( p->GetIndexInParent() )
	{
	case 0: m_value.setValue( p->DoGetValue().GetDouble() , m_value.y() , m_value.z() ); break;
	case 1: m_value.setValue( m_value.x(),  p->DoGetValue().GetDouble() , m_value.z() ); break;
	case 2: m_value.setValue( m_value.x(),  m_value.y() , p->DoGetValue().GetDouble() ); break;
	}
}