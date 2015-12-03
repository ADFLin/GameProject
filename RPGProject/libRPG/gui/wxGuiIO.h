#ifndef wxGuiIO_h__
#define wxGuiIO_h__

#include "GuiIO.h"
#include "wx/wx.h"
#include <string>



namespace GuiIO {

class wxPlatform
{
public:
	GUI_INLINE void output( wxChoice* gui , int val ){  gui->SetSelection( val );  }
	GUI_INLINE void output( wxTextCtrl* gui , int val ){  gui->SetValue( wxString() << val );  }
	GUI_INLINE void output( wxTextCtrl* gui , std::string const& val ){  gui->SetValue( val );  }
	GUI_INLINE void output( wxTextCtrl* gui , float val ){  gui->SetValue( wxString() << val );  }
	GUI_INLINE void output( wxSpinCtrl* gui , int val ){  gui->SetValue( val );}
	GUI_INLINE void output( wxCheckBox* gui , bool val ){ gui->SetValue( val ); }
	GUI_INLINE void output( wxBitmapComboBox* gui , unsigned& val ){  gui->SetSelection( val );  }

	GUI_INLINE void input( wxChoice* gui , int& val ){  val = gui->GetSelection();  }
	GUI_INLINE void input( wxTextCtrl* gui , int& val )
	{
		long temp;
		gui->GetValue().ToLong( &temp );
		val = temp;
	}
	GUI_INLINE void input( wxTextCtrl* gui , std::string& val ){  val = gui->GetValue();  }
	GUI_INLINE void input( wxTextCtrl* gui , float& val )
	{
		double temp;
		gui->GetValue().ToDouble( &temp );
		val = temp;
	}
	GUI_INLINE void input( wxSpinCtrl* gui , int& val ){  val = gui->GetValue();  }
	GUI_INLINE void input( wxCheckBox* gui , bool& val ){  val = gui->GetValue();  }
	GUI_INLINE void input( wxBitmapComboBox* gui , unsigned& val ){  val = gui->GetSelection();  }

};


typedef OutputEval< wxPlatform > wxOutputEval;
typedef InputEval< wxPlatform > wxInputEval;


} //namespace GuiIO 

#endif // wxGuiIO_h__
