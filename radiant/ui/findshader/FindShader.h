#pragma once

#include <string>
#include "icommandsystem.h"
#include "gtkutil/dialog/DialogBase.h"
#include "gtkutil/XmlResourceBasedWidget.h"

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
	public wxutil::DialogBase,
	private wxutil::XmlResourceBasedWidget
{
public:
	// Constructor
	FindAndReplaceShader();

	~FindAndReplaceShader();

	/** greebo: Shows the dialog (allocates on heap, dialog self-destructs)
	 */
	static void ShowDialog(const cmd::ArgumentList& args);

private:
	// This is called to initialise the dialog window / create the widgets
	void populateWindow();

	/** greebo: As the name states, this runs the replace algorithm
	 */
	void performReplace();

	// The callback for the buttons
	void onReplace(wxCommandEvent& ev);
	void onClose(wxCommandEvent& ev);

	void onChooseFind(wxCommandEvent& ev);
	void onChooseReplace(wxCommandEvent& ev);

	void onEntryChanged(wxCommandEvent& ev);
};

} // namespace ui
