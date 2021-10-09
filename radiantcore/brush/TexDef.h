#pragma once

#include <iostream>
#include "ibrush.h"

class TexDef final
{
private:
    ShiftScaleRotation _def;

public:
	// Constructs a TexDef out of the given transformation matrix plus width/height
	TexDef(double width, double height, const Matrix4& transform);

    // Construct this TexDef from the given SSR structure
    TexDef(const ShiftScaleRotation& ssr);

	Vector2 getShift() const;
	void setShift(const Vector2& shift);
	Vector2 getScale() const;
	void setScale(const Vector2& scale);
	double getRotation() const;
	void setRotation(double rotation);

	void shift(double s, double t);
	void scale(double s, double t);
	void rotate(double angle);

	// Checks the TexDef for insanely large values
	bool isSane() const;

	// All texture-projection translation (shift) values are congruent modulo the dimensions of the texture.
	// This function normalises shift values to the smallest positive congruent values.
	void normalise(double width, double height);

	// Converts this instance's values to a ShiftScaleRotation structure
	// Since TexDef is using the same format to store its values internally
	// this is equivalent to a few simple assignment or copy operations.
	ShiftScaleRotation toShiftScaleRotation() const;

	static TexDef CreateFromShiftScaleRotation(const ShiftScaleRotation& scr);

	friend std::ostream& operator<<(std::ostream& st, const TexDef& texdef);
};

inline std::ostream& operator<<(std::ostream& st, const TexDef& texdef)
{
	st << "Scale: <" << texdef.getScale()[0] << ", " << texdef.getScale()[1] << ">, ";
	st << "Rotation: <" << texdef.getRotation() << ">";
	return st;
}
