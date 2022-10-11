#pragma once

#include "math/Quaternion.h"
#include "math/FloatTools.h"
#include <sigc++/trackable.h>
#include <functional>

class Entity;

namespace entity
{

/// Wrapper for an "angle" spawnarg
class AngleKey: public sigc::trackable
{
private:
	std::function<void()> _angleChanged;

    float _value;

public:
    static const float IDENTITY;

    AngleKey(const std::function<void()>& angleChanged) :
        _angleChanged(angleChanged),
        _value(IDENTITY)
    {}

    float getValue() const
    {
        return _value;
    }

    void setValue(float value)
    {
        _value = value;
    }

    // Callback method invoked when the corresponding spawnarg changes
    void angleChanged(const std::string& value);

    void write(Entity* entity) const
    {
        writeToEntity(_value, entity);
    }

    // Writes the given angle value as angle spawnarg to the given entity
    // Doesn't remove a pre-existing angle key even if its future value will be 0
    static void writeToEntity(double angle, Entity* entity);

    // Returns the new angle value rotated by the given quaternion
    static float getRotatedValue(float angle, const Quaternion& rotation);

private:
    float getNormalisedAngle(float angle)
    {
	    return float_mod(angle, 360.0f);
    }
};

} // namespace
