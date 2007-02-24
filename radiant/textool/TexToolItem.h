#ifndef TEXTOOLITEM_H_
#define TEXTOOLITEM_H_

#include "Selectable.h"
#include "Transformable.h"
#include "Renderable.h"
#include <vector>
#include <boost/shared_ptr.hpp>

namespace selection {
	namespace textool {

class TexToolItem;
typedef boost::shared_ptr<TexToolItem> TexToolItemPtr;
typedef std::vector<TexToolItemPtr> TexToolItemVec;

/** greebo: Abstract base class of a TexToolItem visitor class
 */
class ItemVisitor
{
public:
	virtual void visit(TexToolItemPtr texToolItem) = 0;	
};

/** greebo: A TexToolItem is an object that...
 * 
 * ...has a visual representation in the TexTool (Renderable).
 * ...can be selected (Selectable (implicitly derived from Transformable))
 * ...can be transformed (Transformable)
 * ...can have one or more children of the same type, to allow grouping.
 */
class TexToolItem :
	public Renderable,
	public Transformable
{
protected:
	// The list of children of this object
	TexToolItemVec _children;

public:
	/** greebo: Adds the given TexToolItem as child of this Item.
	 * 
	 * Any transformations will affect the children as well. 
	 */
	virtual void addChild(TexToolItemPtr child) {
		_children.push_back(child);
	}
	
	/** greebo: Returns the vector of children of this object.
	 * 
	 * A reference to the internal list of this object is returned. 
	 */
	virtual TexToolItemVec& getChildren() {
		return _children;
	}
	
	virtual void foreachItem(ItemVisitor& visitor) {
		for (unsigned int i = 0; i < _children.size(); i++) {
			// Visit the children
			visitor.visit(_children[i]);
			
			// Propagate the visitor class down the hierarchy
			_children[i]->foreachItem(visitor);
		}
	}
	
	/** greebo: Returns a list of selectable items that correspond
	 * to the given coords. 
	 */
	virtual TexToolItemVec getSelectableChilds(const Rectangle& rectangle) {
		TexToolItemVec returnVector;
		
		for (unsigned int i = 0; i < _children.size(); i++) {
			// Add every children to the list
			if (_children[i]->testSelect(rectangle)) {
				returnVector.push_back(_children[i]);
			}
		}
		
		return returnVector;
	}
	
	/** greebo: Default transform implementation: transform all children.
	 */
	virtual void transform(const Matrix4& matrix) {
		// Cycle through all the children and ask them to render themselves
		for (unsigned int i = 0; i < _children.size(); i++) {
			_children[i]->transform(matrix);
		}
	}
	
	/** greebo: Transforms this object if it's selected only.
	 * 
	 * Default implementation for a TexToolItem: transform self
	 * and pass the call to the children.  
	 */
	virtual void transformSelected(const Matrix4& matrix) {
		// If this object is selected, transform <self>
		if (_selected) {
			transform(matrix);
		}
		else {
			// Object is not selected, propagate the call to the children
			for (unsigned int i = 0; i < _children.size(); i++) {
				_children[i]->transformSelected(matrix);
			}
		}
	}
	
	virtual void flipSelected(const int& axis) {
		// Default behaviour: Propagate the call to the children
		for (unsigned int i = 0; i < _children.size(); i++) {
			_children[i]->flipSelected(axis);
		}
	}
	
	virtual void snapSelectedToGrid(float grid) {
		// Default behaviour: Propagate the call to the children
		for (unsigned int i = 0; i < _children.size(); i++) {
			_children[i]->snapSelectedToGrid(grid);
		}
	}
	
	// Default implementation of getExtents(). All children's AABB are combined.
	virtual AABB getExtents() {
		AABB returnValue;
		
		// Cycle through all the children and include their AABB
		for (unsigned int i = 0; i < _children.size(); i++) {
			returnValue.includeAABB(_children[i]->getExtents());
		}
		
		return returnValue;
	}
	
	virtual AABB getSelectedExtents() {
		AABB returnValue;
		
		// Add <self> to the resulting AABB if <self> is selected
		if (_selected) {
			returnValue.includeAABB(getExtents());
		}
		
		// Cycle through all the children and include their AABB
		for (unsigned int i = 0; i < _children.size(); i++) {
			if (_children[i]->isSelected()) {
				returnValue.includeAABB(_children[i]->getExtents());
			}
		}
		
		return returnValue;
	}
	
	virtual void moveSelectedTo(const Vector2& targetCoords) {
		// Default: Cycle through all children and move the selected
		for (unsigned int i = 0; i < _children.size(); i++) {
			_children[i]->moveSelectedTo(targetCoords);
		}
	}
	
	// Default render routine: ask all children to render their part
	virtual void render() {
		// Cycle through all the children and ask them to render themselves
		for (unsigned int i = 0; i < _children.size(); i++) {
			_children[i]->render();
		}
	}

}; // class TexToolItem

/** greebo: Visitor class to select/deselect all visited TexToolItems
 */
class SetSelectedWalker :
	public selection::textool::ItemVisitor
{
	bool _selected;
public:
	SetSelectedWalker(bool selected) : 
		_selected(selected)
	{}

	void visit(TexToolItemPtr texToolItem) {
		texToolItem->setSelected(_selected);
	}
};

/** greebo: Visitor class to count the selected items
 */
class SelectedCounter :
	public selection::textool::ItemVisitor
{
	int& _counter;
public:
	SelectedCounter(int& counter) : 
		_counter(counter)
	{}

	void visit(TexToolItemPtr texToolItem) {
		if (texToolItem->isSelected()) {
			_counter++;
		}
	}
};

	} // namespace textool
} // namespace selection

#endif /*TEXTOOLITEM_H_*/
