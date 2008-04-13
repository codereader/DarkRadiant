#include "ClipPoint.h"

#include "igl.h"

#include "math/Vector2.h"
#include "stream/textstream.h"
#include "string/string.h"
#include "iradiant.h"

ClipPoint::ClipPoint() {
	reset();
};

void ClipPoint::reset() {
	_coords[0] = _coords[1] = _coords[2] = 0.0;
	m_bSet = false;
}

bool ClipPoint::isSet() const {
	return m_bSet;
}

void ClipPoint::Set(bool b) {
	m_bSet = b;
}

ClipPoint::operator Vector3&() {
	return _coords;
}

/* Drawing clip points */
void ClipPoint::Draw(int num, float scale) {
	Draw(intToStr(num), scale);
}

void ClipPoint::Draw(const std::string& label, float scale) {
	// draw point
	glBegin (GL_POINTS);
	glVertex3dv(_coords);
	glEnd();

	float offset = 2.0f / scale;

	// draw label
	glRasterPos3f (_coords[0] + offset, _coords[1] + offset, _coords[2] + offset);
	glCallLists (GLsizei(label.length()), GL_UNSIGNED_BYTE, label.c_str());
}

float fDiff(float f1, float f2) {
	if (f1 > f2)
		return f1 - f2;
	else
		return f2 - f1;
}

double ClipPoint::intersect(const Vector3& point, EViewType viewtype, float scale) {
	int nDim1 = (viewtype == YZ) ? 1 : 0;
	int nDim2 = (viewtype == XY) ? 1 : 2;
	double screenDistanceSquared(
	    Vector2(
	        fDiff(_coords[nDim1], point[nDim1]) * scale,
	        fDiff(_coords[nDim2], point[nDim2]) * scale).getLengthSquared()
	);
	if (screenDistanceSquared < 8*8) {
		return screenDistanceSquared;
	}
	return FLT_MAX;
}

void ClipPoint::testSelect(const Vector3& point, EViewType viewtype, float scale, double& bestDistance, ClipPoint*& bestClip) {
	if (isSet()) {
		double distance = intersect(point, viewtype, scale);
		
		if (distance < bestDistance) {
			bestDistance = distance;
			bestClip = this;
		}
	}
}

