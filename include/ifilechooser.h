#ifndef _IFILECHOOSER_H_
#define _IFILECHOOSER_H_

#include <boost/shared_ptr.hpp>

namespace Gtk { class Widget; }

namespace ui
{

/**
 * The FileChooser class can be used to query a path from the user.
 * Use the GlobalRadiant module to acquire a new instance of this class.
 */
class IFileChooser
{
public:
	/**
	 * greebo: A Preview class can be attached to a FileChooser (in "open" mode),
	 * to allow for adding and updating a preview widget to the dialog.
	 * The Preview object must provide two methods, one for retrieving
	 * the preview widget for addition to the dialog, and one
	 * update method which gets called as soon as the dialog emits the
	 * selection change signal.
	 */
	class Preview
	{
	public:
		virtual ~Preview() {}

		// Retrieve the preview widget for packing into the dialog
		// Ownership of the widget will remain in the Preview class.
		virtual Gtk::Widget& getPreviewWidget() = 0;

		/**
		 * Gets called whenever the user changes the file selection.
		 * Note: this method must call the setPreviewActive() method on the
		 * FileChooser class to indicate whether the widget is active or not.
		 */
		virtual void onFileSelectionChanged(const std::string& newFileName, IFileChooser& fileChooser) = 0;
	};
	typedef boost::shared_ptr<Preview> PreviewPtr;

public:
	virtual ~IFileChooser() {}

	// Lets the dialog start at a certain path
	virtual void setCurrentPath(const std::string& path) = 0;

	// Pre-fills the currently selected file
	virtual void setCurrentFile(const std::string& file) = 0;

	/**
	 * FileChooser in "open" mode (see constructor) can have one
	 * single preview attached to it. The Preview object will
	 * get notified on selection changes to update the widget it provides.
	 */
	virtual void attachPreview(const PreviewPtr& preview) = 0;

	/**
	 * Returns the selected filename (default extension
	 * will be added if appropriate).
	 */
	virtual std::string getSelectedFileName() = 0;

	/**
	 * greebo: Displays the dialog and enters the GTK main loop.
	 * Returns the filename or "" if the user hit cancel.
	 *
	 * The returned file name is normalised using the os::standardPath() method.
	 */
	virtual std::string display() = 0;

	// Public function for Preview objects. These must set the "active" state
	// of the preview when the onFileSelectionChange() signal is emitted.
	virtual void setPreviewActive(bool active) = 0;
};
typedef boost::shared_ptr<IFileChooser> IFileChooserPtr;

} // namespace ui

#endif /* _IFILECHOOSER_H_ */
