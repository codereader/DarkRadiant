#pragma once

#include <string>
#include <vector>
#include "iregistry.h"

#include <gtkmm/scrolledwindow.h>

#ifdef HAVE_GTKSOURCEVIEW
#include <gtksourceviewmm/sourceview.h>
#include <gtksourceviewmm/sourcelanguagemanager.h>
#include <gtksourceviewmm/sourcestyleschememanager.h>
#else
#include <gtkmm/textview.h>
#endif

namespace gtkutil
{

namespace
{
	const char* const RKEY_SOURCEVIEW_STYLE = "user/ui/sourceView/style";
}

/// An editable text widget for script source code
class SourceView: public Gtk::ScrolledWindow
{
#ifdef HAVE_GTKSOURCEVIEW
	gtksourceview::SourceView* _view;
	Glib::RefPtr<gtksourceview::SourceBuffer> _buffer;
	Glib::RefPtr<gtksourceview::SourceLanguageManager> _langManager;
private:
	// Utility method to retrieve a style scheme manager, readily set up with the correct paths
	static Glib::RefPtr<gtksourceview::SourceStyleSchemeManager> getStyleSchemeManager();
	static std::string getSourceViewDataPath();
	void setStyleSchemeFromRegistry();
#else
    // With no sourceview available this is just a widget containing a text
    // buffer
    Gtk::TextView* _view;
#endif

public:

	/**
	 * Constructs a new sourceview with the given language ID as specified
	 * in the .lang files (e.g. "python").
	 *
	 * @readOnly: Set this to TRUE to disallow editing of the text buffer.
	 */
	SourceView(const std::string& language, bool readOnly);

    /// Set the text contents of the edit buffer
	void setContents(const std::string& newContents);

	/// Returns the contents of the source buffer
	std::string getContents();

	/// Clears the contents of the buffer
	void clear();

	// Utility method to retrieve a list of all available style scheme names.
	static std::list<std::string> getAvailableStyleSchemeIds();

};

} // namespace gtkutil
