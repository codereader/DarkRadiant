#pragma once

#include <wx/artprov.h>
#include <wx/menuitem.h>

namespace wxutil
{

class IconTextMenuItem :
	public wxMenuItem
{
public:
	IconTextMenuItem(const std::string& text, const std::string& localBitmapFilename) :
		wxMenuItem(NULL, wxID_ANY, text, "")
	{
		SetBitmap(wxArtProvider::GetBitmap("darkradiant:" + localBitmapFilename));
	}
};

}