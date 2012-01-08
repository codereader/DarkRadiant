#pragma once

#include "irender.h"

#include <boost/foreach.hpp>

namespace render
{
namespace lib
{

/**
 * Implementation of the LightList interface using a std::vector of pointers.
 */
class VectorLightList
: public LightList
{
    // Vector of lights
    typedef std::vector<const RendererLight*> Lights;
    Lights _lights;

public:

    void addLight(const RendererLight& light) {
        _lights.push_back(&light);
    }

    void clear() {
        _lights.clear();
    }

    void calculateIntersectingLights() const { }

    void setDirty() { }

    void forEachLight(const RendererLightCallback& callback) const
    {
        BOOST_FOREACH(const RendererLight* light, _lights)
        {
            callback(*light);
        }
    }
};

}
}
