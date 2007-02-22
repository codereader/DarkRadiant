#ifndef TEXTOOLITEM_H_
#define TEXTOOLITEM_H_

#include "itextstream.h"
#include "Selectable.h"
#include "Transformable.h"
#include "Renderable.h"
#include <vector>
#include <boost/shared_ptr.hpp>

class AABB;

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
 * ...can be selected (Selectable)
 * ...can be transformed (Transformable)
 * ...can have one or more children of the same type, to allow grouping.
 */
class TexToolItem :
	public Selectable,
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
	
	virtual AABB getExtents() = 0;
	
	/** greebo: Returns a list of selectable items that correspond
	 * to the given coords. 
	 */
	virtual TexToolItemVec getSelectables(const Rectangle& rectangle) = 0;
	
	/** greebo: Tells the attached object to prepare for transformation,
	 * 			this includes saving their current state for later undo.
	 */
	virtual void beginTransformation() {
		// Empty default implementation
	}
};

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
		globalOutputStream() << "visit called\n";
		texToolItem->setSelected(_selected);
	}
};

	} // namespace textool
} // namespace selection

#endif /*TEXTOOLITEM_H_*/
