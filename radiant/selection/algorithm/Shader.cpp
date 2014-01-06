#include "Shader.h"

#include "i18n.h"
#include "imainframe.h"
#include "iselection.h"
#include "iscenegraph.h"
#include "itextstream.h"
#include "iselectiontest.h"
#include "igroupnode.h"
#include "selectionlib.h"
#include "registry/registry.h"
#include "gtkutil/dialog/MessageBox.h"
#include "string/string.h"
#include "brush/FaceInstance.h"
#include "brush/BrushVisit.h"
#include "brush/TextureProjection.h"
#include "patch/PatchSceneWalk.h"
#include "patch/PatchNode.h"
#include "selection/algorithm/Primitives.h"
#include "selection/shaderclipboard/ShaderClipboard.h"
#include "ui/surfaceinspector/SurfaceInspector.h"
#include "selection/shaderclipboard/ClosestTexturableFinder.h"

#include <boost/algorithm/string/case_conv.hpp>

namespace selection
{

namespace algorithm
{

class AmbiguousShaderException:
	public std::runtime_error
{
public:
	// Constructor
	AmbiguousShaderException(const std::string& what):
		std::runtime_error(what)
	{}
};

/** greebo: Cycles through all the Patches/Faces and throws as soon as
 * at least two different non-empty shader names are found.
 *
 * @throws: AmbiguousShaderException
 */
class UniqueShaderFinder
{
	// The string containing the result
	std::string& _shader;

public:
	UniqueShaderFinder(std::string& shader) :
		_shader(shader)
	{}

	void operator()(Face& face) const
	{
		compareShader(face.getShader());
	}

	void operator()(FaceInstance& faceInstance) const
	{
		compareShader(faceInstance.getFace().getShader());
	}

