#ifndef TEXDEF_H_
#define TEXDEF_H_

#include "itexdef.h"
#include "math/matrix.h"

class TexDef : public GenericTextureDefinition {
public:
	// Constructor
	TexDef();
	
	// Constructs a TexDef out of the given transformation matrix plus width/height
	TexDef(double width, double height, const Matrix4& transform);
	
	void shift(double s, double t);
	void scale(double s, double t);
	void rotate(double angle);

	// Checks the TexDef for insanely large values
	bool isSane() const;
	
	// All texture-projection translation (shift) values are congruent modulo the dimensions of the texture.
	// This function normalises shift values to the smallest positive congruent values.
	void normalise(double width, double height);
		
	/* Construct a transform in ST space from the texdef.
	 * Transforms constructed from quake's texdef format 
	 * are (-shift)*(1/scale)*(-rotate) with x translation sign flipped.
	 * This would really make more sense if it was inverseof(shift*rotate*scale).. oh well.*/
	Matrix4 getTransform(double width, double height) const;
};

inline std::ostream& operator<< (std::ostream& st, const TexDef& texdef) {
	st << "Scale: <" << texdef._scale[0] << ", " << texdef._scale[1] << ">, ";
	st << "Rotation: <" << texdef._rotate << ">";
	return st;
}

#endif /*TEXDEF_H_*/
