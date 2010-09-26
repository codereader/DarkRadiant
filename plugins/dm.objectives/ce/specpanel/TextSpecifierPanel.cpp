#include "TextSpecifierPanel.h"

namespace objectives
{

namespace ce
{

// Constructor
Gtk::Widget* TextSpecifierPanel::getWidget()
{
	show_all();
	return this;
}

// Set the displayed value
void TextSpecifierPanel::setValue(const std::string& value)
{
    set_text(value);
}

// Get the edited value
std::string TextSpecifierPanel::getValue() const
{
    return get_text();
}

} // namespace ce

} // namespace objectives
