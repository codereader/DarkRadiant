#ifndef CURVE_BASE_H_
#define CURVE_BASE_H_

#include "signal/signal.h"
#include "math/curve.h"
#include "math/aabb.h"
#include "RenderableCurve.h"

namespace entity {

/** greebo: This is the base class for the two Doom3-supported curve
 * 			subclasses CurveNURBS and CurveCatmullRomSpline.
 * 
 * 			Both curve types share a set of methods, that's why I though
 * 			it makes sense to move them here.
 */
class Curve
{
protected:
	ControlPoints _controlPoints;
	ControlPoints _controlPointsTransformed;
	
	RenderableCurve _renderCurve;
	AABB _bounds;
	
	Callback _boundsChanged;
	Signal0 _curveChanged;
public:
	Curve(const Callback& boundsChanged);
	
	// "Curve changed" signal stuff
	SignalHandlerId connect(const SignalHandler& curveChanged);
	void disconnect(SignalHandlerId id);
	
	bool isEmpty() const;
	
	// Tesselation has to be implemented by the subclasses
	virtual void tesselate() = 0;
	
	// This gets called when the entity keyvalue changes 
	void curveKeyChanged(const std::string& value);
	typedef MemberCaller1<Curve, const std::string&, &Curve::curveKeyChanged> CurveChangedCaller;
	
	// Appens <numPoints> elements at the end of the control point list
	virtual void appendControlPoints(unsigned int numPoints);
	
protected:
	// Clears the control points and other associated elements
	virtual void clearCurve() = 0;

	// Gets called after the control points have changed
	void curveChanged();
	
	// Parses the curve control points from the given string
	// returns TRUE if successful, FALSE otherwise
	virtual bool parseCurve(const std::string& value);
};

} // namespace entity

#endif /*CURVE_BASE_H_*/
