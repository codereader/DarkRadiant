#include "ShaderClipboard.h"

#include "selectable.h"
#include "iscenegraph.h"

namespace selection {

ShaderClipboard::ShaderClipboard()
{}

void ShaderClipboard::clear() {
	_source.clear();
	_target.clear();
}

Texturable ShaderClipboard::getTexturable(SelectionTest& test) {
	// Initialise an empty Texturable structure
	Texturable returnValue;
	
	GlobalSceneGraph().traverse(
		algorithm::ClosestTexturableFinder(test, returnValue)
	);
	
	return returnValue;
}

void ShaderClipboard::setSource(SelectionTest& test) {
	_source = getTexturable(test);
}

} // namespace selection

// global accessor function
selection::ShaderClipboard& GlobalShaderClipboard() {
	static selection::ShaderClipboard _instance;
	
	return _instance;
}
