#pragma once

#include "generic/callback.h"
#include "iselectiontest.h"
#include "ientity.h"
#include "math/curve.h"
#include "math/AABB.h"
#include "RenderableCurve.h"

namespace entity 
{

/** greebo: This is the base class for the two Doom3-supported curve
 * 			subclasses CurveNURBS and CurveCatmullRomSpline.
 *
 * 			Both curve types share a set of methods, that's why I though
 * 			it makes sense to move them here.
 */
class Curve :
	public KeyObserver
{
public:
	// A list of iterators, used for communication with the CurveEditInstance
	typedef std::vector<ControlPoints::iterator> IteratorList;

	typedef std::function<void()> CurveChangedCallback;

protected:
	ControlPoints _controlPoints;
	ControlPoints _controlPointsTransformed;

	RenderableCurve _renderCurve;
	AABB _bounds;

	Callback _boundsChanged;
    sigc::signal<void> _sigCurveChanged;

public:
	Curve(const IEntityNode& entity, const Callback& boundsChanged);

	virtual ~Curve() {}

	bool isEmpty() const;

	const AABB& getBounds() const;

    /// Signal emitted when curve changes
    sigc::signal<void> signal_curveChanged()
    {
        return _sigCurveChanged;
    }

	// Tesselation has to be implemented by the subclasses
	virtual void tesselate() = 0;

	// This gets called when the entity keyvalue changes
	void onKeyValueChanged(const std::string& value);

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

	// Render methods
    void onPreRender(const ShaderPtr& shader, const VolumeTest& volume);
    void renderHighlights(IRenderableCollector& collector, const VolumeTest& volume);

	// Performs a selection test on the point vertices of this curve
	void testSelect(Selector& selector, SelectionTest& test, SelectionIntersection& best);

	// Legacy compatibility
	ControlPoints& getTransformedControlPoints();
	ControlPoints& getControlPoints();

	// Returns the string representation of this curve to store it into entity spawnargs
	std::string getEntityKeyValue();

    void clearRenderable();
    void updateRenderable();

protected:
	// Clears the control points and other associated elements
	virtual void clearCurve() = 0;

	// Parses the curve control points from the given string
	// returns TRUE if successful, FALSE otherwise
	virtual bool parseCurve(const std::string& value);
};

} // namespace entity
