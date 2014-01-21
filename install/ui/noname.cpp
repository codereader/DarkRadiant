///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Nov  6 2013)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "noname.h"

///////////////////////////////////////////////////////////////////////////

MyPanel1::MyPanel1( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style ) : wxPanel( parent, id, pos, size, style )
{
	wxBoxSizer* camSizer;
	camSizer = new wxBoxSizer( wxVERTICAL );
	
	camToolbar = new wxToolBar( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTB_HORIZONTAL ); 
	camToolbar->AddTool( wxID_ANY, wxT("tool"), wxArtProvider::GetBitmap( wireframeMode16.png, wxART_TOOLBAR ), wxNullBitmap, wxITEM_RADIO, wxEmptyString, wxEmptyString, NULL ); 
	
	camToolbar->Realize(); 
	
	camSizer->Add( camToolbar, 0, wxEXPAND, 5 );
	
	
	this->SetSizer( camSizer );
	this->Layout();
}

MyPanel1::~MyPanel1()
{
}
