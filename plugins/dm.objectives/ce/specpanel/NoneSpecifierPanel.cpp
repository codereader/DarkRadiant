#include "NoneSpecifierPanel.h"

#include <gtk/gtklabel.h>

namespace objectives
{

namespace ce
{

// Registration helper
NoneSpecifierPanel::RegHelper NoneSpecifierPanel::_regHelper;

// Constructor
NoneSpecifierPanel::NoneSpecifierPanel()
{
	_widget = gtk_label_new("This specifier type does not take a value.");
}

}

}
