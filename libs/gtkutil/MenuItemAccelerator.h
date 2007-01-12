#ifndef MENUITEMACCELERATOR_H_
#define MENUITEMACCELERATOR_H_

#include <gtk/gtkcheckmenuitem.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtklabel.h>

namespace gtkutil {

/* greebo: This returns a label with a right-aligned accelerator for packing into
 * a GtkMenuItem. 
 * 
 * @arguments: 
 * 
 * label: the main label (e.g. "New Map"), this can contain a mnemonic (e.g. "_New Map")
 * accelLabel: the string representing the shortcut (e.g. "Ctrl-N")
 * menuItem: is the menuwidget the mnemonic should be connected to.
 */
class LabelWithAccelerator
{
	// The accelerator text string
	const std::string _label;
	const std::string _accelLabel;
	GtkWidget* _menuItem;
	
public:
	LabelWithAccelerator(const std::string& label, const std::string accelLabel, GtkWidget* menuItem) :
		_label(label),
		_accelLabel(accelLabel),
		_menuItem(menuItem)
	{}
	
	virtual operator GtkWidget* () {
		// Create the left-aligned menulabel
		GtkWidget* menuLabel = gtk_label_new(NULL);
		gtk_label_set_text_with_mnemonic(GTK_LABEL(menuLabel), _label.c_str());
		gtk_misc_set_alignment(GTK_MISC(menuLabel), 0.0, 0.5);
		
		// Set the mnemonic target to the given menuItem 
		gtk_label_set_mnemonic_widget(GTK_LABEL(menuLabel), _menuItem);
		
		// Create the right-aligned shortcut label
		GtkWidget* accelLabel = gtk_label_new(NULL);
		gtk_label_set_text_with_mnemonic(GTK_LABEL(accelLabel), _accelLabel.c_str());
		gtk_misc_set_alignment(GTK_MISC(accelLabel), 1.0, 0.5);
		
		/* Alternative packing algorithm: use a table
		// Create a non-homogeneous 1-row, 2-column table
		GtkTable* table = GTK_TABLE(gtk_table_new(1, 2, false));
		
		gtk_table_set_col_spacings(table, 5);
		gtk_table_attach_defaults(table, menuLabel, 1, 2, 1, 2);
		gtk_table_attach_defaults(table, accelLabel, 1, 2, 1, 2);*/
		
		// Pack these two into a box
		GtkWidget* hbox = gtk_hbox_new(true, 0);
		
		gtk_box_pack_start(GTK_BOX(hbox), menuLabel, true, true, 0);
		gtk_box_pack_start(GTK_BOX(hbox), accelLabel, true, true, 0);
		
		return hbox;
	}
};

/* greebo: Encapsulation for a menu item with a right-aligned accelerator label 
 */
class TextMenuItemAccelerator
{
	const std::string _label;
	const std::string _accelLabel;

public:
	TextMenuItemAccelerator(const std::string& label, const std::string& accelLabel) : 
		_label(label),
		_accelLabel(accelLabel)
	{}
	
	// Operator cast to GtkWidget* for packing into a menu
	virtual operator GtkWidget* () {
		// Create an empty menu item
		GtkWidget* menuItem = gtk_menu_item_new();
		
		// Create the label
		GtkWidget* label = LabelWithAccelerator(_label, _accelLabel, menuItem);
		
		// Pack the label structure into the MenuItem
		gtk_container_add(GTK_CONTAINER(menuItem), GTK_WIDGET(label));
		
		return GTK_WIDGET(menuItem);
	}
};

/* greebo: Encapsulation for a check menu item with a right-aligned accelerator label 
 */
class TextMenuItemAcceleratorToggle
{
	const std::string _label;
	const std::string _accelLabel;

public:
	TextMenuItemAcceleratorToggle(const std::string& label, const std::string& accelLabel) : 
		_label(label),
		_accelLabel(accelLabel)
	{}
	
	// Operator cast to GtkWidget* for packing into a menu
	virtual operator GtkWidget* () {
		// Create an empty menu item
		GtkWidget* menuItem = gtk_check_menu_item_new();
		
		// Create the label
		GtkWidget* label = LabelWithAccelerator(_label, _accelLabel, menuItem);
		
		// Pack the label structure into the MenuItem
		gtk_container_add(GTK_CONTAINER(menuItem), GTK_WIDGET(label));
		
		return GTK_WIDGET(menuItem);
	}
};	
	
}

#endif /*MENUITEMACCELERATOR_H_*/
