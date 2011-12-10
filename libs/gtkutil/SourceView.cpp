#include "SourceView.h"

#include "itextstream.h"
#include "iregistry.h"
#include "imodule.h"

#include <gtksourceviewmm/sourcestyleschememanager.h>

#include "nonmodal.h"

namespace gtkutil
{

SourceView::SourceView(const std::string& language, bool readOnly)
{
	// Create the GtkScrolledWindow
	set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
	set_shadow_type(Gtk::SHADOW_ETCHED_IN);

	// Set the search path to the language files
	std::vector<Glib::ustring> path;
	std::string langFilesDir = getSourceViewDataPath();
	path.push_back(langFilesDir);

	_langManager = gtksourceview::SourceLanguageManager::create();
	_langManager->set_search_path(path);

	Glib::RefPtr<gtksourceview::SourceLanguage> lang = _langManager->get_language(language);

	if (!lang)
	{
		globalErrorStream() << "SourceView: Cannot find language " << language << " in " << langFilesDir << std::endl;
	}

	// Remember the pointers to the textbuffers
	if (lang)
	{
		_buffer = gtksourceview::SourceBuffer::create(lang);
		_buffer->set_highlight_syntax(true);

		setStyleSchemeFromRegistry();
	}
	else
	{
		Glib::RefPtr<Gtk::TextTagTable> table = Gtk::TextTagTable::create();
		_buffer = gtksourceview::SourceBuffer::create(table);
		_buffer->set_highlight_syntax(false);
	}

	_view = Gtk::manage(Gtk::manage(new gtksourceview::SourceView(_buffer)));

	_view->set_size_request(0, -1); // allow shrinking
	_view->set_wrap_mode(Gtk::WRAP_WORD);
	_view->set_editable(!readOnly);

	_view->set_show_line_numbers(true);
	_view->set_auto_indent(true);

	// Use a fixed width font
	PangoFontDescription* fontDesc = pango_font_description_from_string("Monospace");

	if (fontDesc != NULL)
	{
		gtk_widget_modify_font(GTK_WIDGET(_view->gobj()), fontDesc);
	}

	// Use a tab size of 4
	_view->set_tab_width(4);

	widget_connect_escape_clear_focus_widget(GTK_WIDGET(_view->gobj()));

	add(*_view);

	// Subscribe for style scheme changes
    GlobalRegistry().signalForKey(RKEY_SOURCEVIEW_STYLE).connect(
        sigc::mem_fun(*this, &SourceView::setStyleSchemeFromRegistry)
    );
}

void SourceView::setContents(const std::string& newContents)
{
	_buffer->set_text(newContents);
}

std::string SourceView::getContents()
{
	// Extract the script from the input window
	return _buffer->get_text();
}

void SourceView::clear()
{
	setContents("");
}

std::list<std::string> SourceView::getAvailableStyleSchemeIds()
{
	Glib::RefPtr<gtksourceview::SourceStyleSchemeManager> styleSchemeManager = getStyleSchemeManager();
	return styleSchemeManager->get_scheme_ids();
}

std::string SourceView::getSourceViewDataPath()
{
	// Set the search path to the language and style files
	IModuleRegistry& registry = module::GlobalModuleRegistry();
	std::string dataPath = registry.getApplicationContext().getRuntimeDataPath();
	dataPath += "sourceviewer/";

	return dataPath;
}

Glib::RefPtr<gtksourceview::SourceStyleSchemeManager> SourceView::getStyleSchemeManager()
{
	// Set the search path to the language and style files
	std::string langFileDir = getSourceViewDataPath();

	std::vector<Glib::ustring> path;
	path.push_back(langFileDir);

	Glib::RefPtr<gtksourceview::SourceStyleSchemeManager> styleSchemeManager =
		gtksourceview::SourceStyleSchemeManager::get_default();

	styleSchemeManager->set_search_path(path);
	styleSchemeManager->force_rescan();
	
	return styleSchemeManager;
}

void SourceView::setStyleSchemeFromRegistry()
{
	std::string styleName = GlobalRegistry().get(RKEY_SOURCEVIEW_STYLE);

	if (styleName.empty())
	{
		styleName = "classic";
	}

	Glib::RefPtr<gtksourceview::SourceStyleSchemeManager> styleSchemeManager = getStyleSchemeManager();
	Glib::RefPtr<gtksourceview::SourceStyleScheme> scheme = styleSchemeManager->get_scheme(styleName);

	if (scheme)
	{
		_buffer->set_style_scheme(scheme);
	}
}

} // namespace gtkutil
