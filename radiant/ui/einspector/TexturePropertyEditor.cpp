#include "TexturePropertyEditor.h"
#include "TextureChooser.h"

#include "ishaders.h"
#include "generic/callback.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

namespace ui
{

// Main constructor

TexturePropertyEditor::TexturePropertyEditor(Entity* entity, const std::string& name)
: PropertyEditor(entity, name, "texture"),
//  _prefixes("lights/,fogs/")
  _prefixes("lights/,fogs/")
{
	GtkWidget* outer = gtk_vbox_new(FALSE, 0);

	GtkWidget* editBox = gtk_hbox_new(FALSE, 3);
	gtk_container_set_border_width(GTK_CONTAINER(editBox), 3);
	_textEntry = gtk_entry_new();
	
	std::string caption = getKey();
	caption.append(": ");
	gtk_box_pack_start(GTK_BOX(editBox), gtk_label_new(caption.c_str()), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(editBox), _textEntry, TRUE, TRUE, 0);

	GtkWidget* browseButton = gtk_button_new_with_label("...");
	gtk_box_pack_start(GTK_BOX(editBox), browseButton, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(browseButton), "clicked", G_CALLBACK(callbackBrowse), this);
	
	gtk_box_pack_start(GTK_BOX(outer), editBox, TRUE, FALSE, 0);
	
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(getEditWindow()),
										  outer);
}

// Browse button callback, with local functor object

namespace {

	struct ShaderNameFunctor {

		typedef const char* first_argument_type;

		// Interesting texture prefixes
		std::vector<std::string> _prefixes;

		// Constructor
		ShaderNameFunctor(const std::string& pref) {
			boost::algorithm::split(_prefixes,
									pref,
									boost::algorithm::is_any_of(","));
		
		}
	
		// Functor operator
		void operator() (const char* shaderName) {
			std::string name(shaderName);
			for (std::vector<std::string>::iterator i = _prefixes.begin();
				 i != _prefixes.end();
				 i++)
			{
				if (boost::algorithm::istarts_with(name, *i)) {
					std::cout << name << std::endl;
				}
			}
		}
	};

}

void TexturePropertyEditor::callbackBrowse(GtkWidget* widget, TexturePropertyEditor* self) {
	new TextureChooser(self->_textEntry); // self-destructs on close
}

// Get and set functions

void TexturePropertyEditor::setValue(const std::string& val) {
	gtk_entry_set_text(GTK_ENTRY(_textEntry), val.c_str());
}

const std::string TexturePropertyEditor::getValue() {
	return std::string(gtk_entry_get_text(GTK_ENTRY(_textEntry)));
}

} // namespace ui
