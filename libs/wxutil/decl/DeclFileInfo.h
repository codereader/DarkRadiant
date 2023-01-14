#pragma once

#include "i18n.h"
#include "idecltypes.h"
#include "ideclmanager.h"

#include <wx/panel.h>
#include <wx/stattext.h>
#include <wx/sizer.h>

namespace wxutil
{

// Composite label, displaying the file a certain declaration has been defined in
class DeclFileInfo :
    public wxPanel
{
private:
    wxStaticText* _nameLabel;
    wxStaticText* _fileLabel;
    wxStaticText* _definedInLabel;
    decl::Type _declType;

public:
    DeclFileInfo(wxWindow* parent, decl::Type declType) :
        wxPanel(parent, wxID_ANY),
        _declType(declType)
    {
        _fileLabel = new wxStaticText(this, wxID_ANY, "");
        _fileLabel->SetFont(_fileLabel->GetFont().Bold());

        _nameLabel = new wxStaticText(this, wxID_ANY, "");
        _nameLabel->SetFont(_nameLabel->GetFont().Bold());

        auto sizer = new wxBoxSizer(wxHORIZONTAL);

        sizer->Add(new wxStaticText(this, wxID_ANY, decl::getTypeName(_declType) + " "), 0, wxALIGN_CENTER_VERTICAL, 0);
        sizer->Add(_nameLabel, 0, wxALIGN_CENTER_VERTICAL, 0);
        _definedInLabel = new wxStaticText(this, wxID_ANY, _(" defined in "));
        sizer->Add(_definedInLabel, 0, wxALIGN_CENTER_VERTICAL, 0);
        sizer->Add(_fileLabel, 0, wxALIGN_CENTER_VERTICAL, 0);

        SetSizer(sizer);

        setPath({});
    }

    void SetDeclarationName(const std::string& declName)
    {
        auto decl = GlobalDeclarationManager().findDeclaration(_declType, declName);

        setName(!declName.empty() && decl ? decl->getDeclName() : std::string());
        setPath(!declName.empty() && decl ? decl->getDeclFilePath() : std::string());
    }

    void Clear()
    {
        setName({});
        setPath({});
    }

    void setName(const std::string& name)
    {
        _nameLabel->SetLabel(!name.empty() ? name : "-");
        GetSizer()->Layout();
    }

    void setPath(const std::string& path)
    {
        _fileLabel->SetLabel(path);
        _fileLabel->Show(!path.empty());
        _definedInLabel->Show(!path.empty());
        GetSizer()->Layout();
    }
};

}
