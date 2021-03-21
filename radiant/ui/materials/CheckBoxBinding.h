#pragma once

namespace ui
{

template<typename Source>
class CheckBoxBinding :
    public TwoWayBinding<Source, bool>
{
public:
    using BaseBinding = TwoWayBinding<Source, bool>;

private:
    wxCheckBox* _checkbox;

public:
    CheckBoxBinding(wxCheckBox* checkbox,
        const typename BaseBinding::LoadFunc& loadFunc,
        const typename BaseBinding::UpdateFunc& saveFunc) :
        CheckBoxBinding(checkbox, loadFunc, saveFunc, BaseBinding::PostUpdateFunc())
    {}

    CheckBoxBinding(wxCheckBox* checkbox,
        const typename BaseBinding::LoadFunc& loadFunc,
        const typename BaseBinding::UpdateFunc& saveFunc,
        const typename BaseBinding::PostUpdateFunc& postChangeNotify) :
        CheckBoxBinding(checkbox, loadFunc, saveFunc, postChangeNotify, std::bind(&BaseBinding::UseSourceAsTarget, this))
    {}

    CheckBoxBinding(wxCheckBox* checkbox,
        const typename BaseBinding::LoadFunc& loadFunc,
        const typename BaseBinding::UpdateFunc& saveFunc,
        const typename BaseBinding::PostUpdateFunc& postChangeNotify,
        const typename BaseBinding::AcquireTargetFunc& acquireSaveTarget) :
        BaseBinding(loadFunc, saveFunc, postChangeNotify, acquireSaveTarget),
        _checkbox(checkbox)
    {
        if (BaseBinding::_updateValue)
        {
            _checkbox->Bind(wxEVT_CHECKBOX, &CheckBoxBinding::onValueChanged, this);
        }
    }

    virtual ~CheckBoxBinding()
    {
        if (BaseBinding::_updateValue)
        {
            _checkbox->Unbind(wxEVT_CHECKBOX, &CheckBoxBinding::onValueChanged, this);
        }
    }

protected:
    void setValueOnControl(const bool& value) override
    {
        _checkbox->SetValue(value);
    }

    void onValueChanged(wxCommandEvent& ev)
    {
        BaseBinding::updateValueOnTarget(_checkbox->GetValue());
    }
};

}
