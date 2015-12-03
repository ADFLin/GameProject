///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Aug 25 2009)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "WorldEditorFrameUI.h"

///////////////////////////////////////////////////////////////////////////

WorldEditorFrameUI::WorldEditorFrameUI( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxFrame( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	m_Sizer = new wxBoxSizer( wxHORIZONTAL );
	
	m_splitter3 = new wxSplitterWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D );
	m_splitter3->Connect( wxEVT_IDLE, wxIdleEventHandler( WorldEditorFrameUI::m_splitter3OnIdle ), NULL, this );
	
	wxPanel* m_panel8;
	m_panel8 = new wxPanel( m_splitter3, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxVERTICAL );
	
	m_Notebook = new wxNotebook( m_panel8, ID_Notebook, wxDefaultPosition, wxSize( 250,-1 ), 0 );
	m_ObjEditPanel = new wxPanel( m_Notebook, ID_ObjEditPanel, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxVERTICAL );
	
	m_splitter4 = new wxSplitterWindow( m_ObjEditPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D );
	m_splitter4->Connect( wxEVT_IDLE, wxIdleEventHandler( WorldEditorFrameUI::m_splitter4OnIdle ), NULL, this );
	
	m_panel10 = new wxPanel( m_splitter4, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer10;
	bSizer10 = new wxBoxSizer( wxVERTICAL );
	
	m_GroupTreeCtrl = new wxTreeCtrl( m_panel10, ID_GroupTreeCtrl, wxDefaultPosition, wxSize( 200,-1 ), wxTR_EDIT_LABELS|wxTR_HAS_BUTTONS|wxTR_HIDE_ROOT|wxTR_LINES_AT_ROOT|wxTR_MULTIPLE|wxTR_ROW_LINES|wxTR_SINGLE );
	bSizer10->Add( m_GroupTreeCtrl, 1, wxALL|wxEXPAND, 0 );
	
	m_panel10->SetSizer( bSizer10 );
	m_panel10->Layout();
	bSizer10->Fit( m_panel10 );
	m_splitter4->Initialize( m_panel10 );
	bSizer6->Add( m_splitter4, 1, wxEXPAND, 5 );
	
	m_ObjEditPanel->SetSizer( bSizer6 );
	m_ObjEditPanel->Layout();
	bSizer6->Fit( m_ObjEditPanel );
	m_Notebook->AddPage( m_ObjEditPanel, wxT("Object Edit"), false );
	m_ModelEditPanel = new wxPanel( m_Notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxVERTICAL );
	
	m_ModelList = new wxListCtrl( m_ModelEditPanel, ID_ModelList, wxDefaultPosition, wxDefaultSize, wxLC_REPORT|wxLC_SINGLE_SEL );
	bSizer3->Add( m_ModelList, 1, wxALL|wxEXPAND, 5 );
	
	m_ModelEditPanel->SetSizer( bSizer3 );
	m_ModelEditPanel->Layout();
	bSizer3->Fit( m_ModelEditPanel );
	m_Notebook->AddPage( m_ModelEditPanel, wxT("Model Edit"), true );
	m_RoleEditPanel = new wxPanel( m_Notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_RoleEditPanel->Hide();
	
	wxBoxSizer* bSizer31;
	bSizer31 = new wxBoxSizer( wxVERTICAL );
	
	m_RoleListCtrl = new wxListCtrl( m_RoleEditPanel, ID_RoleListCtrl, wxDefaultPosition, wxDefaultSize, wxLC_REPORT|wxLC_SINGLE_SEL );
	bSizer31->Add( m_RoleListCtrl, 1, wxALL|wxEXPAND, 5 );
	
	m_RoleEditPanel->SetSizer( bSizer31 );
	m_RoleEditPanel->Layout();
	bSizer31->Fit( m_RoleEditPanel );
	m_Notebook->AddPage( m_RoleEditPanel, wxT("Role Edit"), false );
	
	bSizer7->Add( m_Notebook, 1, wxEXPAND, 5 );
	
	m_panel8->SetSizer( bSizer7 );
	m_panel8->Layout();
	bSizer7->Fit( m_panel8 );
	m_panel102 = new wxPanel( m_splitter3, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer14;
	bSizer14 = new wxBoxSizer( wxVERTICAL );
	
	m_splitter41 = new wxSplitterWindow( m_panel102, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D );
	m_splitter41->SetSashGravity( 1 );
	m_splitter41->Connect( wxEVT_IDLE, wxIdleEventHandler( WorldEditorFrameUI::m_splitter41OnIdle ), NULL, this );
	
	wxPanel* m_panel101;
	m_panel101 = new wxPanel( m_splitter41, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer11;
	bSizer11 = new wxBoxSizer( wxVERTICAL );
	
	m_DrawPanel = new wxPanel( m_panel101, ID_DrawPanel, wxDefaultPosition, wxDefaultSize, wxSTATIC_BORDER|wxTAB_TRAVERSAL );
	bSizer11->Add( m_DrawPanel, 1, wxEXPAND | wxALL, 5 );
	
	wxBoxSizer* bSizer15;
	bSizer15 = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer15->Add( 0, 0, 1, 0, 5 );
	
	m_UpdateButton = new wxButton( m_panel101, ID_UpdateButton, wxT("update Game"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer15->Add( m_UpdateButton, 0, wxALL|wxALIGN_BOTTOM, 5 );
	
	bSizer11->Add( bSizer15, 0, wxEXPAND, 5 );
	
	m_panel101->SetSizer( bSizer11 );
	m_panel101->Layout();
	bSizer11->Fit( m_panel101 );
	m_panel9 = new wxPanel( m_splitter41, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxVERTICAL );
	
	m_ObjPropGrid = new wxPropertyGrid(m_panel9, ID_ObjPropGrid, wxDefaultPosition, wxDefaultSize, wxPG_DEFAULT_STYLE);
	bSizer8->Add( m_ObjPropGrid, 1, wxEXPAND | wxALL, 0 );
	
	m_panel9->SetSizer( bSizer8 );
	m_panel9->Layout();
	bSizer8->Fit( m_panel9 );
	m_splitter41->SplitVertically( m_panel101, m_panel9, -200 );
	bSizer14->Add( m_splitter41, 1, wxEXPAND, 5 );
	
	m_panel102->SetSizer( bSizer14 );
	m_panel102->Layout();
	bSizer14->Fit( m_panel102 );
	m_splitter3->SplitVertically( m_panel8, m_panel102, 200 );
	m_Sizer->Add( m_splitter3, 1, wxEXPAND, 5 );
	
	this->SetSizer( m_Sizer );
	this->Layout();
	
	// Connect Events
	m_GroupTreeCtrl->Connect( wxEVT_COMMAND_TREE_BEGIN_DRAG, wxTreeEventHandler( WorldEditorFrameUI::OnTreeBeginDrag ), NULL, this );
	m_GroupTreeCtrl->Connect( wxEVT_COMMAND_TREE_BEGIN_LABEL_EDIT, wxTreeEventHandler( WorldEditorFrameUI::OnTreeLabelEditBegin ), NULL, this );
	m_GroupTreeCtrl->Connect( wxEVT_COMMAND_TREE_END_DRAG, wxTreeEventHandler( WorldEditorFrameUI::OnTreeEndDrag ), NULL, this );
	m_GroupTreeCtrl->Connect( wxEVT_COMMAND_TREE_END_LABEL_EDIT, wxTreeEventHandler( WorldEditorFrameUI::OnTreeLabelEditEnd ), NULL, this );
	m_GroupTreeCtrl->Connect( wxEVT_COMMAND_TREE_ITEM_ACTIVATED, wxTreeEventHandler( WorldEditorFrameUI::OnTreeActivated ), NULL, this );
	m_GroupTreeCtrl->Connect( wxEVT_COMMAND_TREE_ITEM_COLLAPSING, wxTreeEventHandler( WorldEditorFrameUI::OnTreeCollapsing ), NULL, this );
	m_GroupTreeCtrl->Connect( wxEVT_COMMAND_TREE_ITEM_EXPANDING, wxTreeEventHandler( WorldEditorFrameUI::OnTreeExpanding ), NULL, this );
	m_GroupTreeCtrl->Connect( wxEVT_COMMAND_TREE_ITEM_MENU, wxTreeEventHandler( WorldEditorFrameUI::OnTreeMenu ), NULL, this );
	m_GroupTreeCtrl->Connect( wxEVT_COMMAND_TREE_ITEM_RIGHT_CLICK, wxTreeEventHandler( WorldEditorFrameUI::OnTreeRightClick ), NULL, this );
	m_GroupTreeCtrl->Connect( wxEVT_COMMAND_TREE_SEL_CHANGED, wxTreeEventHandler( WorldEditorFrameUI::OnTreeSelChanged ), NULL, this );
	m_ModelList->Connect( wxEVT_COMMAND_LIST_ITEM_SELECTED, wxListEventHandler( WorldEditorFrameUI::OnModelListSelected ), NULL, this );
	m_ModelList->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( WorldEditorFrameUI::OnModelListRightDown ), NULL, this );
	m_RoleListCtrl->Connect( wxEVT_COMMAND_LIST_ITEM_SELECTED, wxListEventHandler( WorldEditorFrameUI::OnRoleListSelected ), NULL, this );
	m_RoleListCtrl->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( WorldEditorFrameUI::OnRoleListRightDown ), NULL, this );
	m_DrawPanel->Connect( wxEVT_ERASE_BACKGROUND, wxEraseEventHandler( WorldEditorFrameUI::OnDraw ), NULL, this );
	m_DrawPanel->Connect( wxEVT_KEY_DOWN, wxKeyEventHandler( WorldEditorFrameUI::OnKeyDown ), NULL, this );
	m_DrawPanel->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( WorldEditorFrameUI::OnMouse ), NULL, this );
	m_DrawPanel->Connect( wxEVT_LEFT_UP, wxMouseEventHandler( WorldEditorFrameUI::OnMouse ), NULL, this );
	m_DrawPanel->Connect( wxEVT_MIDDLE_DOWN, wxMouseEventHandler( WorldEditorFrameUI::OnMouse ), NULL, this );
	m_DrawPanel->Connect( wxEVT_MIDDLE_UP, wxMouseEventHandler( WorldEditorFrameUI::OnMouse ), NULL, this );
	m_DrawPanel->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( WorldEditorFrameUI::OnMouse ), NULL, this );
	m_DrawPanel->Connect( wxEVT_RIGHT_UP, wxMouseEventHandler( WorldEditorFrameUI::OnMouse ), NULL, this );
	m_DrawPanel->Connect( wxEVT_MOTION, wxMouseEventHandler( WorldEditorFrameUI::OnMouse ), NULL, this );
	m_DrawPanel->Connect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( WorldEditorFrameUI::OnMouse ), NULL, this );
	m_DrawPanel->Connect( wxEVT_MIDDLE_DCLICK, wxMouseEventHandler( WorldEditorFrameUI::OnMouse ), NULL, this );
	m_DrawPanel->Connect( wxEVT_RIGHT_DCLICK, wxMouseEventHandler( WorldEditorFrameUI::OnMouse ), NULL, this );
	m_DrawPanel->Connect( wxEVT_LEAVE_WINDOW, wxMouseEventHandler( WorldEditorFrameUI::OnMouse ), NULL, this );
	m_DrawPanel->Connect( wxEVT_ENTER_WINDOW, wxMouseEventHandler( WorldEditorFrameUI::OnMouse ), NULL, this );
	m_DrawPanel->Connect( wxEVT_MOUSEWHEEL, wxMouseEventHandler( WorldEditorFrameUI::OnMouse ), NULL, this );
	m_DrawPanel->Connect( wxEVT_SIZE, wxSizeEventHandler( WorldEditorFrameUI::OnDrawSize ), NULL, this );
	m_UpdateButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( WorldEditorFrameUI::OnUpdateButton ), NULL, this );
}

