#ifndef _GTKUTIL_COMBOBOX_H_
#define _GTKUTIL_COMBOBOX_H_

#include <string>
#include <gtk/gtkcombobox.h>

namespace gtkutil
{

class ComboBox
{
public:

	// Utility method to retrieve the currently selected text from a combo box
	// GTK requires the returned string to be freed to prevent memory leaks
	static std::string getActiveText(GtkComboBox* comboBox)
	{
		gchar* text = gtk_combo_box_get_active_text(comboBox);

		if (text != NULL)
		{
			std::string returnValue(text);
			g_free(text);
			return returnValue;
		}
		else
		{
			return "";
		}
	}
};

} // namespace gtkutil

#endif /* _GTKUTIL_COMBOBOX_H_ */
