#pragma once

#include "wxutil/window/TransientWindow.h"
#include "wxutil/XmlResourceBasedWidget.h"

namespace ui
{


class MergeControlDialog :
    public wxutil::TransientWindow,
    private wxutil::XmlResourceBasedWidget,
    public sigc::trackable
{
public:
    MergeControlDialog();

    static MergeControlDialog& Instance();

    // The command target
    static void Toggle(const cmd::ArgumentList& args);

private:
    void onMainFrameShuttingDown();
    static std::shared_ptr<MergeControlDialog>& InstancePtr();

    void convertTextCtrlToPathEntry(const std::string& ctrlName);
    void onMergeSourceChanged(wxCommandEvent& ev);
    void onLoadAndCompare(wxCommandEvent& ev);
    void onAbortMerge(wxCommandEvent& ev);
    void updateControlSensitivity();
};

}
