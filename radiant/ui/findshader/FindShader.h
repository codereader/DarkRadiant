#pragma once

#include <string>
#include <sigc++/connection.h>
#include "wxutil/XmlResourceBasedWidget.h"

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
	private wxutil::XmlResourceBasedWidget
{
private:
    wxTextCtrl* _lastFocusedEntry;

    sigc::connection _shaderClipboardConn;

public:
	FindAndReplaceShader(wxWindow* parent);
    ~FindAndReplaceShader() override;

protected:
    void onPanelActivated() override;
    void onPanelDeactivated() override;

private:
    void connectListeners();
    void disconnectListeners();

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
