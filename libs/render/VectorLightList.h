#pragma once

#include "irender.h"

namespace render
{

namespace lib
{

/**
 * Implementation of the LightList interface using a std::vector of pointers.
 */
class VectorLightList : 
	public LightList
{
private:
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

    void calculateIntersectingLights() const { }

    void setDirty() { }

    void forEachLight(const RendererLightCallback& callback) const
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
