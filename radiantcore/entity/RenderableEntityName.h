#pragma once

#include <sigc++/trackable.h>
#include <sigc++/functors/mem_fun.h>
#include "math/Matrix4.h"
#include "render/RenderableText.h"
#include "NameKey.h"

namespace entity
{

class RenderableEntityName :
    public render::RenderableText,
    public sigc::trackable
{
    NameKey& _nameKey;

    // The origin in world coordinates
    const Vector3& _entityOrigin;

    bool _needsUpdate;

public:
    RenderableEntityName(NameKey& nameKey, const Vector3& entityOrigin) :
        _nameKey(nameKey),
        _entityOrigin(entityOrigin),
        _needsUpdate(true)
    {
        // Queue an update once the name changes
        _nameKey.signal_nameChanged().connect(
            sigc::mem_fun(*this, &RenderableEntityName::queueUpdate)
        );
    }

    void queueUpdate()
    {
        _needsUpdate = true;
    }

protected:
    void onUpdate() override
    {
        if (!_needsUpdate) return;

        _needsUpdate = false;

        setText(_nameKey.getName());
        setWorldPosition(_entityOrigin);
    }
};

}
