#ifndef MODALINFODIALOG_H_
#define MODALINFODIALOG_H_

#include "window/TransientWindow.h"

#include <string>
#include <stdexcept>
#include <boost/shared_ptr.hpp>

namespace Gtk
{
	class Label;
	class ProgressBar;
}

namespace gtkutil
{

/** A simple dialog containing a progress bar and label which can be updated by the
 * owning process to indicate the progress of a task, such as the loading of
 * textures.
 */

class ModalProgressDialog :
	public TransientWindow
{
private:
	// Label with info text
	Gtk::Label* _label;

	// Progress bar
	Gtk::ProgressBar* _progressBar;

	// Flag to indicate the operation has aborted
	bool _aborted;

private:
	// Cancel button callback
	void _onCancel();

	void _onRealize();

	// Process the GTK events to ensure the progress bar/text is updated
	// on screen
	void handleEvents();

protected:
	// Override TransientWindow's delete event
	void _onDeleteEvent();

public:

	/** Constructor accepts window to be modal for and the dialog
	 *  title.
	 */
	ModalProgressDialog(const Glib::RefPtr<Gtk::Window>& parent, const std::string& title);

	/**
	 * Exception thrown when cancel button is pressed.
	 */
	struct OperationAbortedException
	: public std::runtime_error
	{
		OperationAbortedException(const std::string& what)
		: std::runtime_error(what) {}
	};

	/**
	 * Set the text to display in the label, and pulse the progress bar. If the
	 * user has clicked the Cancel button since the last update, this method
	 * will throw an exception to indicate an aborted operation.
	 */
	void setText(const std::string& text);

	/**
	 * Set the text to display in the label, and the completed fraction of the progress bar.
	 * If the user has clicked the Cancel button since the last update, this method
	 * will throw an exception to indicate an aborted operation.
	 */
	void setTextAndFraction(const std::string& text, double fraction);
};
typedef boost::shared_ptr<ModalProgressDialog> ModalProgressDialogPtr;

}

#endif /*MODALINFODIALOG_H_*/
