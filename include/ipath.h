#ifndef IPATH_H_
#define IPATH_H_

#include "inode.h"
#include <vector>

namespace scene {

/** greebo: This is the base structure used as unique to scenegraph elements.
 * 
 * 			It extends the functionality of std::vector to mimick
 * 			the interface of a std::stack (push(), top(), pop()) and
 * 			provides an additional parent() method that allows
 * 			to retrieve the element right below the top() element.
 * 
 * 			The parent() method is a convenience method helping to insert 
 * 			scenegraph elements at the right place without complicated
 * 			popping() and pushing().
 * 
 * Note: Normally, deriving from STL containers is not a good idea, due to
 *       the non-virtual destructors (all data members in the subclasses
 *       wouldn't get destroyed). In this case it is ok to derive from
 *       std::vector, because no additional data members are introduced. 
 */
class Path :
	public std::vector<INodePtr>
{
public:
	// Default constructor
	Path() 
	{}

	// Constructor taking an initial element. 
	// The stack is starting with one top element. 
	Path(const INodePtr& initialElement) {
		push(initialElement);
	}

	// Accessor method to retrieve the last element inserted.
	INodePtr& top() {
		return back();
	}
	
	// Accessor method to retrieve the last element inserted.
	const INodePtr& top() const {
		return back();
	}
	
	// Accessor method to retrieve the element below the last inserted.
	INodePtr& parent() {
		return *(std::vector<INodePtr>::end()-2);
	}
	
	// Accessor method to retrieve the element below the last inserted.
	const INodePtr& parent() const {
		return *(std::vector<INodePtr>::end()-2);
	}
	
	// Add an element to the stack, this becomes the top() element
	void push(const INodePtr& newElement) {
		push_back(newElement);
	}
	
	// Remove the topmost element, the size of the stack is reduced by 1.
	void pop() {
		pop_back();
	}
}; // class Path

} // namespace scene

#endif /*IPATH_H_*/
