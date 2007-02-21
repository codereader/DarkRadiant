#ifndef TEXTOOLITEM_H_
#define TEXTOOLITEM_H_

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
	
	virtual AABB getExtents() = 0;
	
	/** greebo: Returns a list of selectable items that correspond
	 * to the given coords. 
	 */
	virtual TexToolItemVec getSelectables(const Rectangle& rectangle) = 0;
};

	} // namespace textool
} // namespace selection

#endif /*TEXTOOLITEM_H_*/
