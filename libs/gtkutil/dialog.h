#ifndef _GTKUTIL_DIALOG_H_
#define _GTKUTIL_DIALOG_H_

#include <string>

typedef struct _GtkWindow GtkWindow;
typedef struct _GtkLabel GtkLabel;
typedef struct _GtkTable GtkTable;
typedef struct _GtkVBox GtkVBox;
typedef struct _GtkWidget GtkWidget;

GtkLabel* DialogLabel_new(const char* name);
GtkTable* DialogRow_new(const char* name, GtkWidget* widget);
void DialogVBox_packRow(GtkVBox* vbox, GtkWidget* row);

namespace gtkutil
{
	// Display a modal error dialog	
	void errorDialog(const std::string&, GtkWindow* mainFrame);
	
	// Display a modal error dialog and quit immediately
	void fatalErrorDialog(const std::string&, GtkWindow* mainFrame);

	/**
	 * Display a text entry dialog with the given title and prompt text. Returns a
	 * std::string with the entered value, or throws EntryAbortedException if the
	 * dialog was cancelled. The text entry will be filled with the given defaultText 
	 * at start.
	 */
    const std::string textEntryDialog(const std::string& title, 
    								  const std::string& prompt,
									  const std::string& defaultText,
    								  GtkWindow* mainFrame);
}

#endif /* _GTKUTIL_DIALOG_H_ */
