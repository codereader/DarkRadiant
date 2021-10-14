#pragma once

#include "i18n.h"
#include "ui/imainframe.h"
#include "wxutil/dialog/DialogBase.h"
#include "wxutil/XmlResourceBasedWidget.h"
#include <wx/textctrl.h>
#include "../CommitMetadata.h"

namespace vcs
{

namespace ui
{

class CommitDialog final :
    public wxutil::DialogBase,
    private wxutil::XmlResourceBasedWidget
{
public:
    CommitDialog(wxWindow* parent) :
        DialogBase(_("Create Commit"), parent)
    {
        loadNamedPanel(this, "VcsCommitPanel");
        
        InvalidateBestSize();
        Fit();
        CenterOnParent();
    }

    static git::CommitMetadata RunDialog(const git::CommitMetadata& defaultMetadata)
    {
        auto* dialog = new CommitDialog(GlobalMainFrame().getWxTopLevelWindow());
        
        auto result = defaultMetadata;

        dialog->findNamedObject<wxTextCtrl>(dialog, "CommitNameEntry")->SetValue(defaultMetadata.name);
        dialog->findNamedObject<wxTextCtrl>(dialog, "CommitEmailEntry")->SetValue(defaultMetadata.email);
        dialog->findNamedObject<wxTextCtrl>(dialog, "CommitMessage")->SetValue(defaultMetadata.message);

        if (dialog->ShowModal() != wxID_OK)
        {
            return git::CommitMetadata();
        }

        result.name = dialog->findNamedObject<wxTextCtrl>(dialog, "CommitNameEntry")->GetValue().ToStdString();
        result.email = dialog->findNamedObject<wxTextCtrl>(dialog, "CommitEmailEntry")->GetValue().ToStdString();
        result.message = dialog->findNamedObject<wxTextCtrl>(dialog, "CommitMessage")->GetValue().ToStdString();

        return result;
    }
};

}

}