WorldEditorFrameUI::~WorldEditorFrameUI()
{
	// Disconnect Events
	m_GroupTreeCtrl->Disconnect( wxEVT_COMMAND_TREE_BEGIN_DRAG, wxTreeEventHandler( WorldEditorFrameUI::OnTreeBeginDrag ), NULL, this );
	m_GroupTreeCtrl->Disconnect( wxEVT_COMMAND_TREE_BEGIN_LABEL_EDIT, wxTreeEventHandler( WorldEditorFrameUI::OnTreeLabelEditBegin ), NULL, this );
	m_GroupTreeCtrl->Disconnect( wxEVT_COMMAND_TREE_END_DRAG, wxTreeEventHandler( WorldEditorFrameUI::OnTreeEndDrag ), NULL, this );
	m_GroupTreeCtrl->Disconnect( wxEVT_COMMAND_TREE_END_LABEL_EDIT, wxTreeEventHandler( WorldEditorFrameUI::OnTreeLabelEditEnd ), NULL, this );
	m_GroupTreeCtrl->Disconnect( wxEVT_COMMAND_TREE_ITEM_ACTIVATED, wxTreeEventHandler( WorldEditorFrameUI::OnTreeActivated ), NULL, this );
	m_GroupTreeCtrl->Disconnect( wxEVT_COMMAND_TREE_ITEM_COLLAPSING, wxTreeEventHandler( WorldEditorFrameUI::OnTreeCollapsing ), NULL, this );
	m_GroupTreeCtrl->Disconnect( wxEVT_COMMAND_TREE_ITEM_EXPANDING, wxTreeEventHandler( WorldEditorFrameUI::OnTreeExpanding ), NULL, this );
	m_GroupTreeCtrl->Disconnect( wxEVT_COMMAND_TREE_ITEM_MENU, wxTreeEventHandler( WorldEditorFrameUI::OnTreeMenu ), NULL, this );
	m_GroupTreeCtrl->Disconnect( wxEVT_COMMAND_TREE_ITEM_RIGHT_CLICK, wxTreeEventHandler( WorldEditorFrameUI::OnTreeRightClick ), NULL, this );
	m_GroupTreeCtrl->Disconnect( wxEVT_COMMAND_TREE_SEL_CHANGED, wxTreeEventHandler( WorldEditorFrameUI::OnTreeSelChanged ), NULL, this );
	m_ModelList->Disconnect( wxEVT_COMMAND_LIST_ITEM_SELECTED, wxListEventHandler( WorldEditorFrameUI::OnModelListSelected ), NULL, this );
	m_ModelList->Disconnect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( WorldEditorFrameUI::OnModelListRightDown ), NULL, this );
	m_RoleListCtrl->Disconnect( wxEVT_COMMAND_LIST_ITEM_SELECTED, wxListEventHandler( WorldEditorFrameUI::OnRoleListSelected ), NULL, this );
	m_RoleListCtrl->Disconnect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( WorldEditorFrameUI::OnRoleListRightDown ), NULL, this );
	m_DrawPanel->Disconnect( wxEVT_ERASE_BACKGROUND, wxEraseEventHandler( WorldEditorFrameUI::OnDraw ), NULL, this );
	m_DrawPanel->Disconnect( wxEVT_KEY_DOWN, wxKeyEventHandler( WorldEditorFrameUI::OnKeyDown ), NULL, this );
	m_DrawPanel->Disconnect( wxEVT_LEFT_DOWN, wxMouseEventHandler( WorldEditorFrameUI::OnMouse ), NULL, this );
	m_DrawPanel->Disconnect( wxEVT_LEFT_UP, wxMouseEventHandler( WorldEditorFrameUI::OnMouse ), NULL, this );
	m_DrawPanel->Disconnect( wxEVT_MIDDLE_DOWN, wxMouseEventHandler( WorldEditorFrameUI::OnMouse ), NULL, this );
	m_DrawPanel->Disconnect( wxEVT_MIDDLE_UP, wxMouseEventHandler( WorldEditorFrameUI::OnMouse ), NULL, this );
	m_DrawPanel->Disconnect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( WorldEditorFrameUI::OnMouse ), NULL, this );
	m_DrawPanel->Disconnect( wxEVT_RIGHT_UP, wxMouseEventHandler( WorldEditorFrameUI::OnMouse ), NULL, this );
	m_DrawPanel->Disconnect( wxEVT_MOTION, wxMouseEventHandler( WorldEditorFrameUI::OnMouse ), NULL, this );
	m_DrawPanel->Disconnect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( WorldEditorFrameUI::OnMouse ), NULL, this );
	m_DrawPanel->Disconnect( wxEVT_MIDDLE_DCLICK, wxMouseEventHandler( WorldEditorFrameUI::OnMouse ), NULL, this );
	m_DrawPanel->Disconnect( wxEVT_RIGHT_DCLICK, wxMouseEventHandler( WorldEditorFrameUI::OnMouse ), NULL, this );
	m_DrawPanel->Disconnect( wxEVT_LEAVE_WINDOW, wxMouseEventHandler( WorldEditorFrameUI::OnMouse ), NULL, this );
	m_DrawPanel->Disconnect( wxEVT_ENTER_WINDOW, wxMouseEventHandler( WorldEditorFrameUI::OnMouse ), NULL, this );
	m_DrawPanel->Disconnect( wxEVT_MOUSEWHEEL, wxMouseEventHandler( WorldEditorFrameUI::OnMouse ), NULL, this );
	m_DrawPanel->Disconnect( wxEVT_SIZE, wxSizeEventHandler( WorldEditorFrameUI::OnDrawSize ), NULL, this );
	m_UpdateButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( WorldEditorFrameUI::OnUpdateButton ), NULL, this );
}

