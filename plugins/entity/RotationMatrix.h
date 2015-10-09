#pragma once

#include <string>

#include "math/Quaternion.h"

class Entity;

/// Nine-element matrix representing a rotation in 3D space
class RotationMatrix
{
public:
	float rotation[9];

	RotationMatrix()
	{
		setIdentity();
	}

	/**
     * Cast to float*, also enables operator[].
	 */
	operator float* () {
		return rotation;
	}

	/**
     * Cast to const float* to provide operator[] for const objects.
	 */
	operator const float* () const {
		return rotation;
	}

	// Reverts this rotation to the identity matrix
	void setIdentity();

	void readFromString(const std::string& value);

	void writeToEntity(Entity* entity, const std::string& key = "rotation") const;

    // Returns the string representation of this rotation
    // suitable for writing it to the entity's key/value pairs.
    std::string getRotationKeyValue() const;

	Matrix4 getMatrix4() const;

	void setFromMatrix4(const Matrix4& matrix);

	// Assignment operator
	const RotationMatrix& operator=(const RotationMatrix& other);

	void rotate(const Quaternion& rotate);

	void setFromAngleString(const std::string& value);
};


