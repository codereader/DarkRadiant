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

public:
    CheckBoxBinding(wxCheckBox* checkbox, const std::function<bool(const Source&)> loadFunc) :
        _checkbox(checkbox),
        _loadFunc(loadFunc)
    {}

    virtual void updateFromSource(const Source& source) override
    {
        if (!source)
        {
            _checkbox->SetValue(false);
            return;
        }

        _checkbox->SetValue(_loadFunc(source));
    }
};

}
