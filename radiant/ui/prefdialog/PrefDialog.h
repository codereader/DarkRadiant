#pragma once

#include <map>
#include <wx/bookctrl.h>
#include "iradiant.h"
#include "icommandsystem.h"

#include "PrefPage.h"
#include "wxutil/dialog/DialogBase.h"

class wxTreebook;

namespace ui
{

/// Dialog for editing user preferences
class PrefDialog: public wxutil::DialogBase
{
    wxBookCtrlBase* _notebook = nullptr;

    // Each notebook page is created and maintained by a PrefPage class
    // Map the page path to its widget
    typedef std::map<std::string, PrefPage*> PageMap;
    PageMap _pages;

    PrefDialog(wxWindow* parent);

public:
    /** greebo: Makes sure that the dialog is visible.
     *          (does nothing if the dialog is already on screen)
     */
    static void ShowDialog(const std::string& path = "");

    /** greebo: Displays the page with the specified path.
     *
     * @path: a string like "Settings/Patches"
     */
    void showPage(const std::string& path);

private:
    void showModal(const std::string& requestedPage);

    void createPages();
};

} // namespace ui
