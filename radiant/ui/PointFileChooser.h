#pragma once

#include <wx/dialog.h>

namespace ui
{

/// Selector dialog for pointfiles associated with the current map
class PointFileChooser: public wxDialog
{
    // Dialog constructor
    PointFileChooser(const wxArrayString& files);

public:

    /**
     * \brief Toggle pointfile visibility, showing the chooser if necessary.
     *
     * If there is only a single pointfile available, this method acts as a
     * simple visibility toggle. If there is more than one pointfile, the
     * chooser dialog will be shown before toggling visibility on. If there are
     * no pointfiles available, an error dialog will be shown.
     */
    static void chooseAndToggle();
};

}