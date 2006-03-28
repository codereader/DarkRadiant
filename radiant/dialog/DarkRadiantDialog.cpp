#include "DarkRadiantDialog.h"

namespace darkradiant
{

DarkRadiantDialog::DarkRadiantDialog():
theWindow(GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL)))
{
}

DarkRadiantDialog::~DarkRadiantDialog()
{
}

}
