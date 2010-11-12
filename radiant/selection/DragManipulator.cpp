#include "DragManipulator.h"
#include "Selectors.h"
#include "SelectionTest.h"
#include "Planes.h"

#include "iregistry.h"

const std::string RKEY_TRANSIENT_COMPONENT_SELECTION = "user/ui/transientComponentSelection";

ManipulatorComponent* DragManipulator::getActiveComponent() {
    return _dragSelectable.isSelected() ? &_freeDrag : &_freeResize;
}

void DragManipulator::testSelect(const View& view, const Matrix4& pivot2world) {
    SelectionPool selector;

    SelectionVolume test(view);

    if (GlobalSelectionSystem().Mode() == SelectionSystem::ePrimitive ||
		GlobalSelectionSystem().Mode() == SelectionSystem::eGroupPart)
    {
    	// Find all entities
		BooleanSelector entitySelector;

		EntitySelector selectionTester(entitySelector, test);
		GlobalSceneGraph().foreachVisibleNodeInVolume(view, selectionTester);

    	// Find all primitives that are selectable
		BooleanSelector booleanSelector;

		PrimitiveSelector primitiveTester(booleanSelector, test);
		GlobalSceneGraph().foreachVisibleNodeInVolume(view, primitiveTester);

		if (entitySelector.isSelected()) {
			// Found a selectable entity
			selector.addSelectable(SelectionIntersection(0, 0), &_dragSelectable);
			_selected = false;
		} else if(booleanSelector.isSelected())	{
			// Found a selectable primitive
			selector.addSelectable(SelectionIntersection(0, 0), &_dragSelectable);
			_selected = false;
		}
		else {
			// Check for selectable faces
			_selected = Scene_forEachPlaneSelectable_selectPlanes(selector, test);
		}
    }
    // Check for entities that can be selected
    else if(GlobalSelectionSystem().Mode() == SelectionSystem::eEntity) {
    	// Create a boolean selection pool (can have exactly one selectable or none)
		BooleanSelector booleanSelector;

		// Find the visible entities
		EntitySelector selectionTester(booleanSelector, test);
		GlobalSceneGraph().foreachVisibleNodeInVolume(view, selectionTester);

		// Check, if an entity could be found
      	if (booleanSelector.isSelected()) {
        	selector.addSelectable(SelectionIntersection(0, 0), &_dragSelectable);
        	_selected = false;
		}
    }
    else
    {
		BestSelector bestSelector;

		ComponentSelector selectionTester(bestSelector, test, GlobalSelectionSystem().ComponentMode());
		GlobalSelectionSystem().foreachSelected(selectionTester);

		for (std::list<Selectable*>::iterator i = bestSelector.best().begin();
			 i != bestSelector.best().end(); ++i)
		{
			// greebo: Disabled this, it caused the currently selected patch vertices being deselected.
			if (GlobalRegistry().get(RKEY_TRANSIENT_COMPONENT_SELECTION) == "1")
			{
				if (!(*i)->isSelected())
				{
					GlobalSelectionSystem().setSelectedAllComponents(false);
				}
			}

			_selected = false;
			selector.addSelectable(SelectionIntersection(0, 0), (*i));
			_dragSelectable.setSelected(true);
		}
	}

	for (SelectionPool::iterator i = selector.begin(); i != selector.end(); ++i)
	{
		i->second->setSelected(true);
	}
}

void DragManipulator::setSelected(bool select) {
    _selected = select;
    _dragSelectable.setSelected(select);
}

bool DragManipulator::isSelected() const  {
	return _selected || _dragSelectable.isSelected();
}

