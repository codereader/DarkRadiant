#pragma once

#include "iuimanager.h"
#include <map>
#include "wxutil/event/SingleIdleCallback.h"

#include <wx/panel.h>
#include <wx/stattext.h>

namespace ui
{

class StatusBarManager :
	public IStatusBarManager,
	protected wxutil::SingleIdleCallback
{
	struct StatusBarElement
	{
		// The toplevel container
		wxWindow* toplevel;

		// If this status bar element is a label, this is not NULL
		wxStaticText* label;

		// The text for this label, gets filled in when GTK is idle
		std::string text;

		StatusBarElement(wxWindow* _toplevel) :
			toplevel(_toplevel),
			label(NULL)
		{}

		StatusBarElement(wxWindow* _toplevel, wxStaticText* _label) :
			toplevel(_toplevel),
			label(_label)
		{}
	};
	typedef std::shared_ptr<StatusBarElement> StatusBarElementPtr;

	// Editable status bar widgets
	typedef std::map<std::string, StatusBarElementPtr> ElementMap;
	ElementMap _elements;

	// Container
	typedef std::map<int, StatusBarElementPtr> PositionMap;
	PositionMap _positions;

	// only used during startup
	wxFrame* _tempParent; 

	// The main status bar
	wxPanel* _statusBar;

public:
	StatusBarManager();

	~StatusBarManager();

	/**
	 * Get the status bar widget, for packing into the main window.
	 */
	wxWindow* getStatusBar() override;

	/**
	 * greebo: This adds a named element to the status bar. Pass the widget
	 * which should be added and specify the position order.
	 *
	 * @name: the name of the element (can be used for later lookup).
	 * @widget: the widget to pack.
	 * @pos: the position to insert. Use POS_FRONT or POS_BACK to put the element
	 *       at the front or back of the status bar container.
	 */
	void addElement(const std::string& name, wxWindow* widget, int pos) override;

	/**
	 * Returns a named status bar widget, previously added by addElement().
	 *
	 * @returns: NULL if the named widget does not exist.
	 */
	wxWindow* getElement(const std::string& name) override;

	/**
	 * greebo: A specialised method, adding a named text element.
	 * Use the setText() method to update this element.
	 *
	 * @name: the name for this element (can be used as key for the setText() method).
	 * @icon: the icon file to pack into the element, relative the BITMAPS_PATH. Leave empty
	 *        if no icon is desired.
	 * @pos: the position to insert. Use POS_FRONT or POS_BACK to put the element
	 *       at the front or back of the status bar container.
 	 */
	void addTextElement(const std::string& name, const std::string& icon, int pos, 
						const std::string& description) override;

	/**
	 * Updates the content of the named text element. The name must refer to
	 * an element previously added by addTextElement().
	 */
    void setText(const std::string& name, const std::string& text, bool immediateUpdate) override;

	void onRadiantShutdown();

protected:
	// Gets called when the app is idle - this fills in the status text
	void onIdle() override;

private:
	// Returns an integer position which is not used yet.
	int getFreePosition(int desiredPosition);

	// Rebuilds the status bar after widget addition
	void rebuildStatusBar();
};

} // namespace ui
