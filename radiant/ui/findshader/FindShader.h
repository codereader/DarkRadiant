#ifndef FINDSHADER_H_
#define FINDSHADER_H_

#include <string>
#include "icommandsystem.h"
#include "gtkutil/window/BlockingTransientWindow.h"

namespace Gtk
{
	class Entry;
	class Button;
	class CheckButton;
	class Label;
}

/* greebo: The dialog providing the Find & Replace shader functionality.
 *
 * Note: Show the dialog by instantiating it. It automatically enters a
 * GTK main loop after show().
 */
namespace ui
{

class FindAndReplaceShader :
	public gtkutil::BlockingTransientWindow
{
private:
	// The entry fields
	Gtk::Entry* _findEntry;
	Gtk::Entry* _replaceEntry;

	// The buttons to select the shader
	Gtk::Button* _findSelectButton;
	Gtk::Button* _replaceSelectButton;

	// The checkbox "Search Selected Only"
	Gtk::CheckButton* _selectedOnly;

	// The counter "x shaders replaced."
	Gtk::Label* _counterLabel;

public:
	// Constructor
	FindAndReplaceShader();

	~FindAndReplaceShader();

	/** greebo: Shows the dialog (allocates on heap, dialog self-destructs)
	 */
	static void showDialog(const cmd::ArgumentList& args);

private:
	// This is called to initialise the dialog window / create the widgets
	void populateWindow();

	/** greebo: As the name states, this runs the replace algorithm
	 */
	void performReplace();

	// Helper method to create the OK/Cancel button
	Gtk::Widget& createButtons();

	// The callback for the buttons
	void onReplace();
	void onClose();

	void onChooseFind();
	void onChooseReplace();

	void onFindChanged();
	void onReplaceChanged();
}; // class FindAndReplaceShader

} // namespace ui

#endif /*FINDSHADER_H_*/
