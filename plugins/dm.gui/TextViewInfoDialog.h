#ifndef TEXT_VIEW_INFO_DIALOG_H
#define TEXT_VIEW_INFO_DIALOG_H

#include "gtkutil/window/BlockingTransientWindow.h"
#include "gtkutil/ScrolledFrame.h"
#include <gtk/gtk.h>
#include <string.h>

namespace ui
{

///////////////////////////// TextViewInfoDialog:
// Small Info-Dialog showing text in a scrolled, non-editable textview and an ok button.
class TextViewInfoDialog :
	public gtkutil::BlockingTransientWindow
{
private:
	GtkTextBuffer* _bfr;

public:
	TextViewInfoDialog(const std::string& title, const std::string& text ,guint win_width = 650, guint win_height = 500, GtkWindow* parent = NULL) :
		gtkutil::BlockingTransientWindow( title,
			(parent == 0) ? GlobalMainFrame().getTopLevelWindow() : parent
		)
	{
		// Set the default border width in accordance to the HIG
		gtk_container_set_border_width(GTK_CONTAINER(getWindow()), 12);
		gtk_window_set_type_hint(GTK_WINDOW(getWindow()), GDK_WINDOW_TYPE_HINT_DIALOG);
		gtk_window_set_default_size(GTK_WINDOW(getWindow()), win_width, win_height);

		// Create the textview and add the text.
		GtkWidget* textView = gtk_text_view_new();
		gtk_text_view_set_editable(GTK_TEXT_VIEW(textView), FALSE);
		_bfr = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textView));

		gtk_text_buffer_set_text(_bfr, text.c_str(), static_cast<gint>(text.size()));

		// Create the button and connect the signal
		GtkWidget* okButton = gtk_button_new_from_stock(GTK_STOCK_OK);
		g_signal_connect(G_OBJECT(okButton), "clicked", G_CALLBACK(onOk), this);

		GtkWidget* alignment = gtk_alignment_new(0.5,1,0,0);
		gtk_container_add(GTK_CONTAINER(alignment), okButton);

		// Create a vbox and add the elements.
		GtkWidget* vbox = gtk_vbox_new(FALSE, 6);
		gtk_box_pack_start(GTK_BOX(vbox), gtkutil::ScrolledFrame(textView), TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), alignment, FALSE, FALSE, 0);

		gtk_container_add(GTK_CONTAINER(getWindow()), vbox);
	}

	// Appends the given text to the textview.
	void add(const std::string& text)
	{
		gtk_text_buffer_insert_at_cursor(_bfr, text.c_str(), static_cast<gint>(text.size()));
	}

	static void onOk(GtkWidget* widget, XDataSelector* self)
	{
		self->destroy();
	}
};

} // namespace

#endif TEXT_VIEW_INFO_DIALOG_H
