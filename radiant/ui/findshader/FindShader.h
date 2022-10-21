#pragma once

#include <string>
#include "icommandsystem.h"
#include "wxutil/XmlResourceBasedWidget.h"
#include <sigc++/trackable.h>

#include "wxutil/DockablePanel.h"

class wxTextCtrl;
class wxButton;
class wxCheckButton;
class wxStaticText;

namespace ui
{

/**
 * greebo: The dialog providing the Find & Replace shader functionality.
 */
class FindAndReplaceShader :
	public wxutil::DockablePanel,
	private wxutil::XmlResourceBasedWidget,
    public sigc::trackable
{
private:
    wxTextCtrl* _lastFocusedEntry;

public:
	FindAndReplaceShader(wxWindow* parent);

private:
    void onShaderClipboardChanged();

	// This is called to initialise the dialog window / create the widgets
	void populateWindow();

	/** greebo: As the name states, this runs the replace algorithm
	 */
	void performReplace();

	// The callback for the buttons
	void onReplace(wxCommandEvent& ev);

	void onChooseFind(wxCommandEvent& ev);
	void onChooseReplace(wxCommandEvent& ev);

    void onChoosePick(wxCommandEvent& ev);

	void onEntryChanged(wxCommandEvent& ev);
    void onEntryFocusChanged(wxFocusEvent& ev);

    std::string getPickHelpText();
};

} // namespace
