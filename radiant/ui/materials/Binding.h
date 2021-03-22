#pragma once

#include "ishaders.h"
#include "util/ScopedBoolLock.h"
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

    static TargetType CastToTarget(const Source& source)
    {
        return source;
    }
};

// Specialisation for shader layer editing, where the target type is an IEditableShaderLayer
template<>
struct TargetSelector<IShaderLayer::Ptr>
{
    using TargetType = IEditableShaderLayer::Ptr;

    static TargetType CastToTarget(const IShaderLayer::Ptr& source)
    {
        return std::static_pointer_cast<IEditableShaderLayer>(source);
    } 
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
                  const AcquireTargetFunc& acquireTarget = AcquireTargetFunc()) :
        _loadValue(loadValue),
        _acquireTarget(acquireTarget),
        _updateValue(updateValue),
        _postUpdate(postChangeNotify),
        _blockUpdates(false)
    {}

    Target getTarget()
    {
        if (_blockUpdates)
        {
            return Target();
        }

        if (std::is_same<Source, Target>::value)
        {
            return TargetSelector<Source>::CastToTarget(Binding<Source>::getSource());
        }

        return _acquireTarget ? _acquireTarget() : Target();
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

}
