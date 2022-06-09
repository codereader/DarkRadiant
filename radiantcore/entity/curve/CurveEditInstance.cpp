#include "CurveEditInstance.h"

#include "debugging/debugging.h"
#include "itextstream.h"
#include "CurveControlPointFunctors.h"

namespace entity {

CurveEditInstance::CurveEditInstance(Curve& curve, const SelectionChangedSlot& selectionChanged) :
	_curve(curve),
    _selectionChanged(selectionChanged),
    _controlPointsTransformed(_curve.getTransformedControlPoints()),
    _controlPoints(_curve.getControlPoints())
{}

void CurveEditInstance::testSelect(Selector& selector, SelectionTest& test)
{
    ASSERT_MESSAGE(_controlPointsTransformed.size() == _selectables.size(), "curve instance mismatch");

    ControlPoints::const_iterator p = _controlPointsTransformed.begin();

    for(Selectables::iterator i = _selectables.begin(); i != _selectables.end(); ++i, ++p)
    {
    	SelectionIntersection best;
		test.TestPoint(*p, best);
		if (best.isValid())
		{
			selector.addWithIntersection(*i, best);
		}
    }
}

bool CurveEditInstance::isSelected() const {
    for(Selectables::const_iterator i = _selectables.begin(); i != _selectables.end(); ++i)
    {
      if (i->isSelected()) {
        return true;
      }
    }
    return false;
}

void CurveEditInstance::invertSelected()
{
	for (selection::ObservedSelectable& i : _selectables)
	{
		i.setSelected(!i.isSelected());
	}
}

void CurveEditInstance::setSelected(bool selected) {
	for(Selectables::iterator i = _selectables.begin(); i != _selectables.end(); ++i) {
		i->setSelected(selected);
	}
}

unsigned int CurveEditInstance::numSelected() const {
	unsigned int returnValue = 0;

	for (Selectables::const_iterator i = _selectables.begin(); i != _selectables.end(); ++i) {
		if (i->isSelected()) {
			returnValue++;
		}
	}

    return returnValue;
}

Curve::IteratorList CurveEditInstance::getSelected() {
	// This is the list of iterators to be removed
	Curve::IteratorList iterators;

	ControlPoints::iterator p = _controlPointsTransformed.begin();
    for (Selectables::const_iterator i = _selectables.begin(); i != _selectables.end(); ++i, ++p)
    {
    	if (i->isSelected()) {
    		// This control vertex should be removed, add it to the list
    		iterators.push_back(p);
    	}
    }

    return iterators;
}

void CurveEditInstance::removeSelectedControlPoints() {
	unsigned int numPointsToRemove = numSelected();

	if (numPointsToRemove == 0) {
		rError() << "Can't remove any points, no control vertices selected.\n";
		return;
	}

	if (_controlPoints.size() - numPointsToRemove < 3) {
		// Can't remove so many points
		rError() << "Can't remove so many points, curve would end up with less than 3 points.\n";
		return;
	}

	// This is the list of iterators to be removed
	Curve::IteratorList iterators = getSelected();

	// De-select everything before removal
    setSelected(false);

    // Now remove the points
    _curve.removeControlPoints(iterators);
}

void CurveEditInstance::insertControlPointsAtSelected() {
	unsigned int numPointsToRemove = numSelected();

	if (numPointsToRemove == 0) {
		rError() << "Can't insert any points, no control vertices selected.\n";
		return;
	}

	// This is the list of insert points
	Curve::IteratorList iterators = getSelected();

    // De-select everything before removal
    setSelected(false);

    // Now remove the points
    _curve.insertControlPointsAt(iterators);
}

void CurveEditInstance::write(const std::string& key, Entity& entity) {
	std::string value = _curve.getEntityKeyValue();
	entity.setKeyValue(key, value);
}

void CurveEditInstance::transform(const Matrix4& matrix, bool selectedOnly) {
	ControlPointTransformator transformator(matrix);

  	if (selectedOnly) {
    	forEachSelected(transformator);
  	}
  	else {
  		forEach(transformator);
  	}
}

void CurveEditInstance::snapto(float snap) {
	ControlPointSnapper snapper(snap);
    forEachSelected(snapper);
}

void CurveEditInstance::curveChanged()
{
    _selectables.resize(_controlPointsTransformed.size(), _selectionChanged);
}

void CurveEditInstance::forEachSelected(ControlPointFunctor& functor) {
	ASSERT_MESSAGE(_controlPointsTransformed.size() == _selectables.size(), "curve instance mismatch");
	ControlPoints::iterator transformed = _controlPointsTransformed.begin();
	ControlPoints::const_iterator original = _controlPoints.begin();

	for (Selectables::iterator i = _selectables.begin();
		 i != _selectables.end();
		 ++i, ++transformed, ++original)
	{
		if (i->isSelected()) {
    		functor(*transformed, *original);
  		}
	}
}

void CurveEditInstance::forEachControlPoint(const std::function<void(const Vector3&, bool)>& functor) const
{
    ASSERT_MESSAGE(_controlPointsTransformed.size() == _selectables.size(), "curve instance mismatch");

    auto transformed = _controlPointsTransformed.begin();

    for (auto i = _selectables.begin(); i != _selectables.end(); ++i, ++transformed)
    {
        functor(*transformed, i->isSelected());
    }
}

void CurveEditInstance::forEachSelected(ControlPointConstFunctor& functor) const {
	ASSERT_MESSAGE(_controlPointsTransformed.size() == _selectables.size(), "curve instance mismatch");
	ControlPoints::const_iterator transformed = _controlPointsTransformed.begin();
	ControlPoints::const_iterator original = _controlPoints.begin();

	for (Selectables::const_iterator i = _selectables.begin();
		 i != _selectables.end();
		 ++i, ++transformed, ++original)
	{
		if (i->isSelected()) {
    		functor(*transformed, *original);
  		}
	}
}

void CurveEditInstance::forEach(ControlPointFunctor& functor) {
	ControlPoints::const_iterator original = _controlPoints.begin();
	for (ControlPoints::iterator i = _controlPointsTransformed.begin();
    	i != _controlPointsTransformed.end();
    	++i, ++original)
    {
		functor(*i, *original);
    }
}

} // namespace entity
