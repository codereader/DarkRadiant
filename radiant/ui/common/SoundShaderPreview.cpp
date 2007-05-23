#include "SoundShaderPreview.h"

#include <gtk/gtk.h>

namespace ui {

SoundShaderPreview::SoundShaderPreview() {
	_preview = gtk_hbox_new(FALSE, 6);
}

SoundShaderPreview::operator GtkWidget*() {
	return _preview;
}
	
} // namespace ui