SignalConDlgUI::SignalConDlgUI( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer10;
	bSizer10 = new wxBoxSizer( wxVERTICAL );
	
	m_ConListCtrl = new wxListCtrl( this, ID_ConListCtrl, wxDefaultPosition, wxDefaultSize, wxLC_REPORT|wxLC_SINGLE_SEL );
	bSizer10->Add( m_ConListCtrl, 1, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* bSizer12;
	bSizer12 = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer11;
	bSizer11 = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbSizer3;
	sbSizer3 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxT("Singnal") ), wxHORIZONTAL );
	
	wxArrayString m_SenderChoiceChoices;
	m_SenderChoice = new wxChoice( this, ID_SenderChoice, wxDefaultPosition, wxDefaultSize, m_SenderChoiceChoices, 0 );
	m_SenderChoice->SetSelection( 0 );
	sbSizer3->Add( m_SenderChoice, 1, wxRIGHT|wxLEFT, 2 );
	
	wxArrayString m_SignalChoiceChoices;
	m_SignalChoice = new wxChoice( this, ID_SignalChoice, wxDefaultPosition, wxDefaultSize, m_SignalChoiceChoices, 0 );
	m_SignalChoice->SetSelection( 0 );
	sbSizer3->Add( m_SignalChoice, 1, wxRIGHT|wxLEFT, 2 );
	
	bSizer11->Add( sbSizer3, 0, wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbSizer4;
	sbSizer4 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxT("Slot") ), wxHORIZONTAL );
	
	wxArrayString m_HolderChoiceChoices;
	m_HolderChoice = new wxChoice( this, ID_HolderChoice, wxDefaultPosition, wxDefaultSize, m_HolderChoiceChoices, 0 );
	m_HolderChoice->SetSelection( 0 );
	sbSizer4->Add( m_HolderChoice, 1, wxRIGHT|wxLEFT, 2 );
	
	wxArrayString m_SlotChoiceChoices;
	m_SlotChoice = new wxChoice( this, ID_SlotChoice, wxDefaultPosition, wxDefaultSize, m_SlotChoiceChoices, 0 );
	m_SlotChoice->SetSelection( 0 );
	sbSizer4->Add( m_SlotChoice, 1, wxRIGHT|wxLEFT, 2 );
	
	bSizer11->Add( sbSizer4, 0, wxEXPAND, 5 );
	
	bSizer12->Add( bSizer11, 1, 0, 5 );
	
	wxBoxSizer* bSizer13;
	bSizer13 = new wxBoxSizer( wxVERTICAL );
	
	m_AddButton = new wxButton( this, ID_AddButton, wxT("Add"), wxDefaultPosition, wxSize( 70,-1 ), 0 );
	bSizer13->Add( m_AddButton, 0, wxALL, 5 );
	
	m_MofidyButton = new wxButton( this, ID_MofidyButton, wxT("Modify"), wxDefaultPosition, wxSize( 70,-1 ), 0 );
	bSizer13->Add( m_MofidyButton, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_RemoveButton = new wxButton( this, ID_RemoveButton, wxT("Remove"), wxDefaultPosition, wxSize( 70,-1 ), 0 );
	bSizer13->Add( m_RemoveButton, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	bSizer12->Add( bSizer13, 0, 0, 5 );
	
	bSizer10->Add( bSizer12, 0, wxEXPAND, 5 );
	
	this->SetSizer( bSizer10 );
	this->Layout();
	
	// Connect Events
	m_ConListCtrl->Connect( wxEVT_COMMAND_LIST_ITEM_DESELECTED, wxListEventHandler( SignalConDlgUI::OnListItemDeselected ), NULL, this );
	m_ConListCtrl->Connect( wxEVT_COMMAND_LIST_ITEM_SELECTED, wxListEventHandler( SignalConDlgUI::OnListItemSelected ), NULL, this );
	m_SenderChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( SignalConDlgUI::OnChoice ), NULL, this );
	m_HolderChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( SignalConDlgUI::OnChoice ), NULL, this );
	m_AddButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SignalConDlgUI::OnAddClick ), NULL, this );
	m_MofidyButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SignalConDlgUI::OnModifyClick ), NULL, this );
	m_RemoveButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SignalConDlgUI::OnRemoveClick ), NULL, this );
}

