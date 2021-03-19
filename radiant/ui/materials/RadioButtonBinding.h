#pragma once

#include "Binding.h"
#include "ishaderlayer.h"
#include <wx/radiobut.h>

namespace ui
{

class RadioButtonBinding :
    public TwoWayBinding<IShaderLayer::Ptr, IEditableShaderLayer::Ptr>
{
private:
    wxRadioButton* _radioButton;
    std::function<bool(const IShaderLayer::Ptr&)> _getValue;
    std::function<void(const IEditableShaderLayer::Ptr&, bool)> _updateValue;
    std::function<void()> _postChangeNotify;

public:
    RadioButtonBinding(wxRadioButton* radioButton,
        const std::function<bool(const IShaderLayer::Ptr&)>& loadFunc,
        const std::function<IEditableShaderLayer::Ptr()>& acquireSaveTarget,
        const std::function<void(const IEditableShaderLayer::Ptr&, bool)>& saveFunc,
        const std::function<void()>& postChangeNotify = std::function<void()>()) :
        TwoWayBinding(acquireSaveTarget),
        _radioButton(radioButton),
        _getValue(loadFunc),
        _updateValue(saveFunc),
        _postChangeNotify(postChangeNotify)
    {
        if (_updateValue)
        {
            _radioButton->Bind(wxEVT_RADIOBUTTON, &RadioButtonBinding::onValueChanged, this);
        }
    }

    virtual void updateFromSource() override
    {
        util::ScopedBoolLock lock(_blockUpdates);

        if (!getSource())
        {
            _radioButton->SetValue(false);
            return;
        }

        auto value = _getValue(getSource());
        _radioButton->SetValue(value);
    }

private:
    void onValueChanged(wxCommandEvent& ev)
    {
        auto target = getTarget();

        if (target)
        {
            _updateValue(target, _radioButton->GetValue());
        }

        if (_postChangeNotify)
        {
            _postChangeNotify();
        }
    }
};

}
