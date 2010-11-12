#include "Shader.h"

#include "i18n.h"
#include "imainframe.h"
#include "iselection.h"
#include "iscenegraph.h"
#include "itextstream.h"
#include "iselectable.h"
#include "igroupnode.h"
#include "selectionlib.h"
#include "gtkutil/dialog.h"
#include "string/string.h"
#include "brush/FaceInstance.h"
#include "brush/BrushVisit.h"
#include "brush/TextureProjection.h"
#include "patch/PatchSceneWalk.h"
#include "patch/PatchNode.h"
#include "selection/algorithm/Primitives.h"
#include "selection/shaderclipboard/ShaderClipboard.h"
#include "ui/surfaceinspector/SurfaceInspector.h"

#include <boost/algorithm/string/case_conv.hpp>

// greebo: Nasty global that contains all the selected face instances
extern FaceInstanceSet g_SelectedFaceInstances;

namespace selection {
	namespace algorithm {

// Constants
namespace {
	const std::string RKEY_DEFAULT_TEXTURE_SCALE = "user/ui/textures/defaultTextureScale";
}

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
	mutable std::string& _shader;

public:
	UniqueShaderFinder(std::string& shader) :
		_shader(shader)
	{}

	void operator()(FaceInstance& face) const {

		const std::string& foundShader = face.getFace().getShader();

		if (foundShader != "$NONE" && _shader != "$NONE" &&
			_shader != foundShader)
		{
			throw AmbiguousShaderException(foundShader);
		}

		_shader = foundShader;
	}

