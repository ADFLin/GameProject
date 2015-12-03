#include "wxPropIO.h"


WX_PG_IMPLEMENT_VALUE_TYPE_VOIDP( Vec3D , wxVec3DProp , Vec3D(0,0,0) )
WX_PG_IMPLEMENT_PROPERTY_CLASS( wxVec3DProp , wxBaseParentProperty, Vec3D , const Vec3D& , TextCtrl )


wxVec3DPropClass::wxVec3DPropClass(
	const wxString& label, const wxString& name, const Vec3D&  val )
    :wxPGPropertyWithChildren( label , name )
{
	wxPG_INIT_REQUIRED_TYPE(Vec3D);
	DoSetValue((void*)&val );
	AddChild( wxFloatProperty(wxT("X"),wxPG_LABEL,val.x()) );
	AddChild( wxFloatProperty(wxT("Y"),wxPG_LABEL,val.y()) );
	AddChild( wxFloatProperty(wxT("Z"),wxPG_LABEL,val.z()) );
}

wxVec3DPropClass::~wxVec3DPropClass() { }

void wxVec3DPropClass::DoSetValue( wxPGVariant val )
{
	Vec3D* pObj = (Vec3D*) val.GetVoidPtr();
	m_value = *pObj;
	RefreshChildren();
}

wxPGVariant wxVec3DPropClass::DoGetValue() const
{
	return wxPGVariant((void*)&m_value);
}

void wxVec3DPropClass::RefreshChildren()
{
	if ( !GetCount() ) return;
	Item(0)->DoSetValue( m_value.x() );
	Item(1)->DoSetValue( m_value.y() );
	Item(2)->DoSetValue( m_value.z() );

}

void wxVec3DPropClass::ChildChanged( wxPGProperty* p )
{
	switch ( p->GetIndexInParent() )
	{
	case 0: m_value.setValue( p->DoGetValue().GetDouble() , m_value.y() , m_value.z() ); break;
	case 1: m_value.setValue( m_value.x(),  p->DoGetValue().GetDouble() , m_value.z() ); break;
	case 2: m_value.setValue( m_value.x(),  m_value.y() , p->DoGetValue().GetDouble() ); break;
	}
}

void wxPropIO::setVal( wxPGId& id , TPropBase<VType>* prop )
{
	wxVType vtype( id.GetProperty() );
	prop->getVal( vtype );
}

void wxPropIO::getVal( wxPGId& id , TPropBase<VT	ype>* prop )
{
	wxVType type( id.GetProperty() );
	prop->setVal( type );
}