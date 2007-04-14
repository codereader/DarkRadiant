#include "FloatPropertyEditor.h"

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
: _widget(gtk_vbox_new(FALSE, 6)),
  _entity(entity),
  _key(key)
{
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
	GtkWidget* scale = gtk_hscale_new_with_range(min, max, 1.0);
	gtk_box_pack_start(GTK_BOX(_widget), scale, FALSE, FALSE, 0);
	
}

}
