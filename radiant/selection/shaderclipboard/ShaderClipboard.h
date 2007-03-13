#ifndef SHADERCLIPBOARD_H_
#define SHADERCLIPBOARD_H_

#include "Texturable.h"

namespace selection {

class ShaderClipboard
{
	// The source and target Texturables
	Texturable _source;
	Texturable _target;

public:
	ShaderClipboard();

	/** greebo: Tries to find the best source Texturable object
	 * 			with the given SelectionTest (SelectionVolume)
	 */
	void setSource(SelectionTest& test);

	/** greebo: Clears both the source and target texturables.
	 * 			Call this as soon as the objects might be deleted
	 * 			or the map is changed.
	 */
	void clear();
	
private:

	/** greebo: Retrieves the best texturable object from the
	 * 			given SelectionTest.
	 */
	Texturable getTexturable(SelectionTest& test);
};

} // namespace selection

selection::ShaderClipboard& GlobalShaderClipboard();

#endif /*SHADERCLIPBOARD_H_*/
