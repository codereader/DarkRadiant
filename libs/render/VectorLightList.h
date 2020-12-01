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
 * This is used by the CamRenderer to build a list of lights submitted with
 * addLight().
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
