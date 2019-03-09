#include "AngleKey.h"

#include "ientity.h"
#include "string/convert.h"
#include "math/Matrix4.h"

namespace entity
{

const float AngleKey::IDENTITY = 0;

void AngleKey::angleChanged(const std::string& value)
{
    _value = getNormalisedAngle(string::convert<float>(value, 0));

    _angleChanged();
}

void AngleKey::writeToEntity(float angle, Entity* entity)
{
    // To resolve #3150, let's keep existing "angle" keys even if their value is "0"
    bool hadAngleValue = !entity->getKeyValue("angle").empty();

    entity->setKeyValue("angle", (angle == 0 && !hadAngleValue) ? "" : string::to_string(angle));
}

float AngleKey::getRotatedValue(float angle, const Quaternion& rotation)
{
	return static_cast<float>(Matrix4::getRotationAboutZDegrees(angle).getMultipliedBy(
		Matrix4::getRotationQuantised(rotation)
	).getEulerAnglesXYZDegrees().z());
}

} // namespace