SignalConDlgUI::~SignalConDlgUI()
{
	// Disconnect Events
	m_ConListCtrl->Disconnect( wxEVT_COMMAND_LIST_ITEM_DESELECTED, wxListEventHandler( SignalConDlgUI::OnListItemDeselected ), NULL, this );
	m_ConListCtrl->Disconnect( wxEVT_COMMAND_LIST_ITEM_SELECTED, wxListEventHandler( SignalConDlgUI::OnListItemSelected ), NULL, this );
	m_SenderChoice->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( SignalConDlgUI::OnChoice ), NULL, this );
	m_HolderChoice->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( SignalConDlgUI::OnChoice ), NULL, this );
	m_AddButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SignalConDlgUI::OnAddClick ), NULL, this );
	m_MofidyButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SignalConDlgUI::OnModifyClick ), NULL, this );
	m_RemoveButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SignalConDlgUI::OnRemoveClick ), NULL, this );
}

TerrainToolDlgUI::TerrainToolDlgUI( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( 310,220 ), wxDefaultSize );
	
	wxBoxSizer* bSizer15;
	bSizer15 = new wxBoxSizer( wxVERTICAL );
	
	wxString m_EffectRadioBoxChoices[] = { wxT("Height"), wxT("Texture"), wxT("Smooth") };
	int m_EffectRadioBoxNChoices = sizeof( m_EffectRadioBoxChoices ) / sizeof( wxString );
	m_EffectRadioBox = new wxRadioBox( this, ID_EffectRadioBox, wxT("Paint Effect"), wxDefaultPosition, wxDefaultSize, m_EffectRadioBoxNChoices, m_EffectRadioBoxChoices, 10, wxRA_SPECIFY_COLS|wxRA_USE_CHECKBOX );
	m_EffectRadioBox->SetSelection( 0 );
	bSizer15->Add( m_EffectRadioBox, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	wxStaticBoxSizer* sbSizer3;
	sbSizer3 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxT("Brush") ), wxVERTICAL );
	
	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 2, 3, 0, 0 );
	fgSizer1->AddGrowableCol( 1 );
	fgSizer1->SetFlexibleDirection( wxHORIZONTAL );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticText1 = new wxStaticText( this, wxID_ANY, wxT("Strength -"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	fgSizer1->Add( m_staticText1, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_StrSlider = new wxSlider( this, ID_StrSlider, 0, 0, 100, wxDefaultPosition, wxSize( -1,-1 ), wxSL_HORIZONTAL|wxSL_SELRANGE );
	fgSizer1->Add( m_StrSlider, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxBOTTOM, 5 );
	
	m_StrTCtrl = new wxTextCtrl( this, ID_StrTCtrl, wxEmptyString, wxDefaultPosition, wxSize( 70,-1 ), wxTE_LEFT|wxTE_PROCESS_ENTER );
	fgSizer1->Add( m_StrTCtrl, 0, wxBOTTOM|wxRIGHT, 5 );
	
	m_staticText12 = new wxStaticText( this, wxID_ANY, wxT("Radius -"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText12->Wrap( -1 );
	fgSizer1->Add( m_staticText12, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_RadiusSlider = new wxSlider( this, ID_RadiusSlider, 0, 0, 5120, wxDefaultPosition, wxSize( -1,-1 ), wxSL_HORIZONTAL|wxSL_SELRANGE );
	fgSizer1->Add( m_RadiusSlider, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxBOTTOM, 5 );
	
	m_RadiusTCtrl = new wxTextCtrl( this, ID_RadiusTCtrl, wxEmptyString, wxDefaultPosition, wxSize( 70,-1 ), wxTE_LEFT|wxTE_PROCESS_ENTER );
	fgSizer1->Add( m_RadiusTCtrl, 0, wxBOTTOM|wxRIGHT, 5 );
	
	m_staticText11 = new wxStaticText( this, wxID_ANY, wxT("FallOff -"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText11->Wrap( -1 );
	fgSizer1->Add( m_staticText11, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_FallOffSlider = new wxSlider( this, ID_FallOffSlider, 0, 0, 5120, wxDefaultPosition, wxSize( -1,-1 ), wxSL_HORIZONTAL|wxSL_SELRANGE );
	fgSizer1->Add( m_FallOffSlider, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxBOTTOM, 5 );
	
	m_FallOffTCtrl = new wxTextCtrl( this, ID_FallOffTCtrl, wxEmptyString, wxDefaultPosition, wxSize( 70,-1 ), wxTE_LEFT|wxTE_PROCESS_ENTER );
	fgSizer1->Add( m_FallOffTCtrl, 0, wxBOTTOM|wxRIGHT, 5 );
	
	sbSizer3->Add( fgSizer1, 1, wxEXPAND, 5 );
	
	bSizer15->Add( sbSizer3, 1, wxEXPAND|wxTOP|wxBOTTOM|wxRIGHT, 5 );
	
	this->SetSizer( bSizer15 );
	this->Layout();
	
	// Connect Events
	m_EffectRadioBox->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( TerrainToolDlgUI::OnEffectRadioBox ), NULL, this );
	m_StrSlider->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( TerrainToolDlgUI::OnScroll ), NULL, this );
	m_StrSlider->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( TerrainToolDlgUI::OnScroll ), NULL, this );
	m_StrSlider->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( TerrainToolDlgUI::OnScroll ), NULL, this );
	m_StrSlider->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( TerrainToolDlgUI::OnScroll ), NULL, this );
	m_StrSlider->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( TerrainToolDlgUI::OnScroll ), NULL, this );
	m_StrSlider->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( TerrainToolDlgUI::OnScroll ), NULL, this );
	m_StrSlider->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( TerrainToolDlgUI::OnScroll ), NULL, this );
	m_StrSlider->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( TerrainToolDlgUI::OnScroll ), NULL, this );
	m_StrSlider->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( TerrainToolDlgUI::OnScroll ), NULL, this );
	m_StrTCtrl->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( TerrainToolDlgUI::OnTextEnter ), NULL, this );
	m_RadiusSlider->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( TerrainToolDlgUI::OnScroll ), NULL, this );
	m_RadiusSlider->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( TerrainToolDlgUI::OnScroll ), NULL, this );
	m_RadiusSlider->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( TerrainToolDlgUI::OnScroll ), NULL, this );
	m_RadiusSlider->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( TerrainToolDlgUI::OnScroll ), NULL, this );
	m_RadiusSlider->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( TerrainToolDlgUI::OnScroll ), NULL, this );
	m_RadiusSlider->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( TerrainToolDlgUI::OnScroll ), NULL, this );
	m_RadiusSlider->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( TerrainToolDlgUI::OnScroll ), NULL, this );
	m_RadiusSlider->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( TerrainToolDlgUI::OnScroll ), NULL, this );
	m_RadiusSlider->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( TerrainToolDlgUI::OnScroll ), NULL, this );
	m_RadiusTCtrl->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( TerrainToolDlgUI::OnTextEnter ), NULL, this );
	m_FallOffSlider->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( TerrainToolDlgUI::OnScroll ), NULL, this );
	m_FallOffSlider->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( TerrainToolDlgUI::OnScroll ), NULL, this );
	m_FallOffSlider->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( TerrainToolDlgUI::OnScroll ), NULL, this );
	m_FallOffSlider->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( TerrainToolDlgUI::OnScroll ), NULL, this );
	m_FallOffSlider->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( TerrainToolDlgUI::OnScroll ), NULL, this );
	m_FallOffSlider->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( TerrainToolDlgUI::OnScroll ), NULL, this );
	m_FallOffSlider->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( TerrainToolDlgUI::OnScroll ), NULL, this );
	m_FallOffSlider->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( TerrainToolDlgUI::OnScroll ), NULL, this );
	m_FallOffSlider->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( TerrainToolDlgUI::OnScroll ), NULL, this );
	m_FallOffTCtrl->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( TerrainToolDlgUI::OnTextEnter ), NULL, this );
}

