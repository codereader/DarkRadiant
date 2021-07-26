#include "GameConnectionDialog.h"

#include "i18n.h"
#include "imainframe.h"


namespace
{
    const char* GameConnectionDialog_TITLE = N_("Game connection");
}


namespace ui
{

GameConnectionDialog& GameConnectionDialog::Instance() {
    static GameConnectionDialog _instance;
    return _instance;
}

void GameConnectionDialog::toggleDialog(const cmd::ArgumentList& args) {
    // Toggle the instance
    Instance().ToggleVisibility();
}

GameConnectionDialog::GameConnectionDialog() :
    wxutil::TransientWindow(_(GameConnectionDialog_TITLE), GlobalMainFrame().getWxTopLevelWindow(), true)
{

}


void GameConnectionDialog::_preShow() {
    TransientWindow::_preShow();
}

void GameConnectionDialog::_preHide() {
    TransientWindow::_preHide();
}

}
