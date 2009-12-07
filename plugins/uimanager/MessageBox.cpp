#include "MessageBox.h"

#include "itextstream.h"

#include <gtk/gtkvbox.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkstock.h>

#include "gtkutil/LeftalignedLabel.h"

namespace ui
{

MessageBox::MessageBox(std::size_t id, DialogManager& owner, 
					   const std::string& title, const std::string& text,
					   IDialog::MessageType type) :
	Dialog(id, owner, title),
	_text(text),
	_type(type)
{}

// Constructs the dialog (adds buttons, text and icons)
void MessageBox::construct()
{
	// Base class is adding the buttons
	Dialog::construct();

	// Add an hbox for the icon and the content
	GtkWidget* hbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(_vbox), hbox, TRUE, TRUE, 0);

	// Add the icon
	GtkWidget* icon = createIcon();

	if (icon != NULL)
	{
		gtk_box_pack_start(GTK_BOX(hbox), icon, FALSE, FALSE, 0);
	}

	// Add the text
	gtk_box_pack_start(GTK_BOX(hbox), gtkutil::LeftAlignedLabel(_text), TRUE, TRUE, 0);
}

GtkWidget* MessageBox::createIcon()
{
	const gchar* stockId = NULL;

	switch (_type)
	{
	case MESSAGE_CONFIRM:
		stockId = GTK_STOCK_DIALOG_INFO;
		break;
	case MESSAGE_ASK:
		stockId = GTK_STOCK_DIALOG_QUESTION;
		break;
	case MESSAGE_WARNING:
		stockId = GTK_STOCK_DIALOG_WARNING;
		break;
	case MESSAGE_ERROR:
		stockId = GTK_STOCK_DIALOG_ERROR;
		break;
	default:
		stockId = GTK_STOCK_DIALOG_INFO;
	};

	return (stockId != NULL) ? gtk_image_new_from_stock(stockId, GTK_ICON_SIZE_DIALOG) : NULL;
}

// Override Dialog::createButtons() to add the custom ones
GtkWidget* MessageBox::createButtons()
{
	GtkWidget* buttonHBox = gtk_hbox_new(FALSE, 6);

	// Pack the button hbox into the window
	gtk_box_pack_end(GTK_BOX(_vbox), buttonHBox, FALSE, FALSE, 0);

	if (_type == MESSAGE_CONFIRM || _type == MESSAGE_WARNING || _type == MESSAGE_ERROR)
	{
		// Add an OK button
		GtkWidget* okButton = gtk_button_new_from_stock(GTK_STOCK_OK);
		g_signal_connect(G_OBJECT(okButton), "clicked", G_CALLBACK(onOK), this);
		gtk_box_pack_end(GTK_BOX(buttonHBox), okButton, FALSE, FALSE, 0);
	}
	else if (_type == MESSAGE_ASK)
	{
		// YES button
		GtkWidget* yesButton = gtk_button_new_from_stock(GTK_STOCK_YES);
		g_signal_connect(G_OBJECT(yesButton), "clicked", G_CALLBACK(onYes), this);
		gtk_box_pack_end(GTK_BOX(buttonHBox), yesButton, FALSE, FALSE, 0);

		// NO button
		GtkWidget* noButton = gtk_button_new_from_stock(GTK_STOCK_NO);
		g_signal_connect(G_OBJECT(noButton), "clicked", G_CALLBACK(onNo), this);
		gtk_box_pack_end(GTK_BOX(buttonHBox), noButton, FALSE, FALSE, 0);
	}
	else
	{
		globalErrorStream() << "Invalid message type encountered: " << _type << std::endl;
	}
	
	return buttonHBox;
}

void MessageBox::onYes(GtkWidget* widget, MessageBox* self)
{
	self->_result = RESULT_YES;
	self->hide(); // breaks gtk_main()
}

void MessageBox::onNo(GtkWidget* widget, MessageBox* self)
{
	self->_result = RESULT_NO;
	self->hide(); // breaks gtk_main()
}

} // namespace ui