TerrainToolDlgUI::~TerrainToolDlgUI()
{
	// Disconnect Events
	m_EffectRadioBox->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( TerrainToolDlgUI::OnEffectRadioBox ), NULL, this );
	m_StrSlider->Disconnect( wxEVT_SCROLL_TOP, wxScrollEventHandler( TerrainToolDlgUI::OnScroll ), NULL, this );
	m_StrSlider->Disconnect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( TerrainToolDlgUI::OnScroll ), NULL, this );
	m_StrSlider->Disconnect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( TerrainToolDlgUI::OnScroll ), NULL, this );
	m_StrSlider->Disconnect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( TerrainToolDlgUI::OnScroll ), NULL, this );
	m_StrSlider->Disconnect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( TerrainToolDlgUI::OnScroll ), NULL, this );
	m_StrSlider->Disconnect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( TerrainToolDlgUI::OnScroll ), NULL, this );
	m_StrSlider->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( TerrainToolDlgUI::OnScroll ), NULL, this );
	m_StrSlider->Disconnect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( TerrainToolDlgUI::OnScroll ), NULL, this );
	m_StrSlider->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( TerrainToolDlgUI::OnScroll ), NULL, this );
	m_StrTCtrl->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( TerrainToolDlgUI::OnTextEnter ), NULL, this );
	m_RadiusSlider->Disconnect( wxEVT_SCROLL_TOP, wxScrollEventHandler( TerrainToolDlgUI::OnScroll ), NULL, this );
	m_RadiusSlider->Disconnect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( TerrainToolDlgUI::OnScroll ), NULL, this );
	m_RadiusSlider->Disconnect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( TerrainToolDlgUI::OnScroll ), NULL, this );
	m_RadiusSlider->Disconnect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( TerrainToolDlgUI::OnScroll ), NULL, this );
	m_RadiusSlider->Disconnect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( TerrainToolDlgUI::OnScroll ), NULL, this );
	m_RadiusSlider->Disconnect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( TerrainToolDlgUI::OnScroll ), NULL, this );
	m_RadiusSlider->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( TerrainToolDlgUI::OnScroll ), NULL, this );
	m_RadiusSlider->Disconnect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( TerrainToolDlgUI::OnScroll ), NULL, this );
	m_RadiusSlider->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( TerrainToolDlgUI::OnScroll ), NULL, this );
	m_RadiusTCtrl->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( TerrainToolDlgUI::OnTextEnter ), NULL, this );
	m_FallOffSlider->Disconnect( wxEVT_SCROLL_TOP, wxScrollEventHandler( TerrainToolDlgUI::OnScroll ), NULL, this );
	m_FallOffSlider->Disconnect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( TerrainToolDlgUI::OnScroll ), NULL, this );
	m_FallOffSlider->Disconnect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( TerrainToolDlgUI::OnScroll ), NULL, this );
	m_FallOffSlider->Disconnect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( TerrainToolDlgUI::OnScroll ), NULL, this );
	m_FallOffSlider->Disconnect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( TerrainToolDlgUI::OnScroll ), NULL, this );
	m_FallOffSlider->Disconnect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( TerrainToolDlgUI::OnScroll ), NULL, this );
	m_FallOffSlider->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( TerrainToolDlgUI::OnScroll ), NULL, this );
	m_FallOffSlider->Disconnect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( TerrainToolDlgUI::OnScroll ), NULL, this );
	m_FallOffSlider->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( TerrainToolDlgUI::OnScroll ), NULL, this );
	m_FallOffTCtrl->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( TerrainToolDlgUI::OnTextEnter ), NULL, this );
}
