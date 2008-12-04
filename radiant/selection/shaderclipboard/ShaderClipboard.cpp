#include "ShaderClipboard.h"

#include "selectable.h"
#include "iscenegraph.h"
#include "mainframe.h"
#include "ui/mediabrowser/MediaBrowser.h"
#include "ui/texturebrowser/TextureBrowser.h"

namespace selection {

ShaderClipboard::ShaderClipboard() :
	_updatesDisabled(false)
{}

void ShaderClipboard::clear() {
	_source.clear();
	
	_updatesDisabled = true;
	
	// Update the status bar information
	updateStatusText();
	
	_updatesDisabled = false;
}

Texturable ShaderClipboard::getTexturable(SelectionTest& test) {
	// Initialise an empty Texturable structure
	Texturable returnValue;
	
	algorithm::ClosestTexturableFinder finder(test, returnValue);
	GlobalSceneGraph().root()->traverse(finder);
	
	return returnValue;
}

void ShaderClipboard::updateMediaBrowsers() {
	// Avoid nasty loopbacks
	_updatesDisabled = true;
	
	// Set the active shader in the Texture window as well
	GlobalTextureBrowser().setSelectedShader(_source.getShader());
	std::string sourceShader = _source.getShader();
	ui::MediaBrowser::getInstance().setSelection(sourceShader);
	
	_updatesDisabled = false;
	
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
	if (_updatesDisabled) return; // loopback guard
	
	_source = getTexturable(test);
	
	updateMediaBrowsers();
}

void ShaderClipboard::setSource(std::string shader) {
	if (_updatesDisabled) return; // loopback guard
	
	_source.clear();
	_source.shader = shader;
	
	// Don't update the media browser without loopback guards 
	// if this is desired, one will have to implement them
	updateStatusText(); 
}

void ShaderClipboard::setSource(Patch& sourcePatch) {
	if (_updatesDisabled) return; // loopback guard
	
	_source.clear();
	_source.patch = &sourcePatch;
	
	updateMediaBrowsers();
}

void ShaderClipboard::setSource(Face& sourceFace) {
	if (_updatesDisabled) return; // loopback guard
	
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
