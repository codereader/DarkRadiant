#pragma once

#include "math/Quaternion.h"
#include <functional>

class Entity;

namespace entity
{

class AngleKey
{
private:
	std::function<void()> _angleChanged;

public:
    static const float IDENTITY;

    float m_angle;

    AngleKey(const std::function<void()>& angleChanged) : 
        _angleChanged(angleChanged), 
        m_angle(IDENTITY)
    {}

    // Callback method invoked when the corresponding spawnarg changes
    void angleChanged(const std::string& value);

    void write(Entity* entity) const
    {
        writeToEntity(m_angle, entity);
    }

    // Writes the given angle value as angle spawnarg to the given entity
    // Doesn't remove a pre-existing angle key even if its future value will be 0
    static void writeToEntity(float angle, Entity* entity);

    // Returns the new angle value rotated by the given quaternion
    static float getRotatedValue(float angle, const Quaternion& rotation);

private:
    float getNormalisedAngle(float angle)
    {
	    return float_mod(angle, 360.0f);
    }
};

} // namespace
