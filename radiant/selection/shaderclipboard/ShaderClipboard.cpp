#include "ShaderClipboard.h"

#include "selectable.h"
#include "iscenegraph.h"
#include "texwindow.h"

namespace selection {

ShaderClipboard::ShaderClipboard()
{}

void ShaderClipboard::clear() {
	_source.clear();
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
	
	// Set the active shader in the Texture window as well
	TextureBrowser_SetSelectedShader(GlobalTextureBrowser(), _source.getShader());
}

void ShaderClipboard::setSource(Patch& sourcePatch) {
	_source.clear();
	_source.patch = &sourcePatch;
	
	// Set the active shader in the Texture window as well
	TextureBrowser_SetSelectedShader(GlobalTextureBrowser(), _source.getShader());
}

void ShaderClipboard::setSource(Face& sourceFace) {
	_source.clear();
	_source.face = &sourceFace;
	
	// Set the active shader in the Texture window as well
	TextureBrowser_SetSelectedShader(GlobalTextureBrowser(), _source.getShader());
}

Texturable& ShaderClipboard::getSource() {
	return _source;
}

} // namespace selection

// global accessor function
selection::ShaderClipboard& GlobalShaderClipboard() {
	static selection::ShaderClipboard _instance;
	
	return _instance;
}
