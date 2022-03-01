#pragma once

#include "wxutil/Bitmap.h"
#include <wx/menuitem.h>

namespace wxutil
{

class IconTextMenuItem :
	public wxMenuItem
{
public:
	// Use a local bitmap as found in the application's bitmaps folder.
	IconTextMenuItem(const std::string& text, const std::string& localBitmapFilename) :
		wxMenuItem(nullptr, wxID_ANY, text, "")
	{
		SetBitmap(wxutil::GetLocalBitmap(localBitmapFilename));
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
		// On MSW some stock icons come in 32x32 format, so specify 16x16 explicitly
		SetBitmap(wxArtProvider::GetBitmap(artId, wxART_MENU, wxSize(16, 16)));
	}
};

}