	void operator()(Patch& patch) const
	{
		compareShader(patch.getShader());
	}

private:
	void compareShader(const std::string& foundShader) const
	{
		if (foundShader != "$NONE" && _shader != "$NONE" &&
			_shader != foundShader)
		{
			throw AmbiguousShaderException(foundShader);
		}

		_shader = foundShader;
	}
};

std::string getShaderFromSelection() 
{
	std::string returnValue("");

	const SelectionInfo& selectionInfo = GlobalSelectionSystem().getSelectionInfo();

	if (selectionInfo.totalCount > 0)
	{
		std::string faceShader("$NONE");
		std::string patchShader("$NONE");

		// PATCHES
		if (selectionInfo.patchCount > 0)
		{
			// Try to get the unique shader from the selected patches
			try 
			{
				// Go through all the selected patches
				GlobalSelectionSystem().foreachPatch(UniqueShaderFinder(patchShader));
			}
			catch (AmbiguousShaderException&) {
				patchShader = "";
			}
		}

		// BRUSHES
		// If there are no FaceInstances selected, cycle through the brushes
		if (FaceInstance::Selection().empty())
		{
			// Try to get the unique shader from the selected brushes
			try
			{
				// Go through all the selected brushes and their faces
				GlobalSelectionSystem().foreachFace(UniqueShaderFinder(faceShader));
			}
			catch (AmbiguousShaderException&)
			{
				faceShader = "";
			}
		}
		else
		{
			// Try to get the unique shader from the faces
			try
			{
				forEachSelectedFaceComponent(UniqueShaderFinder(faceShader));
			}
			catch (AmbiguousShaderException&) {
				faceShader = "";
			}
		}

		// Both faceShader and patchShader found?
		if (faceShader != "$NONE" && patchShader != "$NONE") {
			// Compare them and return one of them, if they are equal
			returnValue = (faceShader == patchShader) ? patchShader : "";
		}
		else if (faceShader != "$NONE") {
			// Only a faceShader has been found
			returnValue = faceShader;
		}
		else if (patchShader != "$NONE") {
			// Only a patchShader has been found
			returnValue = patchShader;
		}
	}

	return returnValue;
}

void applyShaderToSelection(const std::string& shaderName)
{
	UndoableCommand undo("setShader");

	GlobalSelectionSystem().foreachFace([&] (Face& face) { face.setShader(shaderName); });
	GlobalSelectionSystem().foreachPatch([&] (Patch& patch) { patch.setShader(shaderName); });

	SceneChangeNotify();
	// Update the Texture Tools
	ui::SurfaceInspector::update();
}

/** greebo: Applies the shader from the clipboard's face to the given <target> face
 */
void applyClipboardFaceToFace(Face& target) {
	// Get a reference to the source Texturable in the clipboard
	Texturable& source = GlobalShaderClipboard().getSource();

	target.applyShaderFromFace(*(source.face));
}

/** greebo: Applies the shader from the clipboard's patch to the given <target> face
 */
void applyClipboardPatchToFace(Face& target)
{
	// Get a reference to the source Texturable in the clipboard
	Texturable& source = GlobalShaderClipboard().getSource();

	// Retrieve the textureprojection from the source face
	TextureProjection projection;

	// Copy just the shader name, the rest is default value
	target.setShader(source.patch->getShader());
	target.SetTexdef(projection);
	target.SetFlags(ContentsFlagsValue(0, 0, 0, false));
}

void applyClipboardToTexturable(Texturable& target, bool projected, bool entireBrush) {
	// Get a reference to the source Texturable in the clipboard
	Texturable& source = GlobalShaderClipboard().getSource();

	// Check the basic conditions
	if (!target.empty() && !source.empty()) {
		// Do we have a Face to copy from?
		if (source.isFace()) {
			if (target.isFace() && entireBrush) {
				// Copy Face >> Whole Brush
				for (Brush::const_iterator i = target.brush->begin();
					 i != target.brush->end();
					 i++)
				{
					applyClipboardFaceToFace(*(*i));
				}
			}
			else if (target.isFace() && !entireBrush) {
				// Copy Face >> Face
				applyClipboardFaceToFace(*target.face);
			}
			else if (target.isPatch() && !entireBrush) {
				// Copy Face >> Patch

				// Set the shader name first
				target.patch->setShader(source.face->getShader());

				// Either paste the texture projected or naturally
				if (projected) {
					target.patch->pasteTextureProjected(source.face);
				}
				else {
					target.patch->pasteTextureNatural(source.face);
				}
			}
		}
		else if (source.isPatch()) {
			// Source holds a patch
			if (target.isFace() && entireBrush) {
				// Copy patch >> whole brush
				for (Brush::const_iterator i = target.brush->begin();
					 i != target.brush->end();
					 i++)
				{
					applyClipboardPatchToFace(*(*i));
				}
			}
			else if (target.isFace() && !entireBrush) {
				// Copy patch >> face
				applyClipboardPatchToFace(*target.face);
			}
			else if (target.isPatch() && !entireBrush) {
				// Copy patch >> patch
				target.patch->setShader(source.patch->getShader());
				target.patch->pasteTextureNatural(*source.patch);
			}
		}
		else if (source.isShader()) {
			if (target.isFace() && entireBrush) {
				// Copy patch >> whole brush
				for (Brush::const_iterator i = target.brush->begin();
					 i != target.brush->end();
					 i++)
				{
					(*i)->setShader(source.getShader());
				}
			}
			else if (target.isFace() && !entireBrush) {
				target.face->setShader(source.getShader());
			}
			else if (target.isPatch() && !entireBrush) {
				target.patch->setShader(source.getShader());
			}
		}
	}
}

void pasteShader(SelectionTest& test, bool projected, bool entireBrush) {
	// Construct the command string
	std::string command("pasteShader");
	command += (projected ? "Projected" : "Natural");
	command += (entireBrush ? "ToBrush" : "");

	UndoableCommand undo(command);

	// Initialise an empty Texturable structure
	Texturable target;

	// Find a suitable target Texturable
	ClosestTexturableFinder finder(test, target);
	GlobalSceneGraph().root()->traverseChildren(finder);

	if (target.isPatch() && entireBrush) {
		gtkutil::MessageBox::ShowError(
			_("Can't paste shader to entire brush.\nTarget is not a brush."),
			GlobalMainFrame().getTopLevelWindow());
	}
	else {
		// Pass the call to the algorithm function taking care of all the IFs
		applyClipboardToTexturable(target, projected, entireBrush);
	}

	SceneChangeNotify();
	// Update the Texture Tools
	ui::SurfaceInspector::update();
}

void pasteTextureCoords(SelectionTest& test) {
	UndoableCommand undo("pasteTextureCoordinates");

	// Initialise an empty Texturable structure
	Texturable target;

	// Find a suitable target Texturable
	ClosestTexturableFinder finder(test, target);
	GlobalSceneGraph().root()->traverseChildren(finder);

	// Get a reference to the source Texturable in the clipboard
	Texturable& source = GlobalShaderClipboard().getSource();

	// Check the basic conditions
	if (target.isPatch() && source.isPatch()) {
		// Check if the dimensions match, emit an error otherwise
		if (target.patch->getWidth() == source.patch->getWidth() &&
			target.patch->getHeight() == source.patch->getHeight())
		{
			target.patch->pasteTextureCoordinates(source.patch);
		}
		else {
			gtkutil::MessageBox::ShowError(
				_("Can't paste Texture Coordinates.\nTarget patch dimensions must match."),
				GlobalMainFrame().getTopLevelWindow());
		}
	}
	else {
		if (source.isPatch()) {
			// Nothing to do, this works for patches only
			gtkutil::MessageBox::ShowError(
				_("Can't paste Texture Coordinates from patches to faces."),
				GlobalMainFrame().getTopLevelWindow());
		}
		else {
			// Nothing to do, this works for patches only
			gtkutil::MessageBox::ShowError(
				_("Can't paste Texture Coordinates from faces."),
				GlobalMainFrame().getTopLevelWindow());
		}
	}

	SceneChangeNotify();
	// Update the Texture Tools
	ui::SurfaceInspector::update();
}

void pickShaderFromSelection(const cmd::ArgumentList& args) {
	GlobalShaderClipboard().clear();

	const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();

	// Check for a single patch
	if (info.totalCount == 1 && info.patchCount == 1) {
		try {
			Patch& sourcePatch = getLastSelectedPatch();
			GlobalShaderClipboard().setSource(sourcePatch);
		}
		catch (InvalidSelectionException e) {
			gtkutil::MessageBox::ShowError(
				_("Can't copy Shader. Couldn't retrieve patch."),
				GlobalMainFrame().getTopLevelWindow());
		}
	}
	else if (selectedFaceCount() == 1) {
		try {
			Face& sourceFace = getLastSelectedFace();
			GlobalShaderClipboard().setSource(sourceFace);
		}
		catch (InvalidSelectionException e) {
			gtkutil::MessageBox::ShowError(
				_("Can't copy Shader. Couldn't retrieve face."),
				GlobalMainFrame().getTopLevelWindow());
		}
	}
	else {
		// Nothing to do, this works for patches only
		gtkutil::MessageBox::ShowError(
			_("Can't copy Shader. Please select a single face or patch."),
			 GlobalMainFrame().getTopLevelWindow());
	}
}

/** greebo: This applies the clipboard to the visited faces/patches.
 */
class ClipboardShaderApplicator
{
	bool _natural;
public:
	ClipboardShaderApplicator(bool natural = false) :
		_natural(natural)
	{}

