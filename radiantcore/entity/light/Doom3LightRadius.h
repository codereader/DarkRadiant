#pragma once

#include "math/Vector3.h"
#include "registry/registry.h"
#include "iscenegraph.h"
#include <string>
#include "gamelib.h"
#include <functional>

/* greebo: The Doom3LightRadius class manages the light center and the light radius.
 * Some of the light callbacks are connected to member methods of this class
 * (see Light::setLightChangedCallback)
 */

class Doom3LightRadius: public sigc::trackable
{
public:
    Vector3 _defaultRadius;
    Vector3 m_radius;
    Vector3 m_radiusTransformed;

    Vector3 m_center;
    Vector3 m_centerTransformed;

    std::function<void()> m_changed;

    Doom3LightRadius()
    : _defaultRadius(game::current::getValue<Vector3>("/defaults/lightRadius")),
      m_radius(_defaultRadius),
      m_radiusTransformed(0, 0, 0),
      m_center(0, 0, 0),
      m_centerTransformed(0, 0, 0)
    {
    }

    void lightRadiusChanged(const std::string& value)
    {
        Vector3 candidate = string::convert<Vector3>(value);

        // The conversion returns (0,0,0), if failed, which is not a valid value for a light radius
        if (candidate != Vector3(0,0,0)) {
            m_radius = candidate;
        }
        else {
            // Take the default radius as value
            m_radius = _defaultRadius;
        }
        // Reset the transformed radius to the current radius
        m_radiusTransformed = m_radius;

        // Notify the callback function
        if (m_changed)
        {
            m_changed();
        }

        // Update the scene
        SceneChangeNotify();
    }

    /* greebo: This gets called by the keyObserver when the light center value is changed.
     * Note: this gets immediately after a new light is constructed as well.
     */
    void lightCenterChanged(const std::string& value)
    {
        Vector3 candidate = string::convert<Vector3>(value);

        // If the conversion failed, the candidate = (0,0,0), which is the same as no center value set at all
        if (candidate != Vector3(0,0,0)) {
            // Load the center key from the value
            m_center = candidate;
        }
        else {
            // Default the center to (0,0,0), if the key is empty
            m_center = Vector3(0, 0, 0);
        }
        // Reset the transformed center to the actual center
        m_centerTransformed = m_center;

        // Update the scene graph
        SceneChangeNotify();
    }
}; // class Doom3LightRadius
