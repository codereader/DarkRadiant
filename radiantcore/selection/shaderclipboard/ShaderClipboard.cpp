#include "ShaderClipboard.h"

#include "i18n.h"
#include "imap.h"
#include "iselectiontest.h"
#include "iscenegraph.h"
#include "ishaders.h"
#include "iclipboard.h"
#include "string/trim.h"
#include "ClosestTexturableFinder.h"

#include "util/ScopedBoolLock.h"
#include "patch/PatchNode.h"
#include "brush/BrushNode.h"
#include "module/StaticModule.h"
#include "../clipboard/Clipboard.h"

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
	_source.node = sourceFace.getBrushInternal().getBrushNode().shared_from_this();

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

	default:
		return;
	};
}

const std::string& ShaderClipboard::getName() const
{
	static std::string _name(MODULE_SHADERCLIPBOARD);
	return _name;
}

const StringSet& ShaderClipboard::getDependencies() const
{
    static StringSet _dependencies{ MODULE_MAP };
	return _dependencies;
}

void ShaderClipboard::initialiseModule(const IApplicationContext& ctx)
{
	rMessage() << getName() << "::initialiseModule called." << std::endl;

	_postUndoConn = GlobalMapModule().signal_postUndo().connect(
		sigc::mem_fun(this, &ShaderClipboard::onUndoRedoOperation));
	_postRedoConn = GlobalMapModule().signal_postRedo().connect(
		sigc::mem_fun(this, &ShaderClipboard::onUndoRedoOperation));

	_mapEventConn = GlobalMapModule().signal_mapEvent().connect(
		sigc::mem_fun(this, &ShaderClipboard::onMapEvent));

	clear();

    module::GlobalModuleRegistry().signal_allModulesInitialised().connect(
        sigc::mem_fun(this, &ShaderClipboard::postModuleInitialisation)
    );
}

void ShaderClipboard::shutdownModule()
{
	_postUndoConn.disconnect();
	_postRedoConn.disconnect();
	_mapEventConn.disconnect();
    _clipboardContentsChangedConn.disconnect();
}

void ShaderClipboard::postModuleInitialisation()
{
    if (module::GlobalModuleRegistry().moduleExists(MODULE_CLIPBOARD))
    {
        // Subscribe to clipboard changes to check for copied material names
        _clipboardContentsChangedConn = GlobalClipboard().signal_clipboardContentChanged().connect(
            sigc::mem_fun(this, &ShaderClipboard::onSystemClipboardContentsChanged)
        );
    }
}

void ShaderClipboard::onSystemClipboardContentsChanged()
{
    if (_updatesDisabled) return;

    auto candidate = clipboard::getMaterialNameFromClipboard();

    if (!candidate.empty())
    {
        rMessage() << "Found a valid material name in the system clipboard: " << candidate << std::endl;
        setSourceShader(candidate);
    }
}

// Define the static module
module::StaticModuleRegistration<ShaderClipboard> shaderClipboardModule;

} // namespace
