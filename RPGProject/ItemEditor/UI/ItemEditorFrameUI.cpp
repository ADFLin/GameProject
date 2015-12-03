///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Mar 22 2011)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "ItemEditorFrameUI.h"

///////////////////////////////////////////////////////////////////////////

ItemEditorFrameUI::ItemEditorFrameUI( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxFrame( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer( wxHORIZONTAL );
	
	m_splitter2 = new wxSplitterWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D|wxSP_LIVE_UPDATE );
	m_splitter2->SetSashGravity( 1 );
	m_splitter2->Connect( wxEVT_IDLE, wxIdleEventHandler( ItemEditorFrameUI::m_splitter2OnIdle ), NULL, this );
	
	m_panel6 = new wxPanel( m_splitter2, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer21;
	bSizer21 = new wxBoxSizer( wxVERTICAL );
	
	m_splitter1 = new wxSplitterWindow( m_panel6, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D|wxSP_LIVE_UPDATE );
	m_splitter1->Connect( wxEVT_IDLE, wxIdleEventHandler( ItemEditorFrameUI::m_splitter1OnIdle ), NULL, this );
	m_splitter1->SetMinimumPaneSize( 1 );
	
	m_panel4 = new wxPanel( m_splitter1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxVERTICAL );
	
	m_ItemListCtrl = new wxListCtrl( m_panel4, ID_ItemListCtrl, wxDefaultPosition, wxDefaultSize, wxLC_REPORT|wxLC_SINGLE_SEL );
	bSizer7->Add( m_ItemListCtrl, 1, wxALL|wxEXPAND, 5 );
	
	wxArrayString m_ItemTypeChoiceChoices;
	m_ItemTypeChoice = new wxChoice( m_panel4, ID_ItemTypeChoice, wxDefaultPosition, wxDefaultSize, m_ItemTypeChoiceChoices, 0 );
	m_ItemTypeChoice->SetSelection( 0 );
	bSizer7->Add( m_ItemTypeChoice, 0, wxALL|wxEXPAND, 5 );
	
	wxArrayString m_BoneChoiceChoices;
	m_BoneChoice = new wxChoice( m_panel4, ID_BoneChoice, wxDefaultPosition, wxDefaultSize, m_BoneChoiceChoices, 0 );
	m_BoneChoice->SetSelection( 0 );
	bSizer7->Add( m_BoneChoice, 0, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxHORIZONTAL );
	
	m_NewItemButton = new wxButton( m_panel4, ID_NewItemButton, wxT("new"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer8->Add( m_NewItemButton, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_ModifyButton = new wxButton( m_panel4, ID_ModifyButton, wxT("Modify"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer8->Add( m_ModifyButton, 1, wxALL, 5 );
	
	bSizer7->Add( bSizer8, 0, wxEXPAND, 5 );
	
	m_staticline2 = new wxStaticLine( m_panel4, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer7->Add( m_staticline2, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_panel4->SetSizer( bSizer7 );
	m_panel4->Layout();
	bSizer7->Fit( m_panel4 );
	m_EditPanel = new wxPanel( m_splitter1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticText* m_staticText1;
	m_staticText1 = new wxStaticText( m_EditPanel, wxID_ANY, wxT("Name"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	bSizer4->Add( m_staticText1, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_ItemNameTextCtrl = new wxTextCtrl( m_EditPanel, ID_ItemNameTextCtrl, wxEmptyString, wxDefaultPosition, wxSize( 200,-1 ), wxTE_PROCESS_ENTER );
	bSizer4->Add( m_ItemNameTextCtrl, 1, wxALL|wxEXPAND, 5 );
	
	m_staticText12 = new wxStaticText( m_EditPanel, wxID_ANY, wxT("CD time"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText12->Wrap( -1 );
	bSizer4->Add( m_staticText12, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_CDTimeTCtrl = new wxTextCtrl( m_EditPanel, ID_CDTimeTCtrl, wxEmptyString, wxDefaultPosition, wxSize( 80,-1 ), wxTE_PROCESS_ENTER );
	bSizer4->Add( m_CDTimeTCtrl, 0, wxALL, 5 );
	
	bSizer5->Add( bSizer4, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticText* m_staticText4;
	m_staticText4 = new wxStaticText( m_EditPanel, wxID_ANY, wxT("BuyCast"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText4->Wrap( -1 );
	bSizer3->Add( m_staticText4, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_BuyCastSpin = new wxSpinCtrl( m_EditPanel, ID_BuyCastSpin, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 999999, 1 );
	bSizer3->Add( m_BuyCastSpin, 1, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 3 );
	
	wxStaticText* m_staticText41;
	m_staticText41 = new wxStaticText( m_EditPanel, wxID_ANY, wxT("SellCast"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText41->Wrap( -1 );
	bSizer3->Add( m_staticText41, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 3 );
	
	m_SellCastSpin = new wxSpinCtrl( m_EditPanel, ID_SellCastSpin, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 999999, 3 );
	bSizer3->Add( m_SellCastSpin, 1, wxBOTTOM|wxRIGHT|wxLEFT, 3 );
	
	wxStaticText* m_staticText3;
	m_staticText3 = new wxStaticText( m_EditPanel, wxID_ANY, wxT("MaxSoltNum"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText3->Wrap( -1 );
	bSizer3->Add( m_staticText3, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 3 );
	
	m_MaxSoltNumSpin = new wxSpinCtrl( m_EditPanel, ID_MaxSoltNumSpin, wxEmptyString, wxDefaultPosition, wxSize( 50,-1 ), wxSP_ARROW_KEYS, 0, 255, 0 );
	bSizer3->Add( m_MaxSoltNumSpin, 0, wxBOTTOM|wxRIGHT|wxLEFT, 3 );
	
	bSizer5->Add( bSizer3, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer18;
	bSizer18 = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticBoxSizer* sbSizer31;
	sbSizer31 = new wxStaticBoxSizer( new wxStaticBox( m_EditPanel, wxID_ANY, wxT("Icon") ), wxVERTICAL );
	
	m_IconCBox = new wxBitmapComboBox( m_EditPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 100,-1 ), 0, NULL, wxCB_READONLY ); 
	sbSizer31->Add( m_IconCBox, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	wxBoxSizer* bSizer17;
	bSizer17 = new wxBoxSizer( wxHORIZONTAL );
	
	m_AddIconButton = new wxButton( m_EditPanel, ID_AddIconButton, wxT("Add"), wxDefaultPosition, wxSize( 45,-1 ), 0 );
	bSizer17->Add( m_AddIconButton, 0, wxALIGN_RIGHT|wxALL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 3 );
	
	m_DelIconButton = new wxButton( m_EditPanel, ID_DelIconButton, wxT("Delete"), wxDefaultPosition, wxSize( 45,-1 ), 0 );
	bSizer17->Add( m_DelIconButton, 0, wxALIGN_RIGHT|wxALL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 3 );
	
	m_ModifyIconButton = new wxButton( m_EditPanel, ID_ModifyIconButton, wxT("Modify"), wxDefaultPosition, wxSize( 45,-1 ), 0 );
	bSizer17->Add( m_ModifyIconButton, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 3 );
	
	sbSizer31->Add( bSizer17, 0, wxEXPAND, 5 );
	
	bSizer18->Add( sbSizer31, 0, 0, 5 );
	
	m_HelpStrTCtrl = new wxTextCtrl( m_EditPanel, ID_HelpStrTCtrl, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_LEFT|wxTE_MULTILINE|wxTE_PROCESS_ENTER );
	bSizer18->Add( m_HelpStrTCtrl, 1, wxALL|wxEXPAND, 5 );
	
	bSizer5->Add( bSizer18, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxHORIZONTAL );
	
	m_PMListCtrl = new wxListCtrl( m_EditPanel, ID_PMListCtrl, wxDefaultPosition, wxSize( -1,150 ), wxLC_REPORT|wxLC_SINGLE_SEL );
	bSizer9->Add( m_PMListCtrl, 1, wxALIGN_CENTER_HORIZONTAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bSizer11;
	bSizer11 = new wxBoxSizer( wxVERTICAL );
	
	m_PMAddButton = new wxButton( m_EditPanel, ID_PMAddButton, wxT("Add"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer11->Add( m_PMAddButton, 0, wxALL, 3 );
	
	m_PMRemoveButton = new wxButton( m_EditPanel, ID_PMRemoveButton, wxT("Remove"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer11->Add( m_PMRemoveButton, 0, wxALL, 3 );
	
	m_PMModifyButton = new wxButton( m_EditPanel, ID_PMModifyButton, wxT("Edit"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer11->Add( m_PMModifyButton, 0, wxALL, 3 );
	
	bSizer9->Add( bSizer11, 0, wxEXPAND, 5 );
	
	bSizer5->Add( bSizer9, 0, wxEXPAND, 5 );
	
	m_staticline1 = new wxStaticLine( m_EditPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer5->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );
	
	m_EquimentPanel = new wxPanel( m_EditPanel, ID_EquimentPanel, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxStaticBoxSizer* sbSizer3;
	sbSizer3 = new wxStaticBoxSizer( new wxStaticBox( m_EquimentPanel, wxID_ANY, wxT("Equipment") ), wxVERTICAL );
	
	wxBoxSizer* bSizer23;
	bSizer23 = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticText* m_staticText9;
	m_staticText9 = new wxStaticText( m_EquimentPanel, wxID_ANY, wxT("Equip Value"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText9->Wrap( -1 );
	bSizer23->Add( m_staticText9, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_EquipValSpin = new wxSpinCtrl( m_EquimentPanel, ID_EquipValSpin, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 9999, 2 );
	bSizer23->Add( m_EquipValSpin, 0, wxALL, 5 );
	
	wxStaticText* m_staticText10;
	m_staticText10 = new wxStaticText( m_EquimentPanel, wxID_ANY, wxT("Equip Solt"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText10->Wrap( -1 );
	bSizer23->Add( m_staticText10, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	wxArrayString m_EquipSoltChoiceChoices;
	m_EquipSoltChoice = new wxChoice( m_EquimentPanel, ID_EquipSoltChoice, wxDefaultPosition, wxDefaultSize, m_EquipSoltChoiceChoices, 0 );
	m_EquipSoltChoice->SetSelection( 0 );
	bSizer23->Add( m_EquipSoltChoice, 1, wxALL|wxEXPAND, 5 );
	
	sbSizer3->Add( bSizer23, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer24;
	bSizer24 = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticText* m_staticText131;
	m_staticText131 = new wxStaticText( m_EquimentPanel, wxID_ANY, wxT("Model Name"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText131->Wrap( -1 );
	bSizer24->Add( m_staticText131, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_ModelNameTCtrl = new wxTextCtrl( m_EquimentPanel, ID_ModelNameTCtrl, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_LEFT|wxTE_READONLY );
	bSizer24->Add( m_ModelNameTCtrl, 0, wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	m_button17 = new wxButton( m_EquimentPanel, wxID_ANY, wxT(".."), wxDefaultPosition, wxSize( 20,-1 ), 0 );
	bSizer24->Add( m_button17, 0, wxTOP|wxBOTTOM, 5 );
	
	m_button18 = new wxButton( m_EquimentPanel, wxID_ANY, wxT("D"), wxDefaultPosition, wxSize( 20,-1 ), 0 );
	bSizer24->Add( m_button18, 0, wxTOP|wxBOTTOM|wxRIGHT, 5 );
	
	m_staticText16 = new wxStaticText( m_EquimentPanel, wxID_ANY, wxT("Scale"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText16->Wrap( -1 );
	bSizer24->Add( m_staticText16, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_ScaleTCtrl = new wxTextCtrl( m_EquimentPanel, ID_ScaleTCtrl, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer24->Add( m_ScaleTCtrl, 0, wxALL, 5 );
	
	m_EditModelCBox = new wxCheckBox( m_EquimentPanel, ID_EditModelCBox, wxT("Edit Model"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer24->Add( m_EditModelCBox, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	sbSizer3->Add( bSizer24, 0, wxEXPAND, 5 );
	
	m_EquimentPanel->SetSizer( sbSizer3 );
	m_EquimentPanel->Layout();
	sbSizer3->Fit( m_EquimentPanel );
	bSizer5->Add( m_EquimentPanel, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	m_WeaponPanel = new wxPanel( m_EditPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxStaticBoxSizer* sbSizer1;
	sbSizer1 = new wxStaticBoxSizer( new wxStaticBox( m_WeaponPanel, wxID_ANY, wxT("Weapon") ), wxHORIZONTAL );
	
	m_staticText13 = new wxStaticText( m_WeaponPanel, wxID_ANY, wxT("WeaponType"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText13->Wrap( -1 );
	sbSizer1->Add( m_staticText13, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	wxArrayString m_WeaponTypeChoiceChoices;
	m_WeaponTypeChoice = new wxChoice( m_WeaponPanel, ID_WeaponTypeChoice, wxDefaultPosition, wxDefaultSize, m_WeaponTypeChoiceChoices, 0 );
	m_WeaponTypeChoice->SetSelection( 0 );
	sbSizer1->Add( m_WeaponTypeChoice, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_WeaponPanel->SetSizer( sbSizer1 );
	m_WeaponPanel->Layout();
	sbSizer1->Fit( m_WeaponPanel );
	bSizer5->Add( m_WeaponPanel, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	m_EditPanel->SetSizer( bSizer5 );
	m_EditPanel->Layout();
	bSizer5->Fit( m_EditPanel );
	m_splitter1->SplitVertically( m_panel4, m_EditPanel, 0 );
	bSizer21->Add( m_splitter1, 1, wxEXPAND, 5 );
	
	m_panel6->SetSizer( bSizer21 );
	m_panel6->Layout();
	bSizer21->Fit( m_panel6 );
	m_DrawPanel = new wxPanel( m_splitter2, ID_DrawPanel, wxDefaultPosition, wxSize( 300,-1 ), wxSTATIC_BORDER|wxTAB_TRAVERSAL );
	m_splitter2->SplitVertically( m_panel6, m_DrawPanel, 0 );
	bSizer1->Add( m_splitter2, 1, wxEXPAND, 5 );
	
	this->SetSizer( bSizer1 );
	this->Layout();
	
	// Connect Events
	m_ItemListCtrl->Connect( wxEVT_COMMAND_LIST_COL_CLICK, wxListEventHandler( ItemEditorFrameUI::OnItemListColClick ), NULL, this );
	m_ItemListCtrl->Connect( wxEVT_COMMAND_LIST_ITEM_DESELECTED, wxListEventHandler( ItemEditorFrameUI::OnItemListDeselected ), NULL, this );
	m_ItemListCtrl->Connect( wxEVT_COMMAND_LIST_ITEM_RIGHT_CLICK, wxListEventHandler( ItemEditorFrameUI::OnItemListRightClick ), NULL, this );
	m_ItemListCtrl->Connect( wxEVT_COMMAND_LIST_ITEM_SELECTED, wxListEventHandler( ItemEditorFrameUI::OnItemListSelected ), NULL, this );
	m_ItemTypeChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( ItemEditorFrameUI::OnItemTypeChoice ), NULL, this );
	m_NewItemButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ItemEditorFrameUI::OnNewItemButton ), NULL, this );
	m_ModifyButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ItemEditorFrameUI::OnModifyItem ), NULL, this );
	m_ItemNameTextCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( ItemEditorFrameUI::OnChangeSetting ), NULL, this );
	m_ItemNameTextCtrl->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( ItemEditorFrameUI::OnChangeSetting ), NULL, this );
	m_CDTimeTCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( ItemEditorFrameUI::OnChangeSetting ), NULL, this );
	m_CDTimeTCtrl->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( ItemEditorFrameUI::OnChangeSetting ), NULL, this );
	m_BuyCastSpin->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( ItemEditorFrameUI::OnChangeSetting ), NULL, this );
	m_BuyCastSpin->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( ItemEditorFrameUI::OnChangeSetting ), NULL, this );
	m_SellCastSpin->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( ItemEditorFrameUI::OnChangeSetting ), NULL, this );
	m_SellCastSpin->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( ItemEditorFrameUI::OnChangeSetting ), NULL, this );
	m_MaxSoltNumSpin->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( ItemEditorFrameUI::OnChangeSetting ), NULL, this );
	m_MaxSoltNumSpin->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( ItemEditorFrameUI::OnChangeSetting ), NULL, this );
	m_IconCBox->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( ItemEditorFrameUI::OnChangeSetting ), NULL, this );
	m_AddIconButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ItemEditorFrameUI::OnIconClick ), NULL, this );
	m_DelIconButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ItemEditorFrameUI::OnIconClick ), NULL, this );
	m_ModifyIconButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ItemEditorFrameUI::OnIconClick ), NULL, this );
	m_HelpStrTCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( ItemEditorFrameUI::OnChangeSetting ), NULL, this );
	m_HelpStrTCtrl->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( ItemEditorFrameUI::OnChangeSetting ), NULL, this );
	m_PMListCtrl->Connect( wxEVT_COMMAND_LIST_ITEM_DESELECTED, wxListEventHandler( ItemEditorFrameUI::OnPMListDeselected ), NULL, this );
	m_PMListCtrl->Connect( wxEVT_COMMAND_LIST_ITEM_SELECTED, wxListEventHandler( ItemEditorFrameUI::OnPMListSelected ), NULL, this );
	m_PMAddButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ItemEditorFrameUI::OnPMAddClick ), NULL, this );
	m_PMRemoveButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ItemEditorFrameUI::OnPMRemoveClick ), NULL, this );
	m_PMModifyButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ItemEditorFrameUI::OnPMModifyClick ), NULL, this );
	m_EquipValSpin->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( ItemEditorFrameUI::OnChangeSetting ), NULL, this );
	m_EquipValSpin->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( ItemEditorFrameUI::OnChangeSetting ), NULL, this );
	m_EquipSoltChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( ItemEditorFrameUI::OnChangeSetting ), NULL, this );
	m_button17->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ItemEditorFrameUI::OnChioceModelClick ), NULL, this );
	m_button18->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ItemEditorFrameUI::OnModelNameRemove ), NULL, this );
	m_ScaleTCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( ItemEditorFrameUI::OnChangeSetting ), NULL, this );
	m_ScaleTCtrl->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( ItemEditorFrameUI::OnChangeSetting ), NULL, this );
	m_EditModelCBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( ItemEditorFrameUI::OnEditModelCheck ), NULL, this );
	m_WeaponTypeChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( ItemEditorFrameUI::OnChangeSetting ), NULL, this );
	m_DrawPanel->Connect( wxEVT_ERASE_BACKGROUND, wxEraseEventHandler( ItemEditorFrameUI::OnDraw ), NULL, this );
	m_DrawPanel->Connect( wxEVT_KEY_DOWN, wxKeyEventHandler( ItemEditorFrameUI::OnKeyDown ), NULL, this );
	m_DrawPanel->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( ItemEditorFrameUI::OnDrawPanelMouse ), NULL, this );
	m_DrawPanel->Connect( wxEVT_LEFT_UP, wxMouseEventHandler( ItemEditorFrameUI::OnDrawPanelMouse ), NULL, this );
	m_DrawPanel->Connect( wxEVT_MIDDLE_DOWN, wxMouseEventHandler( ItemEditorFrameUI::OnDrawPanelMouse ), NULL, this );
	m_DrawPanel->Connect( wxEVT_MIDDLE_UP, wxMouseEventHandler( ItemEditorFrameUI::OnDrawPanelMouse ), NULL, this );
	m_DrawPanel->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( ItemEditorFrameUI::OnDrawPanelMouse ), NULL, this );
	m_DrawPanel->Connect( wxEVT_RIGHT_UP, wxMouseEventHandler( ItemEditorFrameUI::OnDrawPanelMouse ), NULL, this );
	m_DrawPanel->Connect( wxEVT_MOTION, wxMouseEventHandler( ItemEditorFrameUI::OnDrawPanelMouse ), NULL, this );
	m_DrawPanel->Connect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( ItemEditorFrameUI::OnDrawPanelMouse ), NULL, this );
	m_DrawPanel->Connect( wxEVT_MIDDLE_DCLICK, wxMouseEventHandler( ItemEditorFrameUI::OnDrawPanelMouse ), NULL, this );
	m_DrawPanel->Connect( wxEVT_RIGHT_DCLICK, wxMouseEventHandler( ItemEditorFrameUI::OnDrawPanelMouse ), NULL, this );
	m_DrawPanel->Connect( wxEVT_LEAVE_WINDOW, wxMouseEventHandler( ItemEditorFrameUI::OnDrawPanelMouse ), NULL, this );
	m_DrawPanel->Connect( wxEVT_ENTER_WINDOW, wxMouseEventHandler( ItemEditorFrameUI::OnDrawPanelMouse ), NULL, this );
	m_DrawPanel->Connect( wxEVT_MOUSEWHEEL, wxMouseEventHandler( ItemEditorFrameUI::OnDrawPanelMouse ), NULL, this );
	m_DrawPanel->Connect( wxEVT_SIZE, wxSizeEventHandler( ItemEditorFrameUI::OnDrawPanelResize ), NULL, this );
}

ItemEditorFrameUI::~ItemEditorFrameUI()
{
	// Disconnect Events
	m_ItemListCtrl->Disconnect( wxEVT_COMMAND_LIST_COL_CLICK, wxListEventHandler( ItemEditorFrameUI::OnItemListColClick ), NULL, this );
	m_ItemListCtrl->Disconnect( wxEVT_COMMAND_LIST_ITEM_DESELECTED, wxListEventHandler( ItemEditorFrameUI::OnItemListDeselected ), NULL, this );
	m_ItemListCtrl->Disconnect( wxEVT_COMMAND_LIST_ITEM_RIGHT_CLICK, wxListEventHandler( ItemEditorFrameUI::OnItemListRightClick ), NULL, this );
	m_ItemListCtrl->Disconnect( wxEVT_COMMAND_LIST_ITEM_SELECTED, wxListEventHandler( ItemEditorFrameUI::OnItemListSelected ), NULL, this );
	m_ItemTypeChoice->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( ItemEditorFrameUI::OnItemTypeChoice ), NULL, this );
	m_NewItemButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ItemEditorFrameUI::OnNewItemButton ), NULL, this );
	m_ModifyButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ItemEditorFrameUI::OnModifyItem ), NULL, this );
	m_ItemNameTextCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( ItemEditorFrameUI::OnChangeSetting ), NULL, this );
	m_ItemNameTextCtrl->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( ItemEditorFrameUI::OnChangeSetting ), NULL, this );
	m_CDTimeTCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( ItemEditorFrameUI::OnChangeSetting ), NULL, this );
	m_CDTimeTCtrl->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( ItemEditorFrameUI::OnChangeSetting ), NULL, this );
	m_BuyCastSpin->Disconnect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( ItemEditorFrameUI::OnChangeSetting ), NULL, this );
	m_BuyCastSpin->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( ItemEditorFrameUI::OnChangeSetting ), NULL, this );
	m_SellCastSpin->Disconnect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( ItemEditorFrameUI::OnChangeSetting ), NULL, this );
	m_SellCastSpin->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( ItemEditorFrameUI::OnChangeSetting ), NULL, this );
	m_MaxSoltNumSpin->Disconnect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( ItemEditorFrameUI::OnChangeSetting ), NULL, this );
	m_MaxSoltNumSpin->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( ItemEditorFrameUI::OnChangeSetting ), NULL, this );
	m_IconCBox->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( ItemEditorFrameUI::OnChangeSetting ), NULL, this );
	m_AddIconButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ItemEditorFrameUI::OnIconClick ), NULL, this );
	m_DelIconButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ItemEditorFrameUI::OnIconClick ), NULL, this );
	m_ModifyIconButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ItemEditorFrameUI::OnIconClick ), NULL, this );
	m_HelpStrTCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( ItemEditorFrameUI::OnChangeSetting ), NULL, this );
	m_HelpStrTCtrl->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( ItemEditorFrameUI::OnChangeSetting ), NULL, this );
	m_PMListCtrl->Disconnect( wxEVT_COMMAND_LIST_ITEM_DESELECTED, wxListEventHandler( ItemEditorFrameUI::OnPMListDeselected ), NULL, this );
	m_PMListCtrl->Disconnect( wxEVT_COMMAND_LIST_ITEM_SELECTED, wxListEventHandler( ItemEditorFrameUI::OnPMListSelected ), NULL, this );
	m_PMAddButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ItemEditorFrameUI::OnPMAddClick ), NULL, this );
	m_PMRemoveButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ItemEditorFrameUI::OnPMRemoveClick ), NULL, this );
	m_PMModifyButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ItemEditorFrameUI::OnPMModifyClick ), NULL, this );
	m_EquipValSpin->Disconnect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( ItemEditorFrameUI::OnChangeSetting ), NULL, this );
	m_EquipValSpin->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( ItemEditorFrameUI::OnChangeSetting ), NULL, this );
	m_EquipSoltChoice->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( ItemEditorFrameUI::OnChangeSetting ), NULL, this );
	m_button17->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ItemEditorFrameUI::OnChioceModelClick ), NULL, this );
	m_button18->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ItemEditorFrameUI::OnModelNameRemove ), NULL, this );
	m_ScaleTCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( ItemEditorFrameUI::OnChangeSetting ), NULL, this );
	m_ScaleTCtrl->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( ItemEditorFrameUI::OnChangeSetting ), NULL, this );
	m_EditModelCBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( ItemEditorFrameUI::OnEditModelCheck ), NULL, this );
	m_WeaponTypeChoice->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( ItemEditorFrameUI::OnChangeSetting ), NULL, this );
	m_DrawPanel->Disconnect( wxEVT_ERASE_BACKGROUND, wxEraseEventHandler( ItemEditorFrameUI::OnDraw ), NULL, this );
	m_DrawPanel->Disconnect( wxEVT_KEY_DOWN, wxKeyEventHandler( ItemEditorFrameUI::OnKeyDown ), NULL, this );
	m_DrawPanel->Disconnect( wxEVT_LEFT_DOWN, wxMouseEventHandler( ItemEditorFrameUI::OnDrawPanelMouse ), NULL, this );
	m_DrawPanel->Disconnect( wxEVT_LEFT_UP, wxMouseEventHandler( ItemEditorFrameUI::OnDrawPanelMouse ), NULL, this );
	m_DrawPanel->Disconnect( wxEVT_MIDDLE_DOWN, wxMouseEventHandler( ItemEditorFrameUI::OnDrawPanelMouse ), NULL, this );
	m_DrawPanel->Disconnect( wxEVT_MIDDLE_UP, wxMouseEventHandler( ItemEditorFrameUI::OnDrawPanelMouse ), NULL, this );
	m_DrawPanel->Disconnect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( ItemEditorFrameUI::OnDrawPanelMouse ), NULL, this );
	m_DrawPanel->Disconnect( wxEVT_RIGHT_UP, wxMouseEventHandler( ItemEditorFrameUI::OnDrawPanelMouse ), NULL, this );
	m_DrawPanel->Disconnect( wxEVT_MOTION, wxMouseEventHandler( ItemEditorFrameUI::OnDrawPanelMouse ), NULL, this );
	m_DrawPanel->Disconnect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( ItemEditorFrameUI::OnDrawPanelMouse ), NULL, this );
	m_DrawPanel->Disconnect( wxEVT_MIDDLE_DCLICK, wxMouseEventHandler( ItemEditorFrameUI::OnDrawPanelMouse ), NULL, this );
	m_DrawPanel->Disconnect( wxEVT_RIGHT_DCLICK, wxMouseEventHandler( ItemEditorFrameUI::OnDrawPanelMouse ), NULL, this );
	m_DrawPanel->Disconnect( wxEVT_LEAVE_WINDOW, wxMouseEventHandler( ItemEditorFrameUI::OnDrawPanelMouse ), NULL, this );
	m_DrawPanel->Disconnect( wxEVT_ENTER_WINDOW, wxMouseEventHandler( ItemEditorFrameUI::OnDrawPanelMouse ), NULL, this );
	m_DrawPanel->Disconnect( wxEVT_MOUSEWHEEL, wxMouseEventHandler( ItemEditorFrameUI::OnDrawPanelMouse ), NULL, this );
	m_DrawPanel->Disconnect( wxEVT_SIZE, wxSizeEventHandler( ItemEditorFrameUI::OnDrawPanelResize ), NULL, this );
	
}

