#pragma once

#include "Binding.h"
#include <wx/spinctrl.h>

namespace ui
{

template<typename SpinCtrlType>
class SpinCtrlMaterialBinding :
    public TwoWayMaterialBinding<decltype(std::declval<SpinCtrlType>().GetValue())>
{
private:
    SpinCtrlType* _spinCtrl;
    using ValueType = decltype(std::declval<SpinCtrlType>().GetValue());

public:
    SpinCtrlMaterialBinding(SpinCtrlType* spinCtrl,
        const std::function<ValueType(const MaterialPtr&)>& loadFunc,
        const std::function<void(const MaterialPtr&, ValueType)>& saveFunc,
        const std::function<void()>& postChangeNotify = std::function<void()>()) :
        TwoWayMaterialBinding<ValueType>(loadFunc, saveFunc, postChangeNotify),
        _spinCtrl(spinCtrl)
    {
        if (saveFunc)
        {
            if (std::is_integral<ValueType>::value)
            {
                _spinCtrl->Bind(wxEVT_SPINCTRL, &SpinCtrlMaterialBinding::onValueChanged, this);
            }
            else
            {
                _spinCtrl->Bind(wxEVT_SPINCTRLDOUBLE, &SpinCtrlMaterialBinding::onValueChanged, this);
            }
        }
    }

protected:
    void setValueOnControl(const ValueType& value) override
    {
        _spinCtrl->SetValue(value);
    }

    void onValueChanged(wxCommandEvent& ev)
    {
        TwoWayMaterialBinding<ValueType>::updateValueOnTarget(_spinCtrl->GetValue());
    }
};

template<typename SpinCtrlType>
class SpinCtrlStageBinding :
    public TwoWayStageBinding<decltype(std::declval<SpinCtrlType>().GetValue())>
{
private:
    SpinCtrlType* _spinCtrl;
    using ValueType = decltype(std::declval<SpinCtrlType>().GetValue());

public:
    SpinCtrlStageBinding(SpinCtrlType* spinCtrl,
        const std::function<ValueType(const IShaderLayer::Ptr&)>& loadFunc,
        const std::function<IEditableShaderLayer::Ptr()>& acquireSaveTarget,
        const std::function<void(const IEditableShaderLayer::Ptr&, ValueType)>& saveFunc,
        const std::function<void()>& postChangeNotify = std::function<void()>()) :
        TwoWayStageBinding<ValueType>(loadFunc, acquireSaveTarget, saveFunc, postChangeNotify),
        _spinCtrl(spinCtrl)
    {
        if (saveFunc)
        {
            if (std::is_integral<ValueType>::value)
            {
                _spinCtrl->Bind(wxEVT_SPINCTRL, &SpinCtrlStageBinding::onValueChanged, this);
            }
            else
            {
                _spinCtrl->Bind(wxEVT_SPINCTRLDOUBLE, &SpinCtrlStageBinding::onValueChanged, this);
            }
        }
    }

protected:
    void setValueOnControl(const ValueType& value) override
    {
        _spinCtrl->SetValue(value);
    }

    void onValueChanged(wxCommandEvent& ev)
    {
        TwoWayStageBinding<ValueType>::updateValueOnTarget(_spinCtrl->GetValue());
    }
};

}
