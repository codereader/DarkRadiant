#include "CurveCatmullRom.h"

namespace entity {

CurveCatmullRom::CurveCatmullRom(const IEntityNode& entity, const Callback& callback) :
	Curve(entity, callback)
{}

void CurveCatmullRom::clearCurve() {
	_controlPoints.resize(0);
}

void CurveCatmullRom::appendControlPoints(unsigned int numPoints) {
	// Pass the call to the base class first
	Curve::appendControlPoints(numPoints);

	// Recalculate tesselation and emit the signals
	curveChanged();
}

void CurveCatmullRom::removeControlPoints(IteratorList iterators) {
	// Pass the call to the base class first
	Curve::removeControlPoints(iterators);

	// Recalculate the tesselation and emit the signals
	curveChanged();
}

void CurveCatmullRom::insertControlPointsAt(IteratorList iterators) {
	// Pass the call to the base class first
	Curve::insertControlPointsAt(iterators);

	// Recalculate tesselation and emit the signals
	curveChanged();
}

void CurveCatmullRom::tesselate() {
	if (!_controlPointsTransformed.empty()) {
		const std::size_t numSegments = (_controlPointsTransformed.size() - 1) * 16;
		_renderCurve.m_vertices.resize(numSegments + 1);
		_renderCurve.m_vertices[0].vertex = Vertex3f(_controlPointsTransformed[0]);

		for(std::size_t i = 1; i < numSegments; ++i) {
			_renderCurve.m_vertices[i].vertex = Vertex3f(
				CatmullRom_evaluate(_controlPointsTransformed,
									(1.0 / double(numSegments)) * double(i)
				)
			);
		}
		_renderCurve.m_vertices[numSegments].vertex = Vertex3f(
			_controlPointsTransformed[_controlPointsTransformed.size() - 1]
		);
	}
	else {
		_renderCurve.m_vertices.clear();
	}
}

void CurveCatmullRom::saveToEntity(Entity& target) {
	std::string value = getEntityKeyValue();
	target.setKeyValue(curve_CatmullRomSpline, value);
}

} // namespace entity
