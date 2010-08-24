#ifndef GTKUTIL_SOURCEVIEW_H_
#define GTKUTIL_SOURCEVIEW_H_

#include <string>
#include <gtkmm/scrolledwindow.h>
#include <gtksourceviewmm/sourceview.h>
#include <gtksourceviewmm/sourcelanguagemanager.h>

namespace gtkutil 
{

class SourceView :
	public Gtk::ScrolledWindow
{
private:
	gtksourceview::SourceView* _view;
	Glib::RefPtr<gtksourceview::SourceBuffer> _buffer;

	Glib::RefPtr<gtksourceview::SourceLanguageManager> _langManager;

public:
	/**
	 * Constructs a new sourceview with the given language ID as specified
	 * in the .lang files (e.g. "python").
	 *
	 * @readOnly: Set this to TRUE to disallow editing of the text buffer.
	 */
	SourceView(const std::string& language, bool readOnly);

	~SourceView();

	void setContents(const std::string& newContents);

	// Returns the contents of the source buffer
	std::string getContents();

	// Clears the contents of the buffer
	void clear();
};

} // namespace gtkutil

#endif /* GTKUTIL_SOURCEVIEW_H_ */
