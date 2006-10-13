#include "MediaBrowser.h"

#include <gtk/gtkvbox.h>

namespace ui
{

// Constructor
MediaBrowser::MediaBrowser()
: _widget(gtk_vbox_new(FALSE, 0))
{
}

}
