#include "PathEntry.h"

#include "iregistry.h"
#include "i18n.h"

#include <wx/sizer.h>
#include <wx/bmpbuttn.h>
#include <wx/textctrl.h>
#include <wx/bitmap.h>

#include "FileChooser.h"
#include "DirChooser.h"
#include "os/path.h"

namespace wxutil
{

PathEntry::PathEntry(wxWindow* parent, bool foldersOnly) :
	wxPanel(parent, wxID_ANY)
{
	SetSizer(new wxBoxSizer(wxHORIZONTAL));

	// path entry
	_entry = new wxTextCtrl(this, wxID_ANY, "");

	// Generate browse button image
	std::string fullFileName = GlobalRegistry().get(RKEY_BITMAPS_PATH) + "ellipsis.png";

	wxImage image(fullFileName);

	// browse button
	_button = new wxBitmapButton(this, wxID_ANY, wxBitmap(image));

	// Connect the button
	if (foldersOnly)
	{
		_button->Connect(wxEVT_BUTTON, wxCommandEventHandler(PathEntry::onBrowseFolders), NULL, this);
	}
	else
	{
		_button->Connect(wxEVT_BUTTON, wxCommandEventHandler(PathEntry::onBrowseFiles), NULL, this);
	}

	GetSizer()->Add(_entry, 1, wxEXPAND | wxRIGHT, 6);
	GetSizer()->Add(_button, 0, wxEXPAND);
}

void PathEntry::setValue(const std::string& val)
{
	_entry->SetValue(val);
}

std::string PathEntry::getValue() const
{
	return _entry->GetValue().ToStdString();
}

wxTextCtrl* PathEntry::getEntryWidget()
{
	return _entry;
}

void PathEntry::onBrowseFiles(wxCommandEvent& ev)
{
	wxWindow* topLevel = wxGetTopLevelParent(this);

    wxutil::FileChooser fileChooser(topLevel, _("Choose File"), true);

	fileChooser.setCurrentPath(getValue());

	std::string filename = fileChooser.display();

	topLevel->Show();

	if (!filename.empty())
	{
		setValue(filename);
	}
}

void PathEntry::onBrowseFolders(wxCommandEvent& ev)
{
	wxWindow* topLevel = wxGetTopLevelParent(this);

    wxutil::DirChooser dirChooser(topLevel, _("Choose Directory"));

	std::string curEntry = getValue();

	if (!path_is_absolute(curEntry.c_str()))
	{
		curEntry.clear();
	}

	dirChooser.setCurrentPath(curEntry);

	std::string filename = dirChooser.display();

	topLevel->Show();

	if (!filename.empty())
	{
		setValue(filename);
	}
}

} // namespace wxutil
