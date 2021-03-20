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

template<typename Source, typename Target>
class TwoWayBinding :
    public Binding<Source>
{
private:
    std::function<Target()> _acquireTarget;

protected:
    bool _blockUpdates;

protected:
    TwoWayBinding(const std::function<Target()>& acquireTarget) :
        _acquireTarget(acquireTarget),
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
};

template<typename ValueType>
class TwoWayMaterialBinding :
    public TwoWayBinding<MaterialPtr, MaterialPtr>
{
protected:
    std::function<ValueType(const MaterialPtr&)> _getValue;
    std::function<void(const MaterialPtr&, ValueType)> _updateValue;
    std::function<void()> _postChangeNotify;

public:
    TwoWayMaterialBinding(const std::function<ValueType(const MaterialPtr&)>& loadFunc,
        const std::function<void(const MaterialPtr&, ValueType)>& saveFunc,
        const std::function<void()>& postChangeNotify = std::function<void()>()) :
        TwoWayBinding([this] { return getSource(); }),
        _getValue(loadFunc),
        _updateValue(saveFunc),
        _postChangeNotify(postChangeNotify)
    {}

protected:
    // Just load the given value into the control
    virtual void setValueOnControl(const ValueType& value) = 0;

    virtual void updateFromSource() override
    {
        util::ScopedBoolLock lock(_blockUpdates);

        if (!getSource())
        {
            setValueOnControl(ValueType());
            return;
        }

        setValueOnControl(_getValue(getSource()));
    }

    virtual void updateValueOnTarget(const ValueType& newValue)
    {
        auto target = getTarget();

        if (target)
        {
            _updateValue(target, newValue);
        }

        if (_postChangeNotify)
        {
            _postChangeNotify();
        }
    }
};

template<typename ValueType>
class TwoWayStageBinding :
    public TwoWayBinding<IShaderLayer::Ptr, IEditableShaderLayer::Ptr>
{
protected:
    std::function<ValueType(const IShaderLayer::Ptr&)> _getValue;
    std::function<void(const IEditableShaderLayer::Ptr&, ValueType)> _updateValue;
    std::function<void()> _postChangeNotify;

public:
    TwoWayStageBinding(const std::function<ValueType(const IShaderLayer::Ptr&)>& loadFunc,
        const std::function<IEditableShaderLayer::Ptr()>& acquireSaveTarget,
        const std::function<void(const IEditableShaderLayer::Ptr&, ValueType)>& saveFunc,
        const std::function<void()>& postChangeNotify = std::function<void()>()) :
        TwoWayBinding(acquireSaveTarget),
        _getValue(loadFunc),
        _updateValue(saveFunc),
        _postChangeNotify(postChangeNotify)
    {}

protected:
    // Just load the given value into the control
    virtual void setValueOnControl(const ValueType& value) = 0;

    virtual void updateFromSource() override
    {
        util::ScopedBoolLock lock(_blockUpdates);

        if (!getSource())
        {
            setValueOnControl(ValueType());
            return;
        }

        setValueOnControl(_getValue(getSource()));
    }

    virtual void updateValueOnTarget(const ValueType& newValue)
    {
        auto target = getTarget();

        if (target)
        {
            _updateValue(target, newValue);
        }

        if (_postChangeNotify)
        {
            _postChangeNotify();
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