PropModifyDlgUI::PropModifyDlgUI( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer15;
	bSizer15 = new wxBoxSizer( wxVERTICAL );
	
	wxFlexGridSizer* fgSizer;
	fgSizer = new wxFlexGridSizer( 2, 2, 0, 0 );
	fgSizer->AddGrowableCol( 1 );
	fgSizer->SetFlexibleDirection( wxBOTH );
	fgSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticText5 = new wxStaticText( this, wxID_ANY, wxT("PropType - "), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText5->Wrap( -1 );
	fgSizer->Add( m_staticText5, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT|wxALIGN_RIGHT, 5 );
	
	wxArrayString m_PropTyeChoiceChoices;
	m_PropTyeChoice = new wxChoice( this, ID_PropTyeChoice, wxDefaultPosition, wxDefaultSize, m_PropTyeChoiceChoices, 0 );
	m_PropTyeChoice->SetSelection( 0 );
	fgSizer->Add( m_PropTyeChoice, 1, wxTOP|wxBOTTOM|wxRIGHT, 5 );
	
	m_staticText7 = new wxStaticText( this, wxID_ANY, wxT("Value - "), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText7->Wrap( -1 );
	fgSizer->Add( m_staticText7, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxBOTTOM|wxLEFT, 5 );
	
	wxBoxSizer* bSizer18;
	bSizer18 = new wxBoxSizer( wxHORIZONTAL );
	
	wxString m_ValOpChoiceChoices[] = { wxT("Add"), wxT("Mul"), wxT("Sub"), wxT("Div") };
	int m_ValOpChoiceNChoices = sizeof( m_ValOpChoiceChoices ) / sizeof( wxString );
	m_ValOpChoice = new wxChoice( this, ID_ValOpChoice, wxDefaultPosition, wxSize( 60,-1 ), m_ValOpChoiceNChoices, m_ValOpChoiceChoices, 0 );
	m_ValOpChoice->SetSelection( 0 );
	bSizer18->Add( m_ValOpChoice, 0, wxBOTTOM|wxRIGHT, 5 );
	
	m_ValueTextCtrl = new wxTextCtrl( this, ID_ValueTextCtrl, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer18->Add( m_ValueTextCtrl, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	fgSizer->Add( bSizer18, 1, wxEXPAND, 5 );
	
	wxStaticText* m_staticText8;
	m_staticText8 = new wxStaticText( this, wxID_ANY, wxT("Time -"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText8->Wrap( -1 );
	fgSizer->Add( m_staticText8, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_TimeTextCtrl = new wxTextCtrl( this, ID_TimeTextCtrl, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer->Add( m_TimeTextCtrl, 0, wxBOTTOM|wxRIGHT, 5 );
	
	bSizer15->Add( fgSizer, 0, wxEXPAND, 5 );
	
	m_staticline5 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer15->Add( m_staticline5, 0, wxEXPAND | wxALL, 5 );
	
	m_NoRepeatCheckBox = new wxCheckBox( this, ID_NoRepeatCheckBox, wxT("NO_REPEAT"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer15->Add( m_NoRepeatCheckBox, 0, wxALL, 5 );
	
	wxBoxSizer* bSizer20;
	bSizer20 = new wxBoxSizer( wxHORIZONTAL );
	
	m_TAddCheckBox = new wxCheckBox( this, ID_TAddCheckBox, wxT("TOP_ADD"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer20->Add( m_TAddCheckBox, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_TSubCheckBox = new wxCheckBox( this, ID_TSubCheckBox, wxT("TOP_SUB"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer20->Add( m_TSubCheckBox, 0, wxALL, 5 );
	
	bSizer15->Add( bSizer20, 0, 0, 5 );
	
	
	bSizer15->Add( 0, 0, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer21;
	bSizer21 = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer21->Add( 0, 0, 1, 0, 5 );
	
	m_button8 = new wxButton( this, wxID_OK, wxT("OK"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer21->Add( m_button8, 0, wxALL, 5 );
	
	m_button7 = new wxButton( this, wxID_CANCEL, wxT("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer21->Add( m_button7, 0, wxALL, 5 );
	
	bSizer15->Add( bSizer21, 0, wxEXPAND, 5 );
	
	this->SetSizer( bSizer15 );
	this->Layout();
}

PropModifyDlgUI::~PropModifyDlgUI()
{
}

ItemIDChangeDlgUI::ItemIDChangeDlgUI( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer21;
	bSizer21 = new wxBoxSizer( wxVERTICAL );
	
	
	bSizer21->Add( 0, 0, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer31;
	bSizer31 = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer31->Add( 0, 0, 1, wxEXPAND, 5 );
	
	wxStaticText* m_staticText12;
	m_staticText12 = new wxStaticText( this, wxID_ANY, wxT("ID"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText12->Wrap( -1 );
	bSizer31->Add( m_staticText12, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_IDSpinCtrl = new wxSpinCtrl( this, ID_IDSpinCtrl, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxSP_WRAP, 0, 1024, 0 );
	bSizer31->Add( m_IDSpinCtrl, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer31->Add( 0, 0, 1, wxEXPAND, 5 );
	
	bSizer21->Add( bSizer31, 1, wxEXPAND, 5 );
	
	
	bSizer21->Add( 0, 0, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer30;
	bSizer30 = new wxBoxSizer( wxHORIZONTAL );
	
	m_button8 = new wxButton( this, wxID_OK, wxT("OK"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer30->Add( m_button8, 0, wxALL|wxALIGN_BOTTOM, 5 );
	
	m_button7 = new wxButton( this, wxID_CANCEL, wxT("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer30->Add( m_button7, 0, wxALL|wxALIGN_BOTTOM, 5 );
	
	bSizer21->Add( bSizer30, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	this->SetSizer( bSizer21 );
	this->Layout();
}

ItemIDChangeDlgUI::~ItemIDChangeDlgUI()
{
}

SkIconEditorDlgUI::SkIconEditorDlgUI( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer21;
	bSizer21 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer20;
	bSizer20 = new wxBoxSizer( wxHORIZONTAL );
	
	m_IconCBox = new wxBitmapComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY ); 
	bSizer20->Add( m_IconCBox, 1, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* bSizer17;
	bSizer17 = new wxBoxSizer( wxVERTICAL );
	
	m_AddIconButton = new wxButton( this, ID_AddIconButton, wxT("Add"), wxDefaultPosition, wxSize( 80,-1 ), 0 );
	bSizer17->Add( m_AddIconButton, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 3 );
	
	m_ModifyIconButton = new wxButton( this, ID_ModifyIconButton, wxT("Modify"), wxDefaultPosition, wxSize( 80,-1 ), 0 );
	bSizer17->Add( m_ModifyIconButton, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_HORIZONTAL, 3 );
	
	m_staticline4 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer17->Add( m_staticline4, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );
	
	m_ChangeButton = new wxButton( this, ID_ChangeButton, wxT("Change"), wxDefaultPosition, wxSize( 80,-1 ), 0 );
	bSizer17->Add( m_ChangeButton, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 3 );
	
	m_OKButton = new wxButton( this, wxID_OK, wxT("OK"), wxDefaultPosition, wxSize( 80,-1 ), 0 );
	bSizer17->Add( m_OKButton, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	bSizer20->Add( bSizer17, 0, wxEXPAND, 5 );
	
	bSizer21->Add( bSizer20, 1, wxEXPAND, 5 );
	
	wxArrayString m_SKChoiceChoices;
	m_SKChoice = new wxChoice( this, ID_SKChoice, wxDefaultPosition, wxDefaultSize, m_SKChoiceChoices, 0 );
	m_SKChoice->SetSelection( 0 );
	bSizer21->Add( m_SKChoice, 0, wxALL|wxEXPAND, 5 );
	
	this->SetSizer( bSizer21 );
	this->Layout();
	
	// Connect Events
	m_IconCBox->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( SkIconEditorDlgUI::OnIconSelect ), NULL, this );
	m_AddIconButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SkIconEditorDlgUI::OnIconClick ), NULL, this );
	m_ModifyIconButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SkIconEditorDlgUI::OnIconClick ), NULL, this );
	m_ChangeButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SkIconEditorDlgUI::OnChangeClick ), NULL, this );
	m_SKChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( SkIconEditorDlgUI::OnSKChoice ), NULL, this );
}

SkIconEditorDlgUI::~SkIconEditorDlgUI()
{
	// Disconnect Events
	m_IconCBox->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( SkIconEditorDlgUI::OnIconSelect ), NULL, this );
	m_AddIconButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SkIconEditorDlgUI::OnIconClick ), NULL, this );
	m_ModifyIconButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SkIconEditorDlgUI::OnIconClick ), NULL, this );
	m_ChangeButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SkIconEditorDlgUI::OnChangeClick ), NULL, this );
	m_SKChoice->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( SkIconEditorDlgUI::OnSKChoice ), NULL, this );
	
}
