#ifndef CURVE_BASE_H_
#define CURVE_BASE_H_

#include "signal/signal.h"
#include "selectable.h"
#include "math/curve.h"
#include "math/aabb.h"
#include "stream/stringstream.h"
#include "RenderableCurve.h"

namespace entity {

	// greebo: TODO: Migrate this to std::string
	inline void ControlPoints_writeToStream(const ControlPoints& controlPoints, StringOutputStream& value)
	{
	  value << Unsigned(controlPoints.size()) << " (";
	  for(ControlPoints::const_iterator i = controlPoints.begin(); i != controlPoints.end(); ++i)
	  {
	    value << " " << (*i).x() << " " << (*i).y() << " " << (*i).z() << " ";
	  }
	  value << ")";
	}
	
	// greebo: TODO: See above
	inline void ControlPoints_write(ControlPoints& controlPoints, const std::string& key, Entity& entity)
	{
	  StringOutputStream value(256);
	  if(!controlPoints.empty())
	  {
	    ControlPoints_writeToStream(controlPoints, value);
	  }
	  entity.setKeyValue(key, value.c_str());
	}

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
	
	const AABB& getBounds() const;
	
	// Tesselation has to be implemented by the subclasses
	virtual void tesselate() = 0;
	
	// This gets called when the entity keyvalue changes 
	void curveKeyChanged(const std::string& value);
	typedef MemberCaller1<Curve, const std::string&, &Curve::curveKeyChanged> CurveChangedCaller;
	
	// Appens <numPoints> elements at the end of the control point list
	virtual void appendControlPoints(unsigned int numPoints);
	
	// Gets called after the control points have changed
	void curveChanged();
	
	// Reverts/freezes the transformation
	void revertTransform();
	void freezeTransform();
	
	// Each subclass has to implement this save method
	// which writes the points to the spawnargs
	virtual void saveToEntity(Entity& target) = 0;
	
	// Front-end render method
	void renderSolid(Renderer& renderer, const VolumeTest& volume, 
					 const Matrix4& localToWorld) const;
					 
	// Performs a selection test on the point vertices of this curve
	void testSelect(Selector& selector, SelectionTest& test, SelectionIntersection& best);
	
	// Legacy compatibility
	ControlPoints& getTransformedControlPoints();
	ControlPoints& getControlPoints();
	
protected:
	// Clears the control points and other associated elements
	virtual void clearCurve() = 0;
	
	// Parses the curve control points from the given string
	// returns TRUE if successful, FALSE otherwise
	virtual bool parseCurve(const std::string& value);
};

} // namespace entity

#endif /*CURVE_BASE_H_*/
