#pragma once

#include "irender.h"

namespace render
{

namespace lib
{

/**
 * \brief
 * A simple container of lights
 *
 * This is used by particular object types to store the list of intersecting
 * lights passed into LitObject::insertLight() as a result of an earlier call
 * to LightList::calculateIntersectingLights().
 *
 * Objects may use their insertLight() method to perform additional
 * optimisations to exclude lights for object-specific reasons, then store the
 * minimised light list in a VectorLightList for subsequent rendering.
 */
class VectorLightList: public LightSources
{
    // Vector of lights
    typedef std::vector<const RendererLight*> Lights;
    Lights _lights;

    friend std::ostream& operator<< (std::ostream&, const VectorLightList&);

public:

    void addLight(const RendererLight& light)
	{
        _lights.push_back(&light);
    }

    void clear()
	{
        _lights.clear();
    }

    // LightSources implementation
    void forEachLight(const RendererLightCallback& callback) const override
    {
        for (const RendererLight* light : _lights)
        {
            callback(*light);
        }
    }
};

inline std::ostream& operator<< (std::ostream& os, const VectorLightList& ll)
{
    return os << "VectorLightList { size = " << ll._lights.size() << " }";
}

}

}
