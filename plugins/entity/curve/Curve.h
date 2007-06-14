#ifndef CURVE_BASE_H_
#define CURVE_BASE_H_

#include "signal/signal.h"
#include "selectable.h"
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
public:
	// A list of iterators, used for communication with the CurveEditInstance
	typedef std::vector<ControlPoints::iterator> IteratorList;

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
	
	/** greebo: Removes the control points specified by the passed list of iterators.
	 * 			Doesn't do any sanity checking, this has to be done by the calling class.
	 */  
	virtual void removeControlPoints(IteratorList iterators);
	
	/** greebo: Inserts control points before the specified list of iterators.
	 */  
	virtual void insertControlPointsAt(IteratorList iterators);
	
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
	
	// Returns the string representation of this curve to store it into entity spawnargs
	std::string getEntityKeyValue();
	
protected:
	// Clears the control points and other associated elements
	virtual void clearCurve() = 0;
	
	// Parses the curve control points from the given string
	// returns TRUE if successful, FALSE otherwise
	virtual bool parseCurve(const std::string& value);
};

} // namespace entity

#endif /*CURVE_BASE_H_*/
