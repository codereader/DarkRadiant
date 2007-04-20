#include "FloatPropertyEditor.h"

#include "ientity.h"
#include "gtkutil/RightAlignment.h"

#include <iostream>
#include <vector>

#include <gtk/gtk.h>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/lexical_cast.hpp>

namespace ui
{

// Main constructor
FloatPropertyEditor::FloatPropertyEditor(Entity* entity,
										 const std::string& key,
										 const std::string& options)
: _entity(entity),
  _key(key)
{
	_widget = gtk_vbox_new(FALSE, 6);
	gtk_container_set_border_width(GTK_CONTAINER(_widget), 6);

	// Split the options string to get min and max values
	std::vector<std::string> values;
	boost::algorithm::split(values, options, boost::algorithm::is_any_of(","));
	if (values.size() != 2)
		return;
		
	// Attempt to cast to min and max floats
	float min, max;
	try {
		min = boost::lexical_cast<float>(values[0]);
		max = boost::lexical_cast<float>(values[1]);
	}
	catch (boost::bad_lexical_cast e) {
		std::cerr 
			<< "[radiant] FloatPropertyEditor failed to parse options string "
			<< "\"" << options << "\"" << std::endl;
		return;		
	}
	
	// Create the HScale and pack into widget
	_scale = gtk_hscale_new_with_range(min, max, 1.0);
	gtk_box_pack_start(GTK_BOX(_widget), _scale, FALSE, FALSE, 0);
	
	// Set the initial value if the entity has one
	float value = 0;
	try {
		value = boost::lexical_cast<float>(_entity->getKeyValue(_key));
	}
	catch (boost::bad_lexical_cast e) { }
	
	gtk_range_set_value(GTK_RANGE(_scale), value);
	
	// Create and pack in the Apply button
	GtkWidget* applyButton = gtk_button_new_from_stock(GTK_STOCK_APPLY);
	g_signal_connect(
		G_OBJECT(applyButton), "clicked", G_CALLBACK(_onApply), this
	);
	gtk_box_pack_end(GTK_BOX(_widget), gtkutil::RightAlignment(applyButton),
					   FALSE, FALSE, 0);
}

/* GTK CALLBACKS */

void FloatPropertyEditor::_onApply(GtkWidget* w, FloatPropertyEditor* self) {
	float value = gtk_range_get_value(GTK_RANGE(self->_scale));
	self->_entity->setKeyValue(
		self->_key, 
		boost::lexical_cast<std::string>(value)
	);
}

}
