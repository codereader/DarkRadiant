#include "PrefPage.h"

#include "itextstream.h"
#include "stream/textfilestream.h"
#include <gtk/gtk.h>

#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/dialog.h"
#include "gtkutil/SerialisableWidgets.h"

#include <iostream>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include "gtkmisc.h"
#include "modulesystem/ApplicationContextImpl.h"

namespace ui {

	namespace {
		typedef std::vector<std::string> StringVector;
	}

PrefPage::PrefPage(
		const std::string& name, 
		const std::string& parentPath, 
		GtkWidget* notebook,
		gtkutil::RegistryConnector& connector) :
	_name(name),
	_path(parentPath),
	_notebook(notebook),
	_connector(connector)
{
	// If this is not the root item, add a leading slash
	_path += (!_path.empty()) ? "/" : "";
	_path += _name;
	
	// Create the overall vbox
	_pageWidget = gtk_vbox_new(FALSE, 6);
	gtk_container_set_border_width(GTK_CONTAINER(_pageWidget), 12);
	
	// Create the label
	_titleLabel = gtkutil::LeftAlignedLabel(std::string("<b>") + _name + " Settings</b>");
	gtk_box_pack_start(GTK_BOX(_pageWidget), _titleLabel, FALSE, FALSE, 0);
	
	// Create the VBOX for all the client widgets
	_vbox = gtk_vbox_new(FALSE, 6);
	
	// Create the alignment for the client vbox and pack it
	GtkWidget* alignment = gtkutil::LeftAlignment(_vbox, 18, 1.0);
	gtk_box_pack_start(GTK_BOX(_pageWidget), alignment, FALSE, FALSE, 0);
	
	// Append the whole vbox as new page to the notebook
	gtk_notebook_append_page(GTK_NOTEBOOK(_notebook), _pageWidget, NULL);
}

void PrefPage::setTitle(const std::string& title) {
	gtk_label_set_markup(
		GTK_LABEL(_titleLabel),
		std::string("<b>" + title + "</b>").c_str()
	);
}

std::string PrefPage::getPath() const {
	return _path;
}

std::string PrefPage::getName() const {
	return _name;
}

/** greebo: Returns the widget that can be used to determine
 * 			the notebook page number.
 */
GtkWidget* PrefPage::getWidget() {
	return _pageWidget;
}

void PrefPage::foreachPage(Visitor& visitor) {
	for (unsigned int i = 0; i < _children.size(); i++) {
		// Visit this instance
		visitor.visit(_children[i]);

		// Pass the visitor recursively
		_children[i]->foreachPage(visitor);
	}
}

/* greebo: This adds a checkbox and connects it to an XMLRegistry key.
 * @returns: the pointer to the created GtkWidget */
GtkWidget* PrefPage::appendCheckBox(const std::string& name,
                                    const std::string& flag,
                                    const std::string& registryKey) 
{
	// Create a new checkbox with the given caption and display it
	GtkWidget* check = gtk_check_button_new_with_label(flag.c_str());
	
	// Connect the registry key to this toggle button
   using namespace gtkutil;
	_connector.addObject(
      registryKey,
      SerialisableWidgetWrapperPtr(new SerialisableToggleButton(check))
   );
	
	DialogVBox_packRow(GTK_VBOX(_vbox), GTK_WIDGET(DialogRow_new(name.c_str(), check)));
	return check;
}

/* greebo: This adds a horizontal slider to the internally referenced VBox and connects
 * it to the given registryKey. */
void PrefPage::appendSlider(const std::string& name, const std::string& registryKey, bool drawValue,
                            double value, double lower, double upper, double step_increment, double page_increment, double page_size) 
{
	// Create a new adjustment with the boundaries <lower> and <upper> and all the increments
	GtkObject* adj = gtk_adjustment_new(value, lower, upper, step_increment, page_increment, page_size);
	
	// Connect the registry key to this adjustment
    using namespace gtkutil;
	_connector.addObject(
        registryKey,
        StringSerialisablePtr(new SerialisableAdjustment(adj))
    );
	
	// scale
	GtkWidget* alignment = gtk_alignment_new(0.0, 0.5, 1.0, 0.0);
	gtk_widget_show(alignment);
	
	GtkWidget* scale = gtk_hscale_new(GTK_ADJUSTMENT(adj));
	gtk_scale_set_value_pos(GTK_SCALE(scale), GTK_POS_LEFT);
	gtk_widget_show(scale);
	gtk_container_add(GTK_CONTAINER(alignment), scale);
	
	gtk_scale_set_draw_value(GTK_SCALE (scale), drawValue);
	int digits = (step_increment < 1.0f) ? 2 : 0; 
	gtk_scale_set_digits(GTK_SCALE (scale), digits);
	
	GtkTable* row = DialogRow_new(name.c_str(), alignment);
	DialogVBox_packRow(GTK_VBOX(_vbox), GTK_WIDGET(row));
}

/* greebo: Use this to add a dropdown selection box with the given list of strings as captions. The value
 * stored in the registryKey is used to determine the currently selected combobox item */
void PrefPage::appendCombo(const std::string& name,
                           const std::string& registryKey,
                           const ComboBoxValueList& valueList,
                           bool storeValueNotIndex)
{
	GtkWidget* alignment = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
	
    // Create a new combo box of the correct type
    using boost::shared_ptr;
    using namespace gtkutil;

    SerialisableWidgetWrapperPtr combo;
    if (storeValueNotIndex)
    {
        combo = SerialisableWidgetWrapperPtr(
            new gtkutil::SerialisableComboBox_Text
        );
    }
    else
    {
        combo = SerialisableWidgetWrapperPtr(
            new gtkutil::SerialisableComboBox_Index
        );
    }

    // Add all the string values to the combo box
    for (ComboBoxValueList::const_iterator i = valueList.begin();
         i != valueList.end();
         i++) 
    {
        gtk_combo_box_append_text(
            GTK_COMBO_BOX(combo->getWidget()), i->c_str()
        );
    }

    // Connect the registry key to the newly created combo box
    _connector.addObject(registryKey, combo);

    // Add it to the container 
    gtk_container_add(GTK_CONTAINER(alignment), combo->getWidget());
	
	// Add the widget to the dialog row
	GtkTable* row = DialogRow_new(name.c_str(), alignment);
	DialogVBox_packRow(GTK_VBOX(_vbox), GTK_WIDGET(row));
}

/* greebo: Appends an entry field with <name> as caption which is connected to the given registryKey
 */
GtkWidget* PrefPage::appendEntry(const std::string& name, const std::string& registryKey) {
	GtkWidget* alignment = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
	gtk_widget_show(alignment);

	GtkWidget* entry = gtk_entry_new();
	gtk_entry_set_width_chars(
      GTK_ENTRY(entry), 
      static_cast<gint>(std::max(GlobalRegistry().get(registryKey).size(), std::size_t(10)))
   );
	gtk_container_add(GTK_CONTAINER(alignment), GTK_WIDGET(entry));
	
	// Connect the registry key to the newly created input field
   using namespace gtkutil;
	_connector.addObject(
      registryKey,
      SerialisableWidgetWrapperPtr(new SerialisableTextEntry(entry))
   );

	GtkTable* row = DialogRow_new(name.c_str(), GTK_WIDGET(alignment));
	DialogVBox_packRow(GTK_VBOX(_vbox), GTK_WIDGET(row));
	return GTK_WIDGET(entry);
}

/* greebo: Appends a label field with the given caption (static)
 */
GtkWidget* PrefPage::appendLabel(const std::string& caption) {
	GtkLabel* label = GTK_LABEL(gtk_label_new(""));
	gtk_label_set_markup(label, caption.c_str());
		
	DialogVBox_packRow(GTK_VBOX(_vbox), GTK_WIDGET(label));
	return GTK_WIDGET(label);
}

// greebo: Adds a PathEntry to choose files or directories (depending on the given boolean)
GtkWidget* PrefPage::appendPathEntry(const std::string& name, const std::string& registryKey, bool browseDirectories) {
	PathEntry pathEntry = PathEntry_new(GlobalRegistry().get(RKEY_BITMAPS_PATH));
	g_signal_connect(
		G_OBJECT(pathEntry.m_button), 
		"clicked", 
		G_CALLBACK(browseDirectories ? button_clicked_entry_browse_directory : button_clicked_entry_browse_file), 
		pathEntry.m_entry
	);

	// Connect the registry key to the newly created input field
   using namespace gtkutil;
	_connector.addObject(
      registryKey,
      SerialisableWidgetWrapperPtr(
         new SerialisableTextEntry(GTK_WIDGET(pathEntry.m_entry))
      )
   );

	GtkTable* row = DialogRow_new(name.c_str(), GTK_WIDGET(pathEntry.m_frame));
	DialogVBox_packRow(GTK_VBOX(_vbox), GTK_WIDGET(row));

	return GTK_WIDGET(row);
}

GtkSpinButton* Spinner_new(double value, double lower, double upper, int fraction) {
	double step = 1.0 / double(fraction);
	unsigned int digits = 0;
	for (;fraction > 1; fraction /= 10) {
		++digits;
	}
	GtkSpinButton* spin = GTK_SPIN_BUTTON(gtk_spin_button_new(GTK_ADJUSTMENT(gtk_adjustment_new(value, lower, upper, step, 10, 0)), step, digits));
	gtk_widget_show(GTK_WIDGET(spin));
	gtk_widget_set_size_request(GTK_WIDGET(spin), 64, -1);
	return spin;
}

/* greebo: Appends an entry field with spinner buttons which retrieves its value from the given
 * RegistryKey. The lower and upper values have to be passed as well.
 */
GtkWidget* PrefPage::appendSpinner(const std::string& name, const std::string& registryKey,
                                   double lower, double upper, int fraction) {
	// Load the initial value (maybe unnecessary, as the value is loaded upon dialog show)
	float value = GlobalRegistry().getFloat(registryKey); 
	
	GtkWidget* alignment = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
	gtk_widget_show(alignment);
	
	GtkSpinButton* spin = Spinner_new(value, lower, upper, fraction);
	gtk_container_add(GTK_CONTAINER(alignment), GTK_WIDGET(spin));
	
	GtkTable* row = DialogRow_new(name.c_str(), GTK_WIDGET(alignment));
	
	// Connect the registry key to the newly created input field
   using namespace gtkutil;
	_connector.addObject(
      registryKey,
      SerialisableWidgetWrapperPtr(
         new SerialisableSpinButton(GTK_WIDGET(spin))
      )
   );

	DialogVBox_packRow(GTK_VBOX(_vbox), GTK_WIDGET(row));
	return GTK_WIDGET(spin);
}

PrefPagePtr PrefPage::createOrFindPage(const std::string& path) {
	// Split the path into parts
	StringVector parts;
	boost::algorithm::split(parts, path, boost::algorithm::is_any_of("/"));
	
	if (parts.size() == 0) {
		std::cout << "Warning: Could not resolve preference path: " << path << "\n";
		return PrefPagePtr();
	}
	
	PrefPagePtr child;
	
	// Try to lookup the page in the child list
	for (unsigned int i = 0; i < _children.size(); i++) {
		if (_children[i]->getName() == parts[0]) {
			child = _children[i];
			break;
		}
	}
	
	if (child == NULL) {
		// No child found, create a new page and add it to the list
		child = PrefPagePtr(new PrefPage(parts[0], _path, _notebook, _connector));
		_children.push_back(child);
	}
	
	// We now have a child with this name, do we have a leaf?
	if (parts.size() > 1) {
		// We have still more parts, split off the first part
		std::string subPath("");
		for (unsigned int i = 1; i < parts.size(); i++) {
			subPath += (subPath.empty()) ? "" : "/";
			subPath += parts[i];
		}
		// Pass the call to the child
		return child->createOrFindPage(subPath);
	}
	else {
		// We have found a leaf, return the child page		
		return child;
	}
}

} // namespace ui
