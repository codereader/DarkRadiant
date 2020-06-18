#pragma once

#include "imap.h"
#include "ishaderclipboard.h"
#include "Texturable.h"
#include <sigc++/signal.h>
#include <sigc++/trackable.h>
#include <sigc++/connection.h>

namespace selection 
{

class ShaderClipboard :
	public IShaderClipboard
{
private:
	// The source and target Texturables
	Texturable _source;

	bool _updatesDisabled;

    sigc::signal<void> _signalSourceChanged;

	sigc::connection _postUndoConn;
	sigc::connection _postRedoConn;
	sigc::connection _mapEventConn;

public:
	ShaderClipboard();

	SourceType getSourceType() const override;

	/** greebo: Sets the source patch to the given <sourcePatch>
	 */
	void setSource(Patch& sourcePatch);

	/** greebo: Sets the source face to the given <sourceFace>
	 */
	void setSource(Face& sourceFace);

	/** greebo: Sets the source face to the given <shader>
	 */
	void setSourceShader(const std::string& shader) override;

	/** greebo: Retrieves the current source Texturable
	 */
	Texturable& getSource();

    /**
     * Is emitted when the shader source changes.
     */
    sigc::signal<void>& signal_sourceChanged() override;

	// IShaderClipboard implementation

	void clear() override;
	std::string getShaderName() override;
	void pickFromSelectionTest(SelectionTest& test) override;
	void pasteShader(SelectionTest& test, PasteMode mode, bool pasteToAllFaces) override;
	void pasteTextureCoords(SelectionTest& test) override;
	void pasteMaterialName(SelectionTest& test) override;

	// Module-internal accessor
	static ShaderClipboard& Instance();

	// RegisterableModule
	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const ApplicationContext& ctx) override;
	void shutdownModule() override;

private:
	// Fires the signal, disabling updates to avoid loops
	void sourceChanged();

	// UndoSystem callbacks
	void onUndoRedoOperation();

	void onMapEvent(IMap::MapEvent ev);

	/** greebo: Retrieves the best texturable object from the
	 * 			given SelectionTest.
	 */
	Texturable getTexturable(SelectionTest& test);
};

} // namespace selection
