#pragma once

#include "irender.h"
#include <list>
#include <boost/function/function_fwd.hpp>

namespace render
{

typedef std::set<RendererLight*> RendererLights;

/**
 * \brief
 * Main renderer implementation of LightList interface.
 *
 * The LinearLightList is reponsible for associating a single lit object with
 * all of the lights which currently light it.
 */
class LinearLightList :
	public LightList
{
public:
	typedef boost::function<void()> VoidCallback;

private:

    // Target object
	LitObject& _litObject;

    // Set of all available lights
	RendererLights& _allLights;

    // Update callback
	VoidCallback _testDirtyFunc;

    // List of lights which are intersecting our lit object
	typedef std::list<RendererLight*> Lights;
	mutable Lights _activeLights;

    // Dirty flag indicating recalculation needed
	mutable bool m_dirty;

public:

    /**
     * \brief
     * Construct and initialise with values.
     *
     * \param object
     * The illuminatable object whose lit status we are tracking.
     *
     * \param lights
     * Entire set of available light sources provided by the renderer.
     *
     * \param testFunc
     * A callback function to request the renderer check if the light list
     * needs to recalculate its intersections, and call setDirty() if necessary.
     */
    LinearLightList(LitObject& object,
                    RendererLights& lights,
                    VoidCallback testFunc)
    : _litObject(object), _allLights(lights), _testDirtyFunc(testFunc)
	{
		m_dirty = true;
	}

    // LightList implementation
	void calculateIntersectingLights() const;
	void forEachLight(const RendererLightCallback& callback) const;
	void setDirty();
};

} // namespace render
