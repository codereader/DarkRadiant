#pragma once

#include "ishaders.h"
#include <wx/checkbox.h>

namespace ui
{

template<typename Source>
class Binding
{
private:
    Source _source;

public:
    virtual ~Binding()
    {}

    const Source& getSource() const
    {
        return _source;
    }

    void setSource(const Source& source)
    {
        _source = source;
        onSourceChanged();
    }

    virtual void updateFromSource(const Source& source) = 0;

protected:
    virtual void onSourceChanged()
    {
        updateFromSource(getSource());
    }
};

template<typename Source>
class CheckBoxBinding :
    public Binding<Source>
{
private:
    wxCheckBox* _checkbox;
    std::function<bool(const Source&)> _loadFunc;
    std::function<void(const Source&, bool)> _saveFunc;

public:
    CheckBoxBinding(wxCheckBox* checkbox, 
        const std::function<bool(const Source&)> loadFunc) :
        CheckBoxBinding(checkbox, loadFunc, std::function<void(const Source&, bool)>())
    {}

    CheckBoxBinding(wxCheckBox* checkbox,
        const std::function<bool(const Source&)> loadFunc,
        const std::function<void(const Source&, bool)> saveFunc) :
        _checkbox(checkbox),
        _loadFunc(loadFunc),
        _saveFunc(saveFunc)
    {
        if (_saveFunc)
        {
            _checkbox->Bind(wxEVT_CHECKBOX, &CheckBoxBinding::onCheckedChanged, this);
        }
    }

    virtual ~CheckBoxBinding()
    {
        if (_saveFunc)
        {
            _checkbox->Unbind(wxEVT_CHECKBOX, &CheckBoxBinding::onCheckedChanged, this);
        }
    }

    virtual void updateFromSource(const Source& source) override
    {
        if (!source)
        {
            _checkbox->SetValue(false);
            return;
        }

        _checkbox->SetValue(_loadFunc(source));
    }

private:
    void onCheckedChanged(wxCommandEvent& ev)
    {
        _saveFunc(Binding<Source>::getSource(), _checkbox->IsChecked());
    }
};

}
