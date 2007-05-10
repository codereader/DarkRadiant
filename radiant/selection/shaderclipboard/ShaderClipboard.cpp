#include "ShaderClipboard.h"

#include "selectable.h"
#include "iscenegraph.h"
#include "mainframe.h"
#include "ui/mediabrowser/MediaBrowser.h"
#include "ui/texturebrowser/TextureBrowser.h"

namespace selection {

ShaderClipboard::ShaderClipboard()
{}

void ShaderClipboard::clear() {
	_source.clear();
	
	// Update the status bar information
	updateStatusText();
}

Texturable ShaderClipboard::getTexturable(SelectionTest& test) {
	// Initialise an empty Texturable structure
	Texturable returnValue;
	
	GlobalSceneGraph().traverse(
		algorithm::ClosestTexturableFinder(test, returnValue)
	);
	
	return returnValue;
}

void ShaderClipboard::updateMediaBrowsers() {
	// Set the active shader in the Texture window as well
	GlobalTextureBrowser().setSelectedShader(_source.getShader());
	std::string sourceShader = _source.getShader();
	ui::MediaBrowser::getInstance().setSelection(sourceShader);
	
	updateStatusText();
}

void ShaderClipboard::updateStatusText() {
	std::string statusText = "ShaderClipboard is empty.";
		
	if (!_source.empty()) {
		statusText = "ShaderClipboard: " + _source.getShader();
	
		if (_source.isFace()) {
			statusText += " (Face)";
		}
		else if (_source.isPatch()) {
			statusText += " (Patch)";
		}
		else if (_source.isShader()) {
			statusText += " (Shader)";
		}
	}
	
	if (g_pParentWnd != NULL) {
		g_pParentWnd->SetStatusText(g_pParentWnd->m_texture_status, statusText);
	}
}

void ShaderClipboard::setSource(SelectionTest& test) {
	_source = getTexturable(test);
	
	updateMediaBrowsers();
}

void ShaderClipboard::setSource(std::string shader) {
	_source.clear();
	_source.shader = shader;
	// Don't update the media browser without loopback guards 
	// if this is desired, one will have to implement them 
}

void ShaderClipboard::setSource(Patch& sourcePatch) {
	_source.clear();
	_source.patch = &sourcePatch;
	
	updateMediaBrowsers();
}

void ShaderClipboard::setSource(Face& sourceFace) {
	_source.clear();
	_source.face = &sourceFace;
	
	updateMediaBrowsers();
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
