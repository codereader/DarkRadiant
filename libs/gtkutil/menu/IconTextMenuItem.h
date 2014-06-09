#pragma once

#include "iuimanager.h"
#include <wx/artprov.h>
#include <wx/menuitem.h>

namespace wxutil
{

class IconTextMenuItem :
	public wxMenuItem
{
public:
	// Use a local bitmap as found in the application's bitmaps folder.
	IconTextMenuItem(const std::string& text, const std::string& localBitmapFilename) :
		wxMenuItem(NULL, wxID_ANY, text, "")
	{
		SetBitmap(wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + localBitmapFilename));
	}
};

class StockIconTextMenuItem :
	public wxMenuItem
{
public:
	// Use a stock image specified by its art ID
	StockIconTextMenuItem(const std::string& text, const wxArtID& artId) :
		wxMenuItem(NULL, wxID_ANY, text, "")
	{
		SetBitmap(wxArtProvider::GetBitmap(artId, wxART_MENU));
	}
};

}