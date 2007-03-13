#ifndef TEXTURABLE_H_
#define TEXTURABLE_H_

#include "selectable.h"
#include "scenelib.h"
class Face;
class Brush;
class Patch;

namespace selection {

/** greebo: An abstract Texturable object, can be a patch, brush or face.
 */
class Texturable
{
public:
	Face* face;
	Brush* brush;
	Patch* patch;

	// Constructor
	Texturable();
	
	// Clears the pointers
	void clear();
};

namespace algorithm {

class ClosestTexturableFinder :
	public scene::Graph::Walker
{
	Texturable& _texturable;
	SelectionTest& _selectionTest;
	
	// To store the best intersection candidate
	mutable SelectionIntersection _bestIntersection;
public:
	// Constructor
	ClosestTexturableFinder(SelectionTest& test, Texturable& texturable);
	
	// The visitor function
	bool pre(const scene::Path& path, scene::Instance& instance) const;
};
		
} // namespace algorithm

} // namespace selection

#endif /*TEXTURABLE_H_*/
