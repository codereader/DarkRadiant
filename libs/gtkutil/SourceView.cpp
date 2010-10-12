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

	// Set the search path to the language and style files
	std::string langFilesDir = module::GlobalModuleRegistry()
                               .getApplicationContext()
                               .getRuntimeDataPath() + "sourceviewer/";

	std::vector<Glib::ustring> path;
	path.push_back(langFilesDir);

	Glib::RefPtr<gtksourceview::SourceStyleSchemeManager> styleSchemeManager = 
		gtksourceview::SourceStyleSchemeManager::get_default();

	styleSchemeManager->set_search_path(path);
	styleSchemeManager->force_rescan();
	
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
}

SourceView::~SourceView()
{}

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

} // namespace gtkutil
