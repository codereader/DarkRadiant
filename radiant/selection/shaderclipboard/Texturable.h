#ifndef TEXTURABLE_H_
#define TEXTURABLE_H_

#include <string>
#include "selectable.h"
#include "scenelib.h"
class Face;
class Brush;
class Patch;

namespace selection {

/** greebo: An abstract Texturable object, can be a patch, brush or face
 * 			or just a shader name.
 */
class Texturable
{
public:
	Face* face;
	Brush* brush;
	Patch* patch;
	std::string shader;

	// Constructor
	Texturable();
	
	// True, if all the data is NULL or empty
	bool empty() const;
	
	// True according to the pointer state
	bool isPatch() const;
	bool isFace() const;
	bool isShader() const;
	
	// Returns the shader from the face/patch stored within 
	std::string getShader() const;
	
	// Clears the pointers
	void clear();
};

namespace algorithm {

class ClosestTexturableFinder :
	public scene::NodeVisitor
{
	Texturable& _texturable;
	SelectionTest& _selectionTest;
	
	// To store the best intersection candidate
	SelectionIntersection _bestIntersection;
public:
	// Constructor
	ClosestTexturableFinder(SelectionTest& test, Texturable& texturable);
	
	// The visitor function
	bool pre(const scene::INodePtr& node);
};
		
} // namespace algorithm

} // namespace selection

#endif /*TEXTURABLE_H_*/
