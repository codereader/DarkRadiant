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

using MaterialBinding = Binding<MaterialPtr>;

class CheckBoxBinding :
    public MaterialBinding
{
private:
    wxCheckBox* _checkbox;
    std::function<bool(const MaterialPtr&)> _loadFunc;

public:
    CheckBoxBinding(wxCheckBox* checkbox, const std::function<bool(const MaterialPtr&)> loadFunc) :
        _checkbox(checkbox),
        _loadFunc(loadFunc)
    {}

    virtual void updateFromSource(const MaterialPtr& material) override
    {
        if (!getSource())
        {
            _checkbox->SetValue(false);
            return;
        }

        _checkbox->SetValue(_loadFunc(getSource()));
    }
};

}
