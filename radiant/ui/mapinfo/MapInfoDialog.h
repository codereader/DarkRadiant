#pragma once

#include "icommandsystem.h"
#include "wxutil/dialog/DialogBase.h"

#include <memory>
#include <wx/notebook.h>

namespace ui
{

class MapInfoDialog :
	public wxutil::DialogBase
{
	// The tabs of this dialog
	wxNotebook* _notebook;

	std::unique_ptr<wxImageList> _imageList;

public:
	// Constructor
	MapInfoDialog();

	/**
	 * greebo: Shows the dialog (allocates on heap, dialog self-destructs)
	 */
	static void ShowDialog(const cmd::ArgumentList& args);

private:
	// This is called to create the widgets
	void populateWindow();

	void addTab(wxWindow* panel, const std::string& label, const std::string& icon);
};

} // namespace ui
