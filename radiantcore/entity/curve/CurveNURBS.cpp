#include "CurveNURBS.h"

namespace entity {

	namespace {
		const int NURBS_degree = 3;
	}

CurveNURBS::CurveNURBS(const IEntityNode& entity, const Callback& callback) :
	Curve(entity, callback)
{}

void CurveNURBS::tesselate() {
	if (!_controlPointsTransformed.empty()) {
		const std::size_t numSegments = (_controlPointsTransformed.size() - 1) * 16;

		_renderCurve.m_vertices.resize(numSegments + 1);
		_renderCurve.m_vertices[0].vertex = Vertex3(_controlPointsTransformed[0]);

		for (std::size_t i = 1; i < numSegments; ++i) {
			_renderCurve.m_vertices[i].vertex = Vertex3(
				NURBS_evaluate(_controlPointsTransformed,
							   _weights,
							   _knots,
							   NURBS_degree,
							   (1.0 / double(numSegments)) * double(i)
				)
			);
		}
		_renderCurve.m_vertices[numSegments].vertex = Vertex3(
			_controlPointsTransformed[_controlPointsTransformed.size() - 1]
		);
	}
	else {
		// No control points found, clear the rendercurve object
		_renderCurve.m_vertices.clear();
	}
}

void CurveNURBS::clearCurve() {
	_controlPoints.resize(0);
	_knots.resize(0);
	_weights.resize(0);
}

void CurveNURBS::saveToEntity(Entity& target) {
	std::string value = getEntityKeyValue();
	target.setKeyValue(curve_Nurbs, value);
}

bool CurveNURBS::parseCurve(const std::string& value) {
	// Let the base class do its job
	bool returnValue = Curve::parseCurve(value);

	// If parsing was successful, do the weighting
	if (returnValue) {
		doWeighting();
	}

	return returnValue;
}

void CurveNURBS::appendControlPoints(unsigned int numPoints) {
	// Pass the call to the base class first
	Curve::appendControlPoints(numPoints);

	// Do the weighting calculations and recalculate tesselation
	doWeighting();
	curveChanged();
}

void CurveNURBS::removeControlPoints(IteratorList iterators) {
	// Pass the call to the base class first
	Curve::removeControlPoints(iterators);

	// Do the weighting calculations and recalculate tesselation
	doWeighting();
	curveChanged();
}

void CurveNURBS::insertControlPointsAt(IteratorList iterators) {
	// Pass the call to the base class first
	Curve::insertControlPointsAt(iterators);

	// Do the weighting calculations and recalculate tesselation
	doWeighting();
	curveChanged();
}

void CurveNURBS::doWeighting() {
	// Re-adjust the weights
	_weights.resize(_controlPoints.size());

	// All the weights are set to 1
	for(NURBSWeights::iterator i = _weights.begin(); i != _weights.end(); ++i) {
		(*i) = 1;
	}

	// greebo: Recalculate the knots (?)
	KnotVector_openUniform(_knots, _controlPoints.size(), NURBS_degree);
}

} // namespace entity
