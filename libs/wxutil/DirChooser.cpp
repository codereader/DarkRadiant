#include "DirChooser.h"

#include <wx/dirdlg.h>
#include <wx/app.h>
#include <wx/frame.h>
#include <wx/display.h>
#include "imainframe.h"

namespace wxutil
{

DirChooser::DirChooser(wxWindow* parent, const std::string& title) :
	_dialog(new wxDirDialog(GlobalMainFrame().getWxTopLevelWindow(), title)),
	_title(title)
{}

DirChooser::~DirChooser()
{
	delete _dialog;
}

void DirChooser::setCurrentPath(const std::string& path)
{
	_dialog->SetPath(path);
}

std::string DirChooser::getSelectedFolderName()
{
	return _dialog->GetPath().ToStdString();
}

std::string DirChooser::display()
{
	int curDisplayIdx = wxDisplay::GetFromWindow(wxTheApp->GetTopWindow());
	wxDisplay curDisplay(curDisplayIdx);

	wxRect rect = curDisplay.GetGeometry();
	int newWidth = static_cast<int>(rect.GetWidth() * 0.5f);
	int newHeight = static_cast<int>(rect.GetHeight() * 0.66f);

	_dialog->SetSize(newWidth, newHeight);
	_dialog->CenterOnScreen();

	if (_dialog->ShowModal() == wxID_OK)
	{
		return getSelectedFolderName();
	}

	return "";
}

} // namespace
