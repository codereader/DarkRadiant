#include "MergeControlDialog.h"

#include "i18n.h"
#include "itextstream.h"
#include "imainframe.h"

namespace ui
{

namespace
{
    const char* const WINDOW_TITLE = N_("Merge Maps");
}

MergeControlDialog::MergeControlDialog() :
    TransientWindow(_(WINDOW_TITLE), GlobalMainFrame().getWxTopLevelWindow(), true)
{
    SetSizer(new wxBoxSizer(wxVERTICAL));
    GetSizer()->Add(loadNamedPanel(this, "MergeControlDialogMainPanel"), 1, wxEXPAND);

    SetMinSize(wxSize(300, 300));
}

std::shared_ptr<MergeControlDialog>& MergeControlDialog::InstancePtr()
{
    static std::shared_ptr<MergeControlDialog> _instancePtr;
    return _instancePtr;
}

MergeControlDialog& MergeControlDialog::Instance()
{
    auto& instancePtr = InstancePtr();

    if (!instancePtr)
    {
        // Not yet instantiated, do it now
        instancePtr.reset(new MergeControlDialog);

        // Pre-destruction cleanup
        GlobalMainFrame().signal_MainFrameShuttingDown().connect(
            sigc::mem_fun(*instancePtr, &MergeControlDialog::onMainFrameShuttingDown)
        );
    }

    return *instancePtr;
}

void MergeControlDialog::onMainFrameShuttingDown()
{
    rMessage() << "MergeControlDialog shutting down." << std::endl;

    if (IsShownOnScreen())
    {
        Hide();
    }

    // Destroy the window 
    SendDestroyEvent();
    InstancePtr().reset();
}

void MergeControlDialog::Toggle(const cmd::ArgumentList& args)
{
    Instance().ToggleVisibility();
}

}
