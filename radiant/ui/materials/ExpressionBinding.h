#pragma once

#include "Binding.h"
#include "ishaderexpression.h"
#include "util/ScopedBoolLock.h"

namespace ui
{

class ExpressionBinding :
    public TwoWayBinding<IShaderLayer::Ptr, IEditableShaderLayer::Ptr>
{
private:
    wxTextCtrl* _textCtrl;
    std::function<shaders::IShaderExpression::Ptr(const IShaderLayer::Ptr&)> _getExpression;
    std::function<void(const IEditableShaderLayer::Ptr&, const std::string&)> _updateExpression;
    std::function<void()> _postChangeNotify;

public:
    ExpressionBinding(wxTextCtrl* textCtrl,
                      const std::function<shaders::IShaderExpression::Ptr(const IShaderLayer::Ptr&)>& loadFunc,
                      const std::function<IEditableShaderLayer::Ptr()>& acquireSaveTarget,
                      const std::function<void(const IEditableShaderLayer::Ptr&, const std::string&)>& saveFunc,
                      const std::function<void()>& postChangeNotify = std::function<void()>()) :
        TwoWayBinding(acquireSaveTarget),
        _textCtrl(textCtrl),
        _getExpression(loadFunc),
        _updateExpression(saveFunc),
        _postChangeNotify(postChangeNotify)
    {
        if (_updateExpression)
        {
            _textCtrl->Bind(wxEVT_TEXT, &ExpressionBinding::onTextChanged, this);
        }
    }

    virtual void updateFromSource() override
    {
        util::ScopedBoolLock lock(_blockUpdates);

        if (!ExpressionBinding::getSource())
        {
            _textCtrl->SetValue("");
            return;
        }

        auto expression = _getExpression(ExpressionBinding::getSource());
        _textCtrl->SetValue(expression ? expression->getExpressionString() : "");
    }

private:
    void onTextChanged(wxCommandEvent& ev)
    {
        auto target = ExpressionBinding::getTarget();

        if (target)
        {
            _updateExpression(target, _textCtrl->GetValue().ToStdString());
        }

        if (_postChangeNotify)
        {
            _postChangeNotify();
        }
    }
};

}
