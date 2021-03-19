#pragma once

#include "Binding.h"
#include "ishaderexpression.h"
#include "util/ScopedBoolLock.h"

namespace ui
{

class ExpressionBinding :
    public TwoWayStageBinding<std::string>
{
private:
    wxTextCtrl* _textCtrl;
    std::function<std::string(const IShaderLayer::Ptr&)> _getExpression;
    std::function<void(const IEditableShaderLayer::Ptr&, const std::string&)> _updateExpression;
    std::function<void()> _postChangeNotify;

public:
    ExpressionBinding(wxTextCtrl* textCtrl,
                      const std::function<std::string(const IShaderLayer::Ptr&)>& loadFunc,
                      const std::function<IEditableShaderLayer::Ptr()>& acquireSaveTarget,
                      const std::function<void(const IEditableShaderLayer::Ptr&, const std::string&)>& saveFunc,
                      const std::function<void()>& postChangeNotify = std::function<void()>()) :
        TwoWayStageBinding(loadFunc, acquireSaveTarget, saveFunc, postChangeNotify),
        _textCtrl(textCtrl)
    {
        if (saveFunc)
        {
            _textCtrl->Bind(wxEVT_TEXT, &ExpressionBinding::onTextChanged, this);
        }
    }

protected:
    void setValueOnControl(const std::string& value) override
    {
        _textCtrl->SetValue(value);
    }

    void onTextChanged(wxCommandEvent& ev)
    {
        updateValueOnTarget(_textCtrl->GetValue().ToStdString());
    }
};

}
