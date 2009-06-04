#include "SourceView.h"

#include "itextstream.h"
#include "iregistry.h"
#include <gtksourceview/gtksourcelanguagemanager.h>
#include <gtksourceview/gtksourcestyleschememanager.h>
#include <gtksourceview/gtksourceview.h>

#include "nonmodal.h"

namespace gtkutil
{

SourceView::SourceView(const std::string& language, bool readOnly)
{
	// Set the search path to the language and style files
	gchar* directories[2];

	std::string langFilesDir = GlobalRegistry().get(RKEY_APP_PATH) + "sourceviewer/";

	directories[0] = const_cast<gchar*>(langFilesDir.c_str()); // stupid GtkSourceLanguageManager is expecting non-const gchar* pointer...
	directories[1] = NULL;

	GtkSourceStyleSchemeManager* styleManager = gtk_source_style_scheme_manager_get_default();
	gtk_source_style_scheme_manager_append_search_path(styleManager, directories[0]);
	gtk_source_style_scheme_manager_force_rescan(styleManager);

	GtkSourceLanguageManager* manager = gtk_source_language_manager_new();
	gtk_source_language_manager_set_search_path(manager, directories);

	GtkSourceLanguage* lang = gtk_source_language_manager_get_language(manager, language.c_str());

	if (lang == NULL)
	{
		globalErrorStream() << "SourceView: Cannot find language " << language << std::endl;
	}

	// Remember the pointers to the textbuffers
	_buffer = gtk_source_buffer_new_with_language(lang);
	gtk_source_buffer_set_highlight_syntax(_buffer, TRUE);

	_view = GTK_SOURCE_VIEW(gtk_source_view_new_with_buffer(_buffer));

	gtk_widget_set_size_request(GTK_WIDGET(_view), 0, -1); // allow shrinking
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(_view), GTK_WRAP_WORD);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(_view), readOnly ? FALSE : TRUE);

	gtk_source_view_set_show_line_numbers(GTK_SOURCE_VIEW(_view), TRUE);
	gtk_source_view_set_auto_indent(GTK_SOURCE_VIEW(_view), TRUE);

	widget_connect_escape_clear_focus_widget(GTK_WIDGET(_view));
}

SourceView::operator GtkWidget* () const
{
	return GTK_WIDGET(_view);
}

void SourceView::setContents(const std::string& newContents)
{
	gtk_text_buffer_set_text(GTK_TEXT_BUFFER(_buffer), newContents.c_str(), -1);
}

std::string SourceView::getContents()
{
	GtkTextIter start;
	GtkTextIter end;

	gtk_text_buffer_get_bounds(GTK_TEXT_BUFFER(_buffer), &start, &end);

	// Extract the script from the input window
	gchar* text = gtk_text_buffer_get_text(GTK_TEXT_BUFFER(_buffer), &start, &end, TRUE);

	// Convert to std::string, free the GLIB stuff and return
	std::string contents(text);
	g_free(text);

	return contents;
}

void SourceView::clear()
{
	gtk_text_buffer_set_text(GTK_TEXT_BUFFER(_buffer), "", -1);
}

} // namespace gtkutil
