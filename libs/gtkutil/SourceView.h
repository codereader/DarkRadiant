#ifndef GTKUTIL_SOURCEVIEW_H_
#define GTKUTIL_SOURCEVIEW_H_

#include <string>
#include <vector>
#include "iregistry.h"

#include <gtkmm/scrolledwindow.h>
#include <gtksourceviewmm/sourceview.h>
#include <gtksourceviewmm/sourcelanguagemanager.h>
#include <gtksourceviewmm/sourcestyleschememanager.h>

namespace gtkutil
{

namespace
{
	const char* const RKEY_SOURCEVIEW_STYLE = "user/ui/sourceView/style";
}

class SourceView: public Gtk::ScrolledWindow
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

	void setContents(const std::string& newContents);

	// Returns the contents of the source buffer
	std::string getContents();

	// Clears the contents of the buffer
	void clear();

	// Utility method to retrieve a list of all available style scheme names.
	static std::list<std::string> getAvailableStyleSchemeIds();

private:
	// Utility method to retrieve a style scheme manager, readily set up with the correct paths
	static Glib::RefPtr<gtksourceview::SourceStyleSchemeManager> getStyleSchemeManager();

	static std::string getSourceViewDataPath();

	void setStyleSchemeFromRegistry();
};

} // namespace gtkutil

#endif /* GTKUTIL_SOURCEVIEW_H_ */