	void operator()(Patch& patch)
	{
		Texturable target;
		target.patch = &patch;
		target.node = patch.getPatchNode().shared_from_this();

		// Apply the shader (projected, not to the entire brush)
		applyClipboardToTexturable(target, !_natural, false);
	}

	void operator()(Face& face)
	{
		Texturable target;
		target.face = &face;
		target.node = face.getBrush().getBrushNode().shared_from_this();

		// Apply the shader (projected, not to the entire brush)
		applyClipboardToTexturable(target, !_natural, false);
	}
};

void pasteShaderToSelection(const cmd::ArgumentList& args)
{
	if (GlobalShaderClipboard().getSource().empty())
	{
		return;
	}

	// Start a new command
	UndoableCommand command("pasteShaderToSelection");

	ClipboardShaderApplicator applicator;
	GlobalSelectionSystem().foreachFace(applicator);
	GlobalSelectionSystem().foreachPatch(applicator);

	SceneChangeNotify();
	// Update the Texture Tools
	ui::SurfaceInspector::update();
}

void pasteShaderNaturalToSelection(const cmd::ArgumentList& args)
{
	if (GlobalShaderClipboard().getSource().empty())
	{
		return;
	}

	// Start a new command
	UndoableCommand command("pasteShaderNaturalToSelection");

	// greebo: Construct a visitor and traverse the selection
	ClipboardShaderApplicator applicator(true); // true == paste natural
	GlobalSelectionSystem().foreachFace(applicator);
	GlobalSelectionSystem().foreachPatch(applicator);

	SceneChangeNotify();
	// Update the Texture Tools
	ui::SurfaceInspector::update();
}

TextureProjection getSelectedTextureProjection()
{
	TextureProjection returnValue;

	if (selectedFaceCount() == 1)
	{
		// Get the last selected face instance from the global
		FaceInstance& faceInstance = *FaceInstance::Selection().back();
		faceInstance.getFace().GetTexdef(returnValue);
	}

	return returnValue;
}

Vector2 getSelectedFaceShaderSize()
{
	Vector2 returnValue(0,0);

	if (selectedFaceCount() == 1)
	{
		// Get the last selected face instance from the global
		FaceInstance& faceInstance = *FaceInstance::Selection().back();

		returnValue[0] = faceInstance.getFace().getFaceShader().width();
		returnValue[1] = faceInstance.getFace().getFaceShader().height();
	}

	return returnValue;
}

void fitTexture(const float repeatS, const float repeatT)
{
	UndoableCommand command("fitTexture");

	GlobalSelectionSystem().foreachFace([&] (Face& face) { face.fitTexture(repeatS, repeatT); });
	GlobalSelectionSystem().foreachPatch([&] (Patch& patch) { patch.SetTextureRepeat(repeatS, repeatT); });

	SceneChangeNotify();
	// Update the Texture Tools
	ui::SurfaceInspector::update();
}

void flipTexture(unsigned int flipAxis)
{
	UndoableCommand undo("flipTexture");

	GlobalSelectionSystem().foreachFace([&] (Face& face) { face.flipTexture(flipAxis); });
	GlobalSelectionSystem().foreachPatch([&] (Patch& patch) { patch.FlipTexture(flipAxis); });

	SceneChangeNotify();
}

void flipTextureS(const cmd::ArgumentList& args)
{
	flipTexture(0);
}

void flipTextureT(const cmd::ArgumentList& args)
{
	flipTexture(1);
}

void naturalTexture(const cmd::ArgumentList& args)
{
	UndoableCommand undo("naturalTexture");

	// Patches
	GlobalSelectionSystem().foreachPatch(
        [] (Patch& patch) { patch.NaturalTexture(); }
    );
	GlobalSelectionSystem().foreachFace(
        [] (Face& face) { face.SetTexdef(TextureProjection()); }
    );

	SceneChangeNotify();

	// Update the Texture Tools
	ui::SurfaceInspector::update();
}

void applyTextureProjectionToFaces(TextureProjection& projection)
{
	UndoableCommand undo("textureProjectionSetSelected");

	GlobalSelectionSystem().foreachFace([&] (Face& face) { face.SetTexdef(projection); });

	SceneChangeNotify();
	// Update the Texture Tools
	ui::SurfaceInspector::update();
}

void shiftTexture(const Vector2& shift) 
{
	std::string command("shiftTexture: ");
	command += "s=" + string::to_string(shift[0]) + ", t=" + string::to_string(shift[1]);

	UndoableCommand undo(command);

	GlobalSelectionSystem().foreachFace([&] (Face& face) { face.shiftTexdef(shift[0], shift[1]); });
	GlobalSelectionSystem().foreachPatch([&] (Patch& patch) { patch.TranslateTexture(shift[0], shift[1]); });

	SceneChangeNotify();
	// Update the Texture Tools
	ui::SurfaceInspector::update();
}

void scaleTexture(const Vector2& scale)
{
	std::string command("scaleTexture: ");
	command += "sScale=" + string::to_string(scale[0]) + ", tScale=" + string::to_string(scale[1]);

	UndoableCommand undo(command);

	// Prepare the according patch scale value (they are relatively scaled)
	Vector2 patchScale;

	// We need to have 1.05 for a +0.05 scale
	// and a 1/1.05 for a -0.05 scale
	for (int i = 0; i < 2; i++)
	{
		if (scale[i] >= 0.0f)
		{
			patchScale[i] = 1.0f + scale[i];
		}
		else
		{
			patchScale[i] = 1/(1.0f + fabs(scale[i]));
		}
	}

	GlobalSelectionSystem().foreachFace([&] (Face& face) { face.scaleTexdef(scale[0], scale[1]); });
	GlobalSelectionSystem().foreachPatch([&] (Patch& patch) { patch.ScaleTexture(patchScale[0], patchScale[1]); });

	SceneChangeNotify();
	// Update the Texture Tools
	ui::SurfaceInspector::update();
}

void rotateTexture(const float angle)
{
	std::string command("rotateTexture: ");
	command += "angle=" + string::to_string(angle);

	UndoableCommand undo(command);

	GlobalSelectionSystem().foreachFace([&] (Face& face) { face.rotateTexdef(angle); });
	GlobalSelectionSystem().foreachPatch([&] (Patch& patch) { patch.RotateTexture(angle); });

	SceneChangeNotify();
	// Update the Texture Tools
	ui::SurfaceInspector::update();
}

void shiftTextureLeft() {
	shiftTexture(Vector2(-registry::getValue<float>("user/ui/textures/surfaceInspector/hShiftStep"), 0.0f));
}

void shiftTextureRight() {
	shiftTexture(Vector2(registry::getValue<float>("user/ui/textures/surfaceInspector/hShiftStep"), 0.0f));
}

void shiftTextureUp() {
	shiftTexture(Vector2(0.0f, registry::getValue<float>("user/ui/textures/surfaceInspector/vShiftStep")));
}

void shiftTextureDown() {
	shiftTexture(Vector2(0.0f, -registry::getValue<float>("user/ui/textures/surfaceInspector/vShiftStep")));
}

void scaleTextureLeft() {
	scaleTexture(Vector2(-registry::getValue<float>("user/ui/textures/surfaceInspector/hScaleStep"), 0.0f));
}

void scaleTextureRight() {
	scaleTexture(Vector2(registry::getValue<float>("user/ui/textures/surfaceInspector/hScaleStep"), 0.0f));
}

void scaleTextureUp() {
	scaleTexture(Vector2(0.0f, registry::getValue<float>("user/ui/textures/surfaceInspector/vScaleStep")));
}

void scaleTextureDown() {
	scaleTexture(Vector2(0.0f, -registry::getValue<float>("user/ui/textures/surfaceInspector/vScaleStep")));
}

void rotateTextureClock() {
	rotateTexture(fabs(registry::getValue<float>("user/ui/textures/surfaceInspector/rotStep")));
}

void rotateTextureCounter() {
	rotateTexture(-fabs(registry::getValue<float>("user/ui/textures/surfaceInspector/rotStep")));
}

void rotateTexture(const cmd::ArgumentList& args) {
	if (args.size() != 1) {
		rMessage() << "Usage: TexRotate [+1|-1]" << std::endl;
		return;
	}

	if (args[0].getInt() > 0) {
		// Clockwise
		rotateTextureClock();
	}
	else {
		// Counter-Clockwise
		rotateTextureCounter();
	}
}

void scaleTexture(const cmd::ArgumentList& args) {
	if (args.size() != 1) {
		rMessage() << "Usage: TexScale 's t'" << std::endl;
		rMessage() << "       TexScale [up|down|left|right]" << std::endl;
		rMessage() << "Example: TexScale '0.05 0' performs"
			<< " a 105% scale in the s direction." << std::endl;
		rMessage() << "Example: TexScale up performs"
			<< " a vertical scale using the step value defined in the Surface Inspector."
			<< std::endl;
		return;
	}

	std::string arg = boost::algorithm::to_lower_copy(args[0].getString());

	if (arg == "up") {
		scaleTextureUp();
	}
	else if (arg == "down") {
		scaleTextureDown();
	}
	if (arg == "left") {
		scaleTextureLeft();
	}
	if (arg == "right") {
		scaleTextureRight();
	}
	else {
		// No special argument, retrieve the Vector2 argument and pass the call
		scaleTexture(args[0].getVector2());
	}
}

void shiftTextureCmd(const cmd::ArgumentList& args) {
	if (args.size() != 1) {
		rMessage() << "Usage: TexShift 's t'" << std::endl
			 << "       TexShift [up|down|left|right]" << std::endl
			 << "[up|down|left|right| takes the step values "
			 << "from the Surface Inspector." << std::endl;
		return;
	}

	std::string arg = boost::algorithm::to_lower_copy(args[0].getString());

	if (arg == "up") {
		shiftTextureUp();
	}
	else if (arg == "down") {
		shiftTextureDown();
	}
	if (arg == "left") {
		shiftTextureLeft();
	}
	if (arg == "right") {
		shiftTextureRight();
	}
	else {
		// No special argument, retrieve the Vector2 argument and pass the call
		shiftTexture(args[0].getVector2());
	}
}

void alignTexture(EAlignType align)
{
	std::string command("alignTexture: ");
	command += "edge=";

	switch (align)
	{
	case ALIGN_TOP:
		command += "top";
		break;
	case ALIGN_BOTTOM:
		command += "bottom";
		break;
	case ALIGN_LEFT:
		command += "left";
		break;
	case ALIGN_RIGHT:
		command += "right";
		break;
	};

	UndoableCommand undo(command);

	GlobalSelectionSystem().foreachFace([&] (Face& face) { face.alignTexture(align); });
	GlobalSelectionSystem().foreachPatch([&] (Patch& patch) { patch.alignTexture(align); });

	SceneChangeNotify();
	// Update the Texture Tools
	ui::SurfaceInspector::update();
}

void alignTextureCmd(const cmd::ArgumentList& args)
{
	if (args.size() != 1)
	{
		rMessage() << "Usage: TexAlign [top|bottom|left|right]" << std::endl;
		return;
	}

	std::string arg = boost::algorithm::to_lower_copy(args[0].getString());

	if (arg == "top")
	{
		alignTexture(ALIGN_TOP);
	}
	else if (arg == "bottom")
	{
		alignTexture(ALIGN_BOTTOM);
	}
	if (arg == "left")
	{
		alignTexture(ALIGN_LEFT);
	}
	if (arg == "right")
	{
		alignTexture(ALIGN_RIGHT);
	}
	else
	{
		rMessage() << "Usage: TexAlign [top|bottom|left|right]" << std::endl;
	}
}

void normaliseTexture(const cmd::ArgumentList& args)
{
	UndoableCommand undo("normaliseTexture");

	GlobalSelectionSystem().foreachFace([] (Face& face) { face.normaliseTexture(); });
	GlobalSelectionSystem().foreachPatch([] (Patch& patch) { patch.normaliseTexture(); });

	SceneChangeNotify();
	// Update the Texture Tools
	ui::SurfaceInspector::update();
}

/** greebo: This replaces the shader of the visited face/patch with <replace>
 * 			if the face is textured with <find> and increases the given <counter>.
 */
class ShaderReplacer
{
	const std::string _find;
	const std::string _replace;
	mutable int _counter;
public:
	ShaderReplacer(const std::string& find, const std::string& replace) :
		_find(find),
		_replace(replace),
		_counter(0)
	{}

