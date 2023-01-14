#pragma once

#include <wx/dataview.h>
#include <wx/dc.h>
#include <wx/renderer.h>
#include <wx/textctrl.h>

#include "ui/materials/MaterialChooser.h"
#include "wxutil/Bitmap.h"

namespace ui
{

class MaterialSelectorRenderer :
    public wxDataViewCustomRenderer
{
private:
    wxString _materialName;

    constexpr static auto EDITOR_TEXT_CONTROL_NAME = "TextEntry";

    sigc::signal<void(const std::string&)> _sigMaterialSelected;

public:
    MaterialSelectorRenderer() :
        wxDataViewCustomRenderer("string", wxDATAVIEW_CELL_EDITABLE)
    {}

    sigc::signal<void(const std::string&)>& signal_onMaterialSelected()
    {
        return _sigMaterialSelected;
    }

    bool Render(wxRect cell, wxDC* dc, int state) override
    {
        RenderText(_materialName, 0, cell, dc, state);
        return true;
    }

    wxSize GetSize() const override
    {
        auto size = GetTextExtent(_materialName);
        size.IncTo(wxSize(wxDVC_DEFAULT_RENDERER_SIZE, -1));

        return size;
    }

    bool GetValue(wxVariant& value) const override
    {
        value = _materialName;
        return true;
    }

    bool SetValue(const wxVariant& value) override
    {
        _materialName = value.GetString();
        return true;
    }

    bool HasEditorCtrl() const override
    {
        return true;
    }

    wxWindow* CreateEditorCtrl(wxWindow* parent, wxRect rect, const wxVariant& value) override
    {
        auto panel = new wxPanel(parent, wxID_ANY, rect.GetPosition(), rect.GetSize());
        panel->SetSizer(new wxBoxSizer(wxHORIZONTAL));

        auto ctrl = new wxTextCtrl(panel, wxID_ANY, value, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
        ctrl->SetName(EDITOR_TEXT_CONTROL_NAME);
        ctrl->SetSize(wxSize(rect.GetWidth() - 32, -1));

        // Dispatch the signal when the control is receiving a value
        ctrl->Bind(wxEVT_TEXT, [ctrl, this](auto& ev) { signal_onMaterialSelected().emit(ctrl->GetValue().ToStdString()); });

        // select the text in the control an place the cursor at the end
        ctrl->SetInsertionPointEnd();
        ctrl->SelectAll();

        auto button = new wxBitmapButton(panel, wxID_ANY, wxutil::GetLocalBitmap("treeView16.png"));
        button->Bind(wxEVT_BUTTON, [=](auto& ev)
        {
            auto selector = new MaterialChooser(panel, MaterialSelector::TextureFilter::All, ctrl);
            selector->ShowModal();
        });

        panel->GetSizer()->Add(ctrl, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
        panel->GetSizer()->Add(button, 0, wxALIGN_CENTER_VERTICAL);

        panel->Layout();
        panel->Fit();

        return panel;
    }

    bool GetValueFromEditorCtrl(wxWindow* editPanel, wxVariant& value) override
    {
        auto textCtrl = static_cast<wxTextCtrl*>(editPanel->FindWindow(EDITOR_TEXT_CONTROL_NAME));
        value = textCtrl->GetValue();
        return true;
    }
};

/**
 * Custom wxDataViewColumn showing a material name and selector button
 */
class MaterialSelectorColumn :
    public wxDataViewColumn
{
private:
    sigc::signal<void(const std::string&)> _sigMaterialSelected;
    sigc::connection _rendererSelectionConn;

public:
    MaterialSelectorColumn(const std::string& title, int modelColumn, int width = wxCOL_WIDTH_AUTOSIZE,
                   wxAlignment align = wxALIGN_LEFT, int flags = wxDATAVIEW_COL_RESIZABLE) :
        wxDataViewColumn(title, new MaterialSelectorRenderer(), modelColumn, width, align, flags)
    {
        // Dispatch the browser selection signal
        auto renderer = static_cast<MaterialSelectorRenderer*>(GetRenderer());
        _rendererSelectionConn = renderer->signal_onMaterialSelected().connect(
            [this](const std::string& value) { signal_onMaterialSelected().emit(value); });
    }

    ~MaterialSelectorColumn() override
    {
        _rendererSelectionConn.disconnect();
    }

    // A signal that is sent out as soon as a material is selected in the browse dialog
    // This doesn't mean that the selected material is going to be accepted, but it can be
    // used to live-update other preview widgets
    sigc::signal<void(const std::string&)>& signal_onMaterialSelected()
    {
        return _sigMaterialSelected;
    }
};


}
