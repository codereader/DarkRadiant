#pragma once

#include "SpecifierPanel.h"
#include "SpecifierPanelFactory.h"
#include <wx/combobox.h>

namespace objectives
{

namespace ce
{

/**
 * SpecifierPanel subclass for the SPEC_NAME (name of single entity) specifier
 * type.
 */
class EntityNameSpecifierPanel : 
    public wxEvtHandler,
    public SpecifierPanel
{
protected:
    wxComboBox* _editCombo;

    // The change callback, invoked when the text changes
    std::function<void()> _valueChanged;

    EntityNameSpecifierPanel();

public:
    EntityNameSpecifierPanel(wxWindow* parent);

    virtual ~EntityNameSpecifierPanel();

    /* SpecifierPanel implementation */
    virtual SpecifierPanelPtr create(wxWindow* parent) const override
    {
        return std::make_shared<EntityNameSpecifierPanel>(parent);
    }

    void setChangedCallback(const std::function<void()>& callback) override
    {
        _valueChanged = callback;
    }

    virtual wxWindow* getWidget() override;
    void setValue(const std::string& value) override;
    std::string getValue() const override;

private:
    void onComboBoxChanged(wxCommandEvent& ev);

	// Map registration
	static struct RegHelper
    {
		RegHelper()
        {
			SpecifierPanelFactory::registerType(
				SpecifierType::SPEC_NAME().getName(),
				SpecifierPanelPtr(new EntityNameSpecifierPanel())
			);
		}
	} _regHelper;
};

}

}