	void operator()(Patch& patch) const {

		const std::string& foundShader = patch.getShader();

		if (foundShader != "$NONE" && _shader != "$NONE" &&
			_shader != foundShader)
		{
			throw AmbiguousShaderException(foundShader);
		}

		_shader = foundShader;
	}
};

std::string getShaderFromSelection() {
	std::string returnValue("");

	const SelectionInfo& selectionInfo = GlobalSelectionSystem().getSelectionInfo();

	if (selectionInfo.totalCount > 0) {
		std::string faceShader("$NONE");
		std::string patchShader("$NONE");

		// PATCHES
		if (selectionInfo.patchCount > 0) {
			// Try to get the unique shader from the selected patches
			try {
				// Go through all the selected patches
				Scene_forEachSelectedPatch(
					UniqueShaderFinder(patchShader)
				);
			}
			catch (AmbiguousShaderException&) {
				patchShader = "";
			}
		}

		// BRUSHES
		// If there are no FaceInstances selected, cycle through the brushes
		if (g_SelectedFaceInstances.empty()) {
			// Try to get the unique shader from the selected brushes
			try {
				// Go through all the selected brushes and their faces
				Scene_ForEachSelectedBrush_ForEachFaceInstance(
					GlobalSceneGraph(),
					UniqueShaderFinder(faceShader)
				);
			}
			catch (AmbiguousShaderException&) {
				faceShader = "";
			}
		}
		else {
			// Try to get the unique shader from the faces
			try {
				UniqueShaderFinder finder(faceShader);
				g_SelectedFaceInstances.foreach(finder);
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

/** greebo: Applies the given shader to the visited face/patch
 */
class ShaderApplicator :
	public PrimitiveVisitor
{
	std::string _shader;

public:
	ShaderApplicator(const std::string& shader) :
		_shader(shader)
	{}

	virtual void visit(Patch& patch) {
		patch.setShader(_shader);
	}

	virtual void visit(Face& face) {
		face.setShader(_shader);
	}
};

void applyShaderToSelection(const std::string& shaderName) {
	UndoableCommand undo("setShader");

	// Construct an applicator class
	ShaderApplicator applicator(shaderName);
	// and traverse the selection using the applicator as walker
	forEachSelectedPrimitive(applicator);

	SceneChangeNotify();
	// Update the Texture Tools
	ui::SurfaceInspector::Instance().queueUpdate();
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
void applyClipboardPatchToFace(Face& target) {
	// Get a reference to the source Texturable in the clipboard
	Texturable& source = GlobalShaderClipboard().getSource();

	// Retrieve the textureprojection from the source face
	TextureProjection projection;
	projection.constructDefault();

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
	GlobalSceneGraph().root()->traverse(finder);

	if (target.isPatch() && entireBrush) {
		gtkutil::errorDialog(
			_("Can't paste shader to entire brush.\nTarget is not a brush."),
			GlobalMainFrame().getTopLevelWindow());
	}
	else {
		// Pass the call to the algorithm function taking care of all the IFs
		applyClipboardToTexturable(target, projected, entireBrush);
	}

	SceneChangeNotify();
	// Update the Texture Tools
	ui::SurfaceInspector::Instance().queueUpdate();
}

void pasteTextureCoords(SelectionTest& test) {
	UndoableCommand undo("pasteTextureCoordinates");

	// Initialise an empty Texturable structure
	Texturable target;

	// Find a suitable target Texturable
	ClosestTexturableFinder finder(test, target);
	GlobalSceneGraph().root()->traverse(finder);

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
			gtkutil::errorDialog(
				_("Can't paste Texture Coordinates.\nTarget patch dimensions must match."),
				GlobalMainFrame().getTopLevelWindow());
		}
	}
	else {
		if (source.isPatch()) {
		 	// Nothing to do, this works for patches only
		 	gtkutil::errorDialog(
				_("Can't paste Texture Coordinates from patches to faces."),
				GlobalMainFrame().getTopLevelWindow());
		}
		else {
			// Nothing to do, this works for patches only
		 	gtkutil::errorDialog(
				_("Can't paste Texture Coordinates from faces."),
				GlobalMainFrame().getTopLevelWindow());
		}
	}

	SceneChangeNotify();
	// Update the Texture Tools
	ui::SurfaceInspector::Instance().queueUpdate();
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
			gtkutil::errorDialog(
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
			gtkutil::errorDialog(
				_("Can't copy Shader. Couldn't retrieve face."),
		 		GlobalMainFrame().getTopLevelWindow());
		}
	}
	else {
		// Nothing to do, this works for patches only
		gtkutil::errorDialog(
			_("Can't copy Shader. Please select a single face or patch."),
			 GlobalMainFrame().getTopLevelWindow());
	}
}

/** greebo: This applies the clipboard to the visited faces/patches.
 */
class ClipboardShaderApplicator :
	public PrimitiveVisitor
{
	bool _natural;
public:
	ClipboardShaderApplicator(bool natural = false) :
		_natural(natural)
	{}

	virtual void visit(Patch& patch) {
		Texturable target;
		target.patch = &patch;
		target.node = patch.getPatchNode().shared_from_this();

		// Apply the shader (projected, not to the entire brush)
		applyClipboardToTexturable(target, !_natural, false);
	}

	virtual void visit(Face& face) {
		Texturable target;
		target.face = &face;
		target.node = face.getBrush().getBrushNode().shared_from_this();

		// Apply the shader (projected, not to the entire brush)
		applyClipboardToTexturable(target, !_natural, false);
	}
};

void pasteShaderToSelection(const cmd::ArgumentList& args) {
	if (GlobalShaderClipboard().getSource().empty()) {
		return;
	}

	// Start a new command
	UndoableCommand command("pasteShaderToSelection");

	ClipboardShaderApplicator applicator;
	forEachSelectedPrimitive(applicator);

	SceneChangeNotify();
	// Update the Texture Tools
	ui::SurfaceInspector::Instance().queueUpdate();
}

void pasteShaderNaturalToSelection(const cmd::ArgumentList& args) {
	if (GlobalShaderClipboard().getSource().empty()) {
		return;
	}

	// Start a new command
	UndoableCommand command("pasteShaderNaturalToSelection");

	// greebo: Construct a visitor and traverse the selection
	ClipboardShaderApplicator applicator(true); // true == paste natural
	forEachSelectedPrimitive(applicator);

	SceneChangeNotify();
	// Update the Texture Tools
	ui::SurfaceInspector::Instance().queueUpdate();
}

TextureProjection getSelectedTextureProjection() {
	TextureProjection returnValue;

	if (selectedFaceCount() == 1) {
		// Get the last selected face instance from the global
		FaceInstance& faceInstance = g_SelectedFaceInstances.last();
		faceInstance.getFace().GetTexdef(returnValue);
	}

	return returnValue;
}

Vector2 getSelectedFaceShaderSize() {
	Vector2 returnValue(0,0);

	if (selectedFaceCount() == 1) {
		// Get the last selected face instance from the global
		FaceInstance& faceInstance = g_SelectedFaceInstances.last();

		returnValue[0] = faceInstance.getFace().getFaceShader().width();
		returnValue[1] = faceInstance.getFace().getFaceShader().height();
	}

	return returnValue;
}

/** greebo: Applies the given texture repeat to the visited patch/face
 */
class TextureFitter :
	public PrimitiveVisitor
{
	float _repeatS, _repeatT;
public:
	TextureFitter(float repeatS, float repeatT) :
		_repeatS(repeatS), _repeatT(repeatT)
	{}

	virtual void visit(Patch& patch) {
		patch.SetTextureRepeat(_repeatS, _repeatT);
	}

	virtual void visit(Face& face) {
		face.fitTexture(_repeatS, _repeatT);
	}
};

void fitTexture(const float& repeatS, const float& repeatT) {
	UndoableCommand command("fitTexture");

	// Instantiate a walker and traverse the selection
	TextureFitter fitter(repeatS, repeatT);
	forEachSelectedPrimitive(fitter);

	SceneChangeNotify();
	// Update the Texture Tools
	ui::SurfaceInspector::Instance().queueUpdate();
}

/** greebo: Flips the visited object about the axis given to the constructor.
 */
class TextureFlipper :
	public PrimitiveVisitor
{
	unsigned int _flipAxis;
public:
	TextureFlipper(unsigned int flipAxis) :
		_flipAxis(flipAxis)
	{}

	virtual void visit(Patch& patch) {
		patch.FlipTexture(_flipAxis);
	}

	virtual void visit(Face& face) {
		face.flipTexture(_flipAxis);
	}
};

void flipTexture(unsigned int flipAxis) {
	UndoableCommand undo("flipTexture");

	// Instantiate the visitor class
	TextureFlipper flipper(flipAxis);
	// traverse the selection
	forEachSelectedPrimitive(flipper);

	SceneChangeNotify();
}

void flipTextureS(const cmd::ArgumentList& args) {
	flipTexture(0);
}

void flipTextureT(const cmd::ArgumentList& args) {
	flipTexture(1);
}

/** greebo: Applies the default texture projection to all
 * the visited faces.
 */
class FaceTextureProjectionSetter :
	public PrimitiveVisitor
{
	TextureProjection& _projection;
public:
	FaceTextureProjectionSetter(TextureProjection& projection) :
		_projection(projection)
	{}

	virtual void visit(Face& face) {
		face.SetTexdef(_projection);
	}
};

class PatchTextureNaturaliser
{
public:
	void operator()(Patch& patch) const {
		patch.NaturalTexture();
	}
};

void naturalTexture(const cmd::ArgumentList& args) {
	UndoableCommand undo("naturalTexture");

	// Patches
	Scene_forEachSelectedPatch(PatchTextureNaturaliser());

	TextureProjection projection;
	projection.constructDefault();

	// Instantiate a visitor and walk the selection
	FaceTextureProjectionSetter projectionSetter(projection);
	forEachSelectedPrimitive(projectionSetter);

	SceneChangeNotify();
	// Update the Texture Tools
	ui::SurfaceInspector::Instance().queueUpdate();
}

void applyTextureProjectionToFaces(TextureProjection& projection) {
	UndoableCommand undo("textureProjectionSetSelected");

	// Instantiate a visitor and walk the selection
	FaceTextureProjectionSetter projectionSetter(projection);
	forEachSelectedPrimitive(projectionSetter);

	SceneChangeNotify();
	// Update the Texture Tools
	ui::SurfaceInspector::Instance().queueUpdate();
}

/** greebo: Translates the texture of the visited faces/patches
 * about the specified <shift> Vector2
 */
class TextureShifter :
	public PrimitiveVisitor
{
	const Vector2& _shift;
public:
	TextureShifter(const Vector2& shift) :
		_shift(shift)
	{}

	virtual void visit(Patch& patch) {
		patch.TranslateTexture(_shift[0], _shift[1]);
	}

	virtual void visit(Face& face) {
		face.shiftTexdef(_shift[0], _shift[1]);
	}
};

void shiftTexture(const Vector2& shift) {
	std::string command("shiftTexture: ");
	command += "s=" + floatToStr(shift[0]) + ", t=" + floatToStr(shift[1]);

	UndoableCommand undo(command);

	TextureShifter shifter(shift);

	// Visit each selected primitive instance using the TextureShifter object
	forEachSelectedPrimitive(shifter);

	SceneChangeNotify();
	// Update the Texture Tools
	ui::SurfaceInspector::Instance().queueUpdate();
}

/** greebo: Scales the texture of the visited faces/patches
 * about the specified x,y-scale values in the given Vector2
 */
class TextureScaler :
	public PrimitiveVisitor
{
	const Vector2& _scale;
	const Vector2& _patchScale;
public:
	TextureScaler(const Vector2& scale, const Vector2& patchScale) :
		_scale(scale),
		_patchScale(patchScale)
	{}

	virtual void visit(Patch& patch) {
		patch.ScaleTexture(_patchScale[0], _patchScale[1]);
	}

	virtual void visit(Face& face) {
		face.scaleTexdef(_scale[0], _scale[1]);
	}
};

void scaleTexture(const Vector2& scale) {
	std::string command("scaleTexture: ");
	command += "sScale=" + floatToStr(scale[0]) + ", tScale=" + floatToStr(scale[1]);

	UndoableCommand undo(command);

	// Prepare the according patch scale value (they are relatively scaled)
	Vector2 patchScale;

	// We need to have 1.05 for a +0.05 scale
	// and a 1/1.05 for a -0.05 scale
	for (int i = 0; i < 2; i++) {
		if (scale[i] >= 0.0f) {
			patchScale[i] = 1.0f + scale[i];
		}
		else {
			patchScale[i] = 1/(1.0f + fabs(scale[i]));
		}
	}

	// Instantiate the texture scaler
	TextureScaler scaler(scale, patchScale);

	// traverse the selection
	forEachSelectedPrimitive(scaler);

	SceneChangeNotify();
	// Update the Texture Tools
	ui::SurfaceInspector::Instance().queueUpdate();
}

/** greebo: Rotates the texture of the visited faces/patches
 * about the specified angle
 */
class TextureRotator :
	public PrimitiveVisitor
{
	const float& _angle;
public:
	TextureRotator(const float& angle) :
		_angle(angle)
	{}

	virtual void visit(Patch& patch) {
		patch.RotateTexture(_angle);
	}

	virtual void visit(Face& face) {
		face.rotateTexdef(_angle);
	}
};

void rotateTexture(const float& angle) {
	std::string command("rotateTexture: ");
	command += "angle=" + floatToStr(angle);

	UndoableCommand undo(command);

	// Instantiate a rotator class and traverse the selection
	TextureRotator rotator(angle);
	forEachSelectedPrimitive(rotator);

	SceneChangeNotify();
	// Update the Texture Tools
	ui::SurfaceInspector::Instance().queueUpdate();
}

void shiftTextureLeft() {
	shiftTexture(Vector2(-GlobalRegistry().getFloat("user/ui/textures/surfaceInspector/hShiftStep"), 0.0f));
}

void shiftTextureRight() {
	shiftTexture(Vector2(GlobalRegistry().getFloat("user/ui/textures/surfaceInspector/hShiftStep"), 0.0f));
}

void shiftTextureUp() {
	shiftTexture(Vector2(0.0f, GlobalRegistry().getFloat("user/ui/textures/surfaceInspector/vShiftStep")));
}

void shiftTextureDown() {
	shiftTexture(Vector2(0.0f, -GlobalRegistry().getFloat("user/ui/textures/surfaceInspector/vShiftStep")));
}

void scaleTextureLeft() {
	scaleTexture(Vector2(-GlobalRegistry().getFloat("user/ui/textures/surfaceInspector/hScaleStep"), 0.0f));
}

void scaleTextureRight() {
	scaleTexture(Vector2(GlobalRegistry().getFloat("user/ui/textures/surfaceInspector/hScaleStep"), 0.0f));
}

void scaleTextureUp() {
	scaleTexture(Vector2(0.0f, GlobalRegistry().getFloat("user/ui/textures/surfaceInspector/vScaleStep")));
}

void scaleTextureDown() {
	scaleTexture(Vector2(0.0f, -GlobalRegistry().getFloat("user/ui/textures/surfaceInspector/vScaleStep")));
}

void rotateTextureClock() {
	rotateTexture(fabs(GlobalRegistry().getFloat("user/ui/textures/surfaceInspector/rotStep")));
}

void rotateTextureCounter() {
	rotateTexture(-fabs(GlobalRegistry().getFloat("user/ui/textures/surfaceInspector/rotStep")));
}

void rotateTexture(const cmd::ArgumentList& args) {
	if (args.size() != 1) {
		globalOutputStream() << "Usage: TexRotate [+1|-1]" << std::endl;
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
		globalOutputStream() << "Usage: TexScale 's t'" << std::endl;
		globalOutputStream() << "       TexScale [up|down|left|right]" << std::endl;
		globalOutputStream() << "Example: TexScale '0.05 0' performs"
			<< " a 105% scale in the s direction." << std::endl;
		globalOutputStream() << "Example: TexScale up performs"
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
		globalOutputStream() << "Usage: TexShift 's t'" << std::endl
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

/**
 * greebo: Aligns the texture of the visited faces/patches
 * to the given edge.
 */
class TextureAligner :
	public PrimitiveVisitor
{
	const EAlignType _align;
public:
	TextureAligner(EAlignType align) :
		_align(align)
	{}

	void visit(Patch& patch)
	{
		patch.alignTexture(_align);
	}

	void visit(Face& face)
	{
		face.alignTexture(_align);
	}
};

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

	// Instantiate an aligner class and traverse the selection
	TextureAligner aligner(align);
	forEachSelectedPrimitive(aligner);

	SceneChangeNotify();
	// Update the Texture Tools
	ui::SurfaceInspector::Instance().queueUpdate();
}

void alignTextureCmd(const cmd::ArgumentList& args)
{
	if (args.size() != 1)
	{
		globalOutputStream() << "Usage: TexAlign [top|bottom|left|right]" << std::endl;
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
		globalOutputStream() << "Usage: TexAlign [top|bottom|left|right]" << std::endl;
	}
}

/** greebo: Normalises the texture of the visited faces/patches.
 */
class TextureNormaliser :
	public PrimitiveVisitor
{
public:
	virtual void visit(Patch& patch) {
		patch.normaliseTexture();
	}

	virtual void visit(Face& face) {
		face.normaliseTexture();
	}
};

void normaliseTexture(const cmd::ArgumentList& args) {
	UndoableCommand undo("normaliseTexture");

	TextureNormaliser normaliser;
	forEachSelectedPrimitive(normaliser);

	SceneChangeNotify();
	// Update the Texture Tools
	ui::SurfaceInspector::Instance().queueUpdate();
}

/** greebo: This replaces the shader of the visited face/patch with <replace>
 * 			if the face is textured with <find> and increases the given <counter>.
 */
class ShaderReplacer :
	public BrushInstanceVisitor
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
	virtual void visit(FaceInstance& face) const {
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

	if (selectedOnly) {
		if (GlobalSelectionSystem().Mode() != SelectionSystem::eComponent) {
			// Find & replace all the brush shaders
			Scene_ForEachSelectedBrush_ForEachFace(GlobalSceneGraph(), replacer);
			Scene_forEachSelectedPatch(replacer);
		}

		// Search the single selected faces
		Scene_ForEachSelectedBrushFace(GlobalSceneGraph(), replacer);
	}
	else {
		Scene_ForEachBrush_ForEachFace(replacer);
		// Search all patches
		Scene_forEachVisiblePatch(replacer);
	}

	return replacer.getReplacedCount();
}

	} // namespace algorithm
} // namespace selection