	int getReplacedCount() const {
		return _counter;
	}

	void operator()(Face& face) const {
		if (face.getShader() == _find) {
			face.setShader(_replace);
			_counter++;
		}
	}

	// BrushInstanceVisitor implementation
	void operator()(FaceInstance& face) const
	{
		if (face.getFace().getShader() == _find) {
			face.getFace().setShader(_replace);
			_counter++;
		}
	}

	void operator()(Patch& patch) const {
		if (patch.getShader() == _find) {
			patch.setShader(_replace);
			_counter++;
		}
	}
};

int findAndReplaceShader(const std::string& find,
	const std::string& replace, bool selectedOnly)
{
	std::string command("textureFindReplace");
	command += "-find " + find + " -replace " + replace;
	UndoableCommand undo(command);

	// Construct a visitor class
	ShaderReplacer replacer(find, replace);

	if (selectedOnly)
	{
		if (GlobalSelectionSystem().Mode() != SelectionSystem::eComponent)
		{
			// Find & replace all the brush and patch shaders
			GlobalSelectionSystem().foreachFace(replacer);
			GlobalSelectionSystem().foreachPatch(replacer);
		}
		
		// Search the single selected faces in any case
		forEachSelectedFaceComponent(replacer);
	}
	else
	{
		scene::foreachVisibleFaceInstance(replacer);
		scene::foreachVisiblePatch(replacer);
	}

	return replacer.getReplacedCount();
}

class ByShaderSelector :
	public scene::NodeVisitor
{
private:
	std::string _shaderName;

	bool _select;

public:
	ByShaderSelector(const std::string& shaderName, bool select = true) :
		_shaderName(shaderName),
		_select(select)
	{}

	bool pre(const scene::INodePtr& node)
	{
		Brush* brush = Node_getBrush(node);

		if (brush != NULL)
		{
			if (brush->hasShader(_shaderName))
			{
				Node_setSelected(node, _select);
			}

			return false; // don't traverse primitives
		}

		Patch* patch = Node_getPatch(node);

		if (patch != NULL)
		{
			if (patch->getShader() == _shaderName)
			{
				Node_setSelected(node, _select);
			}

			return false; // don't traverse primitives
		}

		return true;
	}
};

void selectItemsByShader(const std::string& shaderName)
{
	ByShaderSelector selector(shaderName, true);
	GlobalSceneGraph().root()->traverseChildren(selector);
}

void deselectItemsByShader(const std::string& shaderName)
{
	ByShaderSelector selector(shaderName, false);
	GlobalSceneGraph().root()->traverseChildren(selector);
}

void selectItemsByShader(const cmd::ArgumentList& args)
{
	if (args.size() < 1)
	{
		rMessage() << "Usage: selectItemsByShader <SHADERNAME>" << std::endl;
		return;
	}

	selectItemsByShader(args[0].getString());
}

void deselectItemsByShader(const cmd::ArgumentList& args)
{
	if (args.size() < 1)
	{
		rMessage() << "Usage: selectItemsByShader <SHADERNAME>" << std::endl;
		return;
	}

	deselectItemsByShader(args[0].getString());
}

} // namespace algorithm

} // namespace selection
