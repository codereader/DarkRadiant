#include "EntityNameSpecifierPanel.h"

#include <wx/sizer.h>
#include "inode.h"
#include "imap.h"
#include "ientity.h"

namespace objectives
{

namespace ce
{

EntityNameSpecifierPanel::EntityNameSpecifierPanel() :
    _editCombo(nullptr)
{}

EntityNameSpecifierPanel::EntityNameSpecifierPanel(wxWindow* parent) :
    _editCombo(nullptr)
{
    _editCombo = new wxComboBox(parent, wxID_ANY);

    // Bind both events to the same callback
    _editCombo->Bind(wxEVT_TEXT, &EntityNameSpecifierPanel::onComboBoxChanged, this);
    _editCombo->Bind(wxEVT_COMBOBOX, &EntityNameSpecifierPanel::onComboBoxChanged, this);

    // Collect all entity names in the scene
    wxArrayString names;
    GlobalMapModule().getRoot()->foreachNode([&](const scene::INodePtr& node)
    {
        if (Node_isEntity(node))
        {
            names.push_back(Node_getEntity(node)->getKeyValue("name"));
        }

        return true;
    });

    names.Sort();
    _editCombo->Append(names);
}

EntityNameSpecifierPanel::~EntityNameSpecifierPanel()
{
    delete _editCombo;
    _editCombo = nullptr;
}

wxWindow* EntityNameSpecifierPanel::getWidget()
{
    return _editCombo;
}

void EntityNameSpecifierPanel::setValue(const std::string& value)
{
    _editCombo->SetValue(value);
}

std::string EntityNameSpecifierPanel::getValue() const
{
    return _editCombo->GetValue().ToStdString();
}

void EntityNameSpecifierPanel::onComboBoxChanged(wxCommandEvent& ev)
{
    if (_valueChanged)
    {
        _valueChanged(); // usually points to the abstract ComponentEditor::writeToComponent
    }
}

// Reg helper
EntityNameSpecifierPanel::RegHelper EntityNameSpecifierPanel::_regHelper;

}

}

