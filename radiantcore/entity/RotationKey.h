#pragma once

#include "ientity.h"

#include "math/Quaternion.h"
#include <functional>

#include "AngleKey.h"
#include "RotationMatrix.h"

namespace entity
{

class RotationKey: public sigc::trackable
{
private:
	std::function<void()> m_rotationChanged;
public:
	RotationMatrix m_rotation;

	RotationKey(const std::function<void()>& rotationChanged) :
		m_rotationChanged(rotationChanged)
	{}

	void angleChanged(const std::string& value)
	{
		m_rotation.setFromAngleString(value);
		m_rotationChanged();
	}

	void rotationChanged(const std::string& value)
	{
		m_rotation.readFromString(value);
		m_rotationChanged();
	}

	void write(Entity* entity, bool isModel = false) const;
};

} // namespace
