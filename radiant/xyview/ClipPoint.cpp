#include "ClipPoint.h"

#include "igl.h"

#include "math/Vector2.h"
#include "stream/stringstream.h"
#include "plugin.h"

ClipPoint::ClipPoint() {
	reset();
};

void ClipPoint::reset() {
	m_ptClip[0] = m_ptClip[1] = m_ptClip[2] = 0.0;
	m_bSet = false;
}

bool ClipPoint::isSet() const {
	return m_bSet;
}

void ClipPoint::Set(bool b) {
	m_bSet = b;
}

ClipPoint::operator Vector3&() {
	return m_ptClip;
}

/* Drawing clip points */
void ClipPoint::Draw(int num, float scale) {
	StringOutputStream label(4);
	label << num;
	Draw(label.c_str(), scale);
}

void ClipPoint::Draw(const std::string& label, float scale) {
	// draw point
	glPointSize (4);
	glColor3fv(ColourSchemes().getColourVector3("clipper"));
	glBegin (GL_POINTS);
	glVertex3fv(m_ptClip);
	glEnd();
	glPointSize (1);

	float offset = 2.0f / scale;

	// draw label
	glRasterPos3f (m_ptClip[0] + offset, m_ptClip[1] + offset, m_ptClip[2] + offset);
	glCallLists (GLsizei(label.length()), GL_UNSIGNED_BYTE, label.c_str());
}

float fDiff(float f1, float f2) {
	if (f1 > f2)
		return f1 - f2;
	else
		return f2 - f1;
}

double ClipPoint::intersect(const Vector3& point, VIEWTYPE viewtype, float scale) {
	int nDim1 = (viewtype == YZ) ? 1 : 0;
	int nDim2 = (viewtype == XY) ? 1 : 2;
	double screenDistanceSquared(
	    Vector2(
	        fDiff(m_ptClip[nDim1], point[nDim1]) * scale,
	        fDiff(m_ptClip[nDim2], point[nDim2]) * scale).getLengthSquared()
	);
	if (screenDistanceSquared < 8*8) {
		return screenDistanceSquared;
	}
	return FLT_MAX;
}

void ClipPoint::testSelect(const Vector3& point, VIEWTYPE viewtype, float scale, double& bestDistance, ClipPoint*& bestClip) {
	if (isSet()) {
		double distance = intersect(point, viewtype, scale);
		
		if (distance < bestDistance) {
			bestDistance = distance;
			bestClip = this;
		}
	}
}

