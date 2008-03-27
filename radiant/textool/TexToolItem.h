#ifndef TEXTOOLITEM_H_
#define TEXTOOLITEM_H_

#include "Selectable.h"
#include "Transformable.h"
#include "RenderableItem.h"
#include <vector>
#include <boost/shared_ptr.hpp>

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
 * 
 * The virtual default implementations mainly cycle through all the children
 * of this item and pass the call. All the default transformation routines
 * call the object's update() method when finished.
 * 
 * This object should technically be instantiable, but won't do anything but
 * cycling through its (empty) children list. Hence the according methods
 * have to be overridden in order to make a real TexToolItem. 
 */
class TexToolItem :
	public RenderableItem,
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
	virtual void addChild(TexToolItemPtr child);
	
	/** greebo: Returns the vector of children of this object.
	 * 
	 * A reference to the internal list of this object is returned. 
	 */
	virtual TexToolItemVec& getChildren();
	
	virtual void foreachItem(ItemVisitor& visitor);
	
	/** greebo: Returns a list of selectable items that correspond
	 * to the given coords. 
	 */
	virtual TexToolItemVec getSelectableChilds(const Rectangle& rectangle);
	
	/** greebo: Default transform implementation: transform all children.
	 */
	virtual void transform(const Matrix4& matrix);
	
	/** greebo: Transforms this object if it's selected only.
	 * 
	 * Default implementation for a TexToolItem: transform self
	 * and pass the call to the children.  
	 */
	virtual void transformSelected(const Matrix4& matrix);
	
	virtual void flipSelected(const int& axis);
	
	virtual void snapSelectedToGrid(float grid);
	
	// Default implementation of getExtents(). All children's AABB are combined.
	virtual AABB getExtents();
	
	virtual AABB getSelectedExtents();
	
	virtual void moveSelectedTo(const Vector2& targetCoords);
	
	// Default render routine: ask all children to render their part
	virtual void render();
	
	// Default beginTransformation routine: propagate the call to children
	virtual void beginTransformation();
	
	// Default endTransformation routine: propagate the call to children
	virtual void endTransformation();
	
	/** greebo: Selects all siblings if a child node is selected.
	 */
	virtual void selectRelated();
	
	/** greebo: This tells the Transformable to sync up their source objects
	 * 			(e.g. by calling Patch::controlPointsChanged()) to make
	 * 			the changes visible in the scenegraph.
	 */ 
	virtual void update();

}; // class TexToolItem

/** greebo: Visitor class to select/deselect all visited TexToolItems
 */
class SetSelectedWalker :
	public textool::ItemVisitor
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
	public textool::ItemVisitor
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

#endif /*TEXTOOLITEM_H_*/
