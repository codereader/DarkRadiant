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
    using Ptr = std::shared_ptr<Binding>;

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

    virtual void updateFromSource() = 0;

protected:
    virtual void onSourceChanged()
    {}
};

// Target selection, usually the Source type is identical to the target type
template<typename Source>
struct TargetSelector
{
    using TargetType = Source;
};

// Specialisation for shader layer editing, where the target type is an IEditableShaderLayer
template<>
struct TargetSelector<IShaderLayer::Ptr>
{
    using TargetType = IEditableShaderLayer::Ptr;
};

template<typename Source, typename ValueType>
class TwoWayBinding :
    public Binding<Source>
{
public:
    // Select target: will map Material => Material, but IShaderLayer => IEditableShaderLayer
    using Target = typename TargetSelector<Source>::TargetType;

    using LoadFunc = std::function<ValueType(const Source&)>;
    using UpdateFunc = std::function<void(const Target&, ValueType)>;
    using AcquireTargetFunc = std::function<Target()>;
    using PostUpdateFunc = std::function<void()>;

    Target UseSourceAsTarget()
    {
        return Binding<Source>::getSource();
    }

protected:
    LoadFunc _loadValue;
    AcquireTargetFunc _acquireTarget;
    UpdateFunc _updateValue;
    PostUpdateFunc _postUpdate;

protected:
    bool _blockUpdates;

protected:
    TwoWayBinding(const LoadFunc& loadValue,
                  const UpdateFunc& updateValue,
                  const PostUpdateFunc& postChangeNotify = PostUpdateFunc(),
                  const AcquireTargetFunc& acquireTarget = std::bind(&TwoWayBinding::UseSourceAsTarget, this)) :
        _loadValue(loadValue),
        _acquireTarget(acquireTarget),
        _updateValue(updateValue),
        _blockUpdates(false)
    {}

    Target getTarget()
    {
        if (_blockUpdates)
        {
            return Target();
        }

        return _acquireTarget();
    }

protected:
    // Just load the given value into the control
    virtual void setValueOnControl(const ValueType& value) = 0;

    virtual void updateFromSource() override
    {
        util::ScopedBoolLock lock(_blockUpdates);

        if (!Binding<Source>::getSource())
        {
            setValueOnControl(ValueType());
            return;
        }

        setValueOnControl(_loadValue(Binding<Source>::getSource()));
    }

    virtual void updateValueOnTarget(const ValueType& newValue)
    {
        auto target = getTarget();

        if (target)
        {
            _updateValue(target, newValue);

            if (_postUpdate)
            {
                _postUpdate();
            }
        }
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
    std::function<void()> _postUpdateFunc;

public:
    CheckBoxBinding(wxCheckBox* checkbox, 
        const std::function<bool(const Source&)>& loadFunc) :
        CheckBoxBinding(checkbox, loadFunc, std::function<void(const Source&, bool)>(), std::function<void()>())
    {}

    CheckBoxBinding(wxCheckBox* checkbox,
        const std::function<bool(const Source&)>& loadFunc,
        const std::function<void(const Source&, bool)>& saveFunc,
        const std::function<void()>& postUpdateFunc) :
        _checkbox(checkbox),
        _loadFunc(loadFunc),
        _saveFunc(saveFunc),
        _postUpdateFunc(postUpdateFunc)
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

    virtual void updateFromSource() override
    {
        if (!Binding<Source>::getSource())
        {
            _checkbox->SetValue(false);
            return;
        }

        _checkbox->SetValue(_loadFunc(Binding<Source>::getSource()));
    }

private:
    void onCheckedChanged(wxCommandEvent& ev)
    {
        _saveFunc(Binding<Source>::getSource(), _checkbox->IsChecked());

        if (_postUpdateFunc)
        {
            _postUpdateFunc();
        }
    }
};

}
