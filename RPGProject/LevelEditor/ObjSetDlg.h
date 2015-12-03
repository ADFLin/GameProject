#include <wx/wx.h>
#include <wx/propgrid/propgrid.h>
#include <wx/propgrid/propdev.h>
#include <wx/propgrid/advprops.h>

#include "ui/ObjSetDlgUI.h"
#include "common.h"


WX_PG_DECLARE_VALUE_TYPE_VOIDP( Vec3D )
WX_PG_DECLARE_PROPERTY( Vec3DProperty,const Vec3D&, Vec3D(0,0,0) )

class Vec3DPropertyClass : public wxPGPropertyWithChildren
{
	WX_PG_DECLARE_PROPERTY_CLASS()
public:
	Vec3DPropertyClass( const wxString& label,const wxString& name,const Vec3D& val = Vec3D(0,0,0));
	virtual ~Vec3DPropertyClass ();

	WX_PG_DECLARE_PARENTAL_TYPE_METHODS()
	WX_PG_DECLARE_PARENTAL_METHODS()

protected:
	Vec3D    m_value;
};



class ObjSetDlg : public ObjSetDlgUI
{
public:
	ObjSetDlg( wxWindow* parent , wxWindowID id = wxID_ANY );
	void OnPGChange( wxPropertyGridEvent & event );
	void updateProGrid();

private:

	DECLARE_EVENT_TABLE()
};

