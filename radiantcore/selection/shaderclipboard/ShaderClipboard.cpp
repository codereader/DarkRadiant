#include "ShaderClipboard.h"

#include "i18n.h"
#include "imap.h"
#include "iselectiontest.h"
#include "iscenegraph.h"
#include "ClosestTexturableFinder.h"

#include "util/ScopedBoolLock.h"
#include "patch/PatchNode.h"
#include "brush/BrushNode.h"

namespace selection 
{

namespace
{
	const char* const LAST_USED_MATERIAL_KEY = "LastShaderClipboardMaterial";
}

ShaderClipboard::ShaderClipboard() :
	_updatesDisabled(false)
{}

ShaderClipboard::SourceType ShaderClipboard::getSourceType() const
{
	if (_source.empty())
	{
		return SourceType::Empty;
	}

	if (_source.isFace())
	{
		return SourceType::Face;
	}
	else if (_source.isPatch())
	{
		return SourceType::Patch;
	}
	else if (_source.isShader())
	{
		return SourceType::Shader;
	}

	return SourceType::Empty;
}

void ShaderClipboard::sourceChanged()
{
	util::ScopedBoolLock lock(_updatesDisabled); // prevent loopbacks

	_signalSourceChanged.emit();
}

void ShaderClipboard::clear() 
{
	if (_updatesDisabled) return;

	_source.clear();

	sourceChanged();
}

void ShaderClipboard::onUndoRedoOperation()
{
	// Check if the source is still valid
	if (!_source.checkValid())
	{
		clear();
	}
}

Texturable ShaderClipboard::getTexturable(SelectionTest& test)
{
	// Initialise an empty Texturable structure
	Texturable returnValue;

	algorithm::ClosestTexturableFinder finder(test, returnValue);
	GlobalSceneGraph().root()->traverseChildren(finder);

	return returnValue;
}

std::string ShaderClipboard::getShaderName()
{
	return getSource().getShader();
}

void ShaderClipboard::pickFromSelectionTest(SelectionTest& test)
{
	if (_updatesDisabled) return; // loopback guard

	_source = getTexturable(test);

	sourceChanged();
}

void ShaderClipboard::pasteShader(SelectionTest& test, PasteMode mode, bool pasteToAllFaces)
{
	selection::algorithm::pasteShader(test, mode == PasteMode::Projected, pasteToAllFaces);
}

void ShaderClipboard::pasteTextureCoords(SelectionTest& test)
{
	selection::algorithm::pasteTextureCoords(test);
}

void ShaderClipboard::pasteMaterialName(SelectionTest& test)
{
	selection::algorithm::pasteShaderName(test);
}

ShaderClipboard& ShaderClipboard::Instance()
{
	return static_cast<ShaderClipboard&>(GlobalShaderClipboard());
}

void ShaderClipboard::setSourceShader(const std::string& shader)
{
	if (_updatesDisabled) return; // loopback guard

	_source.clear();
	_source.shader = shader;

	sourceChanged();
}

void ShaderClipboard::setSource(Patch& sourcePatch)
{
	if (_updatesDisabled) return; // loopback guard

	_source.clear();
	_source.patch = &sourcePatch;
	_source.node = sourcePatch.getPatchNode().shared_from_this();

	sourceChanged();
}

void ShaderClipboard::setSource(Face& sourceFace) 
{
	if (_updatesDisabled) return; // loopback guard

	_source.clear();
	_source.face = &sourceFace;
	_source.node = sourceFace.getBrush().getBrushNode().shared_from_this();

	sourceChanged();
}

Texturable& ShaderClipboard::getSource()
{
	return _source;
}

sigc::signal<void>& ShaderClipboard::signal_sourceChanged()
{
    return _signalSourceChanged;
}

void ShaderClipboard::onMapEvent(IMap::MapEvent ev)
{
	switch (ev)
	{
	case IMap::MapUnloading:
		// Clear the shaderclipboard, the references are most probably invalid now
		clear();
		break;

	case IMap::MapSaving:
		// Write the current value to the map properties on save
		if (!_source.empty() && GlobalMapModule().getRoot())
		{
			GlobalMapModule().getRoot()->setProperty(LAST_USED_MATERIAL_KEY, _source.getShader());
		}
		break;

	case IMap::MapLoaded:
		// Try to load the last used material name from the properties
		if (GlobalMapModule().getRoot())
		{
			auto shader = GlobalMapModule().getRoot()->getProperty(LAST_USED_MATERIAL_KEY);
			
			if (!shader.empty())
			{
				setSourceShader(shader);
				break;
			}
		}
		clear();
		break;
	};
}

const std::string& ShaderClipboard::getName() const
{
	static std::string _name(MODULE_SHADERCLIPBOARD);
	return _name;
}

const StringSet& ShaderClipboard::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_UNDOSYSTEM);
		_dependencies.insert(MODULE_MAP);
	}

	return _dependencies;
}

void ShaderClipboard::initialiseModule(const ApplicationContext& ctx)
{
	rMessage() << getName() << "::initialiseModule called." << std::endl;

	_postUndoConn = GlobalUndoSystem().signal_postUndo().connect(
		sigc::mem_fun(this, &ShaderClipboard::onUndoRedoOperation));
	_postRedoConn = GlobalUndoSystem().signal_postRedo().connect(
		sigc::mem_fun(this, &ShaderClipboard::onUndoRedoOperation));

	_mapEventConn = GlobalMapModule().signal_mapEvent().connect(
		sigc::mem_fun(this, &ShaderClipboard::onMapEvent));
}

void ShaderClipboard::shutdownModule()
{
	_postUndoConn.disconnect();
	_postRedoConn.disconnect();
	_mapEventConn.disconnect();
}

} // namespace
