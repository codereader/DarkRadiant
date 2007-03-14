#include "Shader.h"

#include "iselection.h"
#include "iscenegraph.h"
#include "selectable.h"
#include "selectionlib.h"
#include "gtkutil/dialog.h"
#include "mainframe.h"
#include "brush/FaceInstance.h"
#include "brush/BrushVisit.h"
#include "brush/TextureProjection.h"
#include "patch/PatchSceneWalk.h"
#include "selection/algorithm/Primitives.h"
#include "selection/shaderclipboard/ShaderClipboard.h"
#include "ui/surfaceinspector/SurfaceInspector.h"

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

/** greebo: Cycles through all the Faces and throws as soon as 
 * at least two different non-empty shader names are found.
 * 
 * @throws: AmbiguousShaderException
 */
class UniqueFaceShaderFinder
{
	// The string containing the result
	mutable std::string& _shader;
	
public:
	UniqueFaceShaderFinder(std::string& shader) : 
		_shader(shader)
	{}

	void operator()(FaceInstance& face) const {
		
		std::string foundShader = face.getFace().GetShader();
			
		if (foundShader != "$NONE" && _shader != "$NONE" && 
			_shader != foundShader) 
		{
			throw AmbiguousShaderException(foundShader);
		}
		
		_shader = foundShader;
	}
};

/** greebo: Cycles through all the Patches and throws as soon as 
 * at least two different non-empty shader names are found.
 * 
 * @throws: AmbiguousShaderException
 */
class UniquePatchShaderFinder
{
	// The string containing the result
	mutable std::string& _shader;
	
public:
	UniquePatchShaderFinder(std::string& shader) : 
		_shader(shader)
	{}

	void operator()(PatchInstance& patch) const {
		
		std::string foundShader = patch.getPatch().GetShader();
			
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
					UniquePatchShaderFinder(patchShader)
				);
			}
			catch (AmbiguousShaderException a) {
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
					UniqueFaceShaderFinder(faceShader)
				);
			}
			catch (AmbiguousShaderException a) {
				faceShader = "";
			}
		}
		else {
			// Try to get the unique shader from the faces
			try {
				g_SelectedFaceInstances.foreach(UniqueFaceShaderFinder(faceShader));
			}
			catch (AmbiguousShaderException a) {
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

/** greebo: Applies the given shader to the visited patch
 */
class PatchShaderSetter
{
	std::string _shader;
public:
	PatchShaderSetter(const std::string& shader) : 
		_shader(shader) 
	{}
	
	void operator()(Patch& patch) const {
		patch.SetShader(_shader);
	}
};

/** greebo: Applies the given texture repeat to the visited face
 */
class FaceShaderSetter
{
	std::string _shader;
public:
	FaceShaderSetter(const std::string& shader) : 
		_shader(shader) 
	{}
	
	void operator()(Face& face) const {
		face.SetShader(_shader);
	}
};

void applyShaderToSelection(const std::string& shaderName) {
	UndoableCommand undo("setShader");
	
	// Patches
	Scene_forEachVisibleSelectedPatch(PatchShaderSetter(shaderName));
	
	// Brushes
	if (GlobalSelectionSystem().Mode() != SelectionSystem::eComponent) {
		Scene_ForEachSelectedBrush_ForEachFace(
			GlobalSceneGraph(), 
			FaceShaderSetter(shaderName)
		);
	}
	
	// Faces
	Scene_ForEachSelectedBrushFace(
		GlobalSceneGraph(), 
		FaceShaderSetter(shaderName)
	);
	
	SceneChangeNotify();
	// Update the Texture Tools
	ui::SurfaceInspector::Instance().update();
}

/** greebo: Applies the shader from the clipboard's face to the given <target> face
 */
void applyClipboardFaceToFace(Face& target) {
	// Get a reference to the source Texturable in the clipboard
	Texturable& source = GlobalShaderClipboard().getSource();
	
	// Retrieve the textureprojection from the source face
	TextureProjection projection;
	source.face->GetTexdef(projection);	
	
	target.SetShader(source.face->GetShader());
	target.SetTexdef(projection);
	target.SetFlags(source.face->getShader().m_flags);
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
	target.SetShader(source.patch->GetShader());
	target.SetTexdef(projection);
	target.SetFlags(ContentsFlagsValue(0, 0, 0, false));
}

void pasteShader(SelectionTest& test, bool projected, bool entireBrush) {
	// Construct the command string
	std::string command("pasteShader");
	command += (projected ? "Projected" : "Natural");
	command += (entireBrush ? "ToBrush" : "");
	
	UndoableCommand undo(command.c_str());
	
	// Initialise an empty Texturable structure
	Texturable target;
	
	// Find a suitable target Texturable
	GlobalSceneGraph().traverse(ClosestTexturableFinder(test, target));
	
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
			 	target.patch->SetShader(source.face->GetShader());
			 	
			 	// Either paste the texture projected or naturally
			 	if (projected) {
			 		target.patch->pasteTextureProjected(source.face);
			 	}
			 	else {
			 		target.patch->pasteTextureNatural(source.face);
			 	}
			}
			else if (target.isPatch() && entireBrush) {
				gtkutil::errorDialog("Can't paste shader to entire brush.\nTarget is not a brush.",
					MainFrame_getWindow());
			}
		}
		else {
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
			 	target.patch->SetShader(source.patch->GetShader());
			 	target.patch->pasteTextureNatural(*source.patch);
			}
			else if (target.isPatch() && entireBrush) {
				gtkutil::errorDialog("Can't paste shader to entire brush.\nSource and target are not a brush.",
					MainFrame_getWindow());
			}
		}
	}
	
	SceneChangeNotify();
	// Update the Texture Tools
	ui::SurfaceInspector::Instance().update();
}

void pasteTextureCoords(SelectionTest& test) {
	UndoableCommand undo("pasteTextureCoordinates");
	
	// Initialise an empty Texturable structure
	Texturable target;
	
	// Find a suitable target Texturable
	GlobalSceneGraph().traverse(ClosestTexturableFinder(test, target));
	
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
			gtkutil::errorDialog("Can't paste Texture Coordinates.\nTarget patch dimensions must match.",
					MainFrame_getWindow());
		}
	}
	else {
		if (source.isPatch()) {
		 	// Nothing to do, this works for patches only
		 	gtkutil::errorDialog("Can't paste Texture Coordinates from patches to faces.",
							 MainFrame_getWindow());
		}
		else {
			// Nothing to do, this works for patches only
		 	gtkutil::errorDialog("Can't paste Texture Coordinates from faces.",
							 MainFrame_getWindow());
		}
	}
	
	SceneChangeNotify();
	// Update the Texture Tools
	ui::SurfaceInspector::Instance().update();
}

void pickShaderFromSelection() {
	GlobalShaderClipboard().clear();
	
	const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();
	
	// Check for a single patch
	if (info.totalCount == 1 && info.patchCount == 1) {
		try {
			Patch& sourcePatch = getLastSelectedPatch();
			GlobalShaderClipboard().setSource(sourcePatch);
		}
		catch (InvalidSelectionException e) {
			gtkutil::errorDialog("Can't copy Shader. Couldn't retrieve patch.",
		 		MainFrame_getWindow());
		}
	}
	else if (selectedFaceCount() == 1) {
		try {
			Face& sourceFace = getLastSelectedFace();
			GlobalShaderClipboard().setSource(sourceFace);
		}
		catch (InvalidSelectionException e) {
			gtkutil::errorDialog("Can't copy Shader. Couldn't retrieve face.",
		 		MainFrame_getWindow());
		}
	}
	else {
		// Nothing to do, this works for patches only
		gtkutil::errorDialog("Can't copy Shader. Please select a single face or patch.",
			 MainFrame_getWindow());
	}
}

void pasteShaderToSelection() {
	
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
		
		returnValue[0] = faceInstance.getFace().getShader().width();
		returnValue[1] = faceInstance.getFace().getShader().height();
	}
	
	return returnValue;
}

/** greebo: Applies the given texture repeat to the visited patch
 */
class PatchTextureFitter
{
	float _repeatS, _repeatT;
public:
	PatchTextureFitter(float repeatS, float repeatT) : 
		_repeatS(repeatS), _repeatT(repeatT) 
	{}
	
	void operator()(Patch& patch) const {
		patch.SetTextureRepeat(_repeatS, _repeatT);
	}
};

/** greebo: Applies the given texture repeat to the visited face
 */
class FaceTextureFitter
{
	float _repeatS, _repeatT;
public:
	FaceTextureFitter(float repeatS, float repeatT) : 
		_repeatS(repeatS), _repeatT(repeatT) 
	{}
	
	void operator()(Face& face) const {
		face.FitTexture(_repeatS, _repeatT);
	}
};

void fitTexture(const float& repeatS, const float& repeatT) {
	UndoableCommand command("fitTexture");
	
	// Cycle through all selected brushes
	if (GlobalSelectionSystem().Mode() != SelectionSystem::eComponent) {
		Scene_ForEachSelectedBrush_ForEachFace(GlobalSceneGraph(), FaceTextureFitter(repeatS, repeatT));
	}
	
	// Cycle through all selected components
	Scene_ForEachSelectedBrushFace(GlobalSceneGraph(), FaceTextureFitter(repeatS, repeatT));
	
	// Cycle through all the selected patches
	Scene_forEachVisibleSelectedPatch(PatchTextureFitter(repeatS, repeatT));
	
	SceneChangeNotify();
	// Update the Texture Tools
	ui::SurfaceInspector::Instance().update();
}

/** greebo: Applies the default texture projection to all
 * the visited faces.
 */
class FaceTextureProjectionSetter
{
	TextureProjection& _projection;
public:
	FaceTextureProjectionSetter(TextureProjection& projection) : 
		_projection(projection) 
	{}
	
	void operator()(Face& face) const {
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

void naturalTexture() {
	UndoableCommand undo("naturalTexture");
	
	// Patches
	Scene_forEachVisibleSelectedPatch(PatchTextureNaturaliser());
	
	TextureProjection projection;
	projection.constructDefault();
	
	if (GlobalSelectionSystem().Mode() != SelectionSystem::eComponent) {
		Scene_ForEachSelectedBrush_ForEachFace(
			GlobalSceneGraph(), 
			FaceTextureProjectionSetter(projection)
		);
	}
	
	// Faces
	Scene_ForEachSelectedBrushFace(
		GlobalSceneGraph(), 
		FaceTextureProjectionSetter(projection)
	);
	
	SceneChangeNotify();
	// Update the Texture Tools
	ui::SurfaceInspector::Instance().update();
}

void applyTextureProjectionToFaces(TextureProjection& projection) {
	UndoableCommand undo("textureProjectionSetSelected");
	
	if (GlobalSelectionSystem().Mode() != SelectionSystem::eComponent) {
		Scene_ForEachSelectedBrush_ForEachFace(
			GlobalSceneGraph(), 
			FaceTextureProjectionSetter(projection)
		);
	}
	
	// Faces
	Scene_ForEachSelectedBrushFace(
		GlobalSceneGraph(), 
		FaceTextureProjectionSetter(projection)
	);
	
	SceneChangeNotify();
	// Update the Texture Tools
	ui::SurfaceInspector::Instance().update();
}

/** greebo: Translates the texture of the visited faces
 * about the specified <shift> Vector2
 */
class FaceTextureShifter
{
	const Vector2& _shift;
public:
	FaceTextureShifter(const Vector2& shift) : 
		_shift(shift) 
	{}
	
	void operator()(Face& face) const {
		face.ShiftTexdef(_shift[0], _shift[1]);
	}
};

/** greebo: Translates the texture of the visited patches
 * about the specified <shift> Vector2
 */
class PatchTextureShifter
{
	const Vector2& _shift;
public:
	PatchTextureShifter(const Vector2& shift) :
		_shift(shift) 
	{}
	
	void operator()(Patch& patch) const {
		patch.TranslateTexture(_shift[0], _shift[1]);
	}
};

void shiftTexture(const Vector2& shift) {
	std::string command("shiftTexture: ");
	command += "s=" + floatToStr(shift[0]) + ", t=" + floatToStr(shift[1]);
	
	UndoableCommand undo(command.c_str());
	
	if (GlobalSelectionSystem().Mode() != SelectionSystem::eComponent) {
		Scene_ForEachSelectedBrush_ForEachFace(GlobalSceneGraph(), FaceTextureShifter(shift));
  		Scene_forEachVisibleSelectedPatch(PatchTextureShifter(shift));
	}
	// Translate the face textures
	Scene_ForEachSelectedBrushFace(
		GlobalSceneGraph(), 
		FaceTextureShifter(shift)
	);
	
	SceneChangeNotify();
	// Update the Texture Tools
	ui::SurfaceInspector::Instance().update();
}

/** greebo: Scales the texture of the visited faces
 * about the specified x,y-scale values in the given Vector2
 */
class FaceTextureScaler
{
	const Vector2& _scale;
public:
	FaceTextureScaler(const Vector2& scale) : 
		_scale(scale) 
	{}
	
	void operator()(Face& face) const {
		face.ScaleTexdef(_scale[0], _scale[1]);
	}
};

/** greebo: Scales the texture of the visited patches
 * about the specified x,y-scale values in the given Vector2
 */
class PatchTextureScaler
{
	const Vector2& _scale;
public:
	PatchTextureScaler(const Vector2& scale) :
		_scale(scale) 
	{}
	
	void operator()(Patch& patch) const {
		patch.ScaleTexture(_scale[0], _scale[1]);
	}
};

/** greebo: Rotates the texture of the visited faces
 * about the specified angle
 */
class FaceTextureRotater
{
	const float& _angle;
public:
	FaceTextureRotater(const float& angle) : 
		_angle(angle) 
	{}
	
	void operator()(Face& face) const {
		face.RotateTexdef(_angle);
	}
};

/** greebo: Rotates the texture of the visited patches
 * about the specified angle
 */
class PatchTextureRotater
{
	const float& _angle;
public:
	PatchTextureRotater(const float& angle) : 
		_angle(angle) 
	{}
	
	void operator()(Patch& patch) const {
		patch.RotateTexture(_angle);
	}
};

void scaleTexture(const Vector2& scale) {
	std::string command("scaleTexture: ");
	command += "sScale=" + floatToStr(scale[0]) + ", tScale=" + floatToStr(scale[1]);

	UndoableCommand undo(command.c_str());
	
	if (GlobalSelectionSystem().Mode() != SelectionSystem::eComponent) {
		Scene_ForEachSelectedBrush_ForEachFace(
			GlobalSceneGraph(), 
			FaceTextureScaler(scale)
		);
		
		// Prepare the according patch scale value
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
		
		// Pass the scale to the patches as they are scaled in relative steps
  		Scene_forEachVisibleSelectedPatch(PatchTextureScaler(patchScale));
	}
	// Scale the face textures
	Scene_ForEachSelectedBrushFace(
		GlobalSceneGraph(), 
		FaceTextureScaler(scale)
	);
	
	SceneChangeNotify();
	// Update the Texture Tools
	ui::SurfaceInspector::Instance().update();
}

void rotateTexture(const float& angle) {
	std::string command("rotateTexture: ");
	command += "angle=" + floatToStr(angle);
	
	UndoableCommand undo(command.c_str());
	
	if (GlobalSelectionSystem().Mode() != SelectionSystem::eComponent) {
		Scene_ForEachSelectedBrush_ForEachFace(
			GlobalSceneGraph(), 
			FaceTextureRotater(angle)
		);
  		Scene_forEachVisibleSelectedPatch(PatchTextureRotater(angle));
	}
	
	// Rotate the face textures
	Scene_ForEachSelectedBrushFace(
		GlobalSceneGraph(), 
		FaceTextureRotater(angle)
	);

	SceneChangeNotify();
	// Update the Texture Tools
	ui::SurfaceInspector::Instance().update();
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

/** greebo: Normalises the texture of the visited faces.
 */
class FaceTextureNormaliser
{
public:
	void operator()(Face& face) const {
		face.normaliseTexture();
	}
};

/** greebo: Normalises the texture of the visited patches.
 */
class PatchTextureNormaliser
{
public:
	void operator()(Patch& patch) const {
		patch.normaliseTexture();
	}
};

void normaliseTexture() {
	UndoableCommand undo("normaliseTexture");
	
	if (GlobalSelectionSystem().Mode() != SelectionSystem::eComponent) {
		Scene_ForEachSelectedBrush_ForEachFace(
			GlobalSceneGraph(), 
			FaceTextureNormaliser()
		);
  		Scene_forEachVisibleSelectedPatch(PatchTextureNormaliser());
	}
	
	// Normalise the face textures (single face instances)
	Scene_ForEachSelectedBrushFace(
		GlobalSceneGraph(), 
		FaceTextureNormaliser()
	);

	SceneChangeNotify();
	// Update the Texture Tools
	ui::SurfaceInspector::Instance().update();
}

/** greebo: This replaces the shader of the visited face with <replace>
 * 			if the face is textured with <find> and increases the given <counter>. 
 */
class FaceShaderReplacer
{
	const std::string& _find;
	const std::string& _replace;
	int& _counter;
public:
	FaceShaderReplacer(const std::string& find, const std::string& replace, int& counter) : 
		_find(find), 
		_replace(replace),
		_counter(counter)
	{}
	
	void operator()(Face& face) const {
		if (face.GetShader() == _find) {
			face.SetShader(_replace);
			_counter++;
		}
	}
};

/** greebo: This replaces the shader of the visited face with <replace>
 * 			if the face is textured with <find> and increases the given <counter>. 
 */
class PatchShaderReplacer
{
	const std::string& _find;
	const std::string& _replace;
	int& _counter;
public:
	PatchShaderReplacer(const std::string& find, const std::string& replace, int& counter) : 
		_find(find), 
		_replace(replace),
		_counter(counter)
	{}
	
	void operator()(Patch& patch) const {
		if (patch.GetShader() == _find) {
			patch.SetShader(_replace);
			_counter++;
		}
	}
};

int findAndReplaceShader(const std::string find, 
	const std::string replace, bool selectedOnly)
{
	// This gets increased by each walker on successful replacement
	int counter = 0;
	
	std::string command("textureFindReplace");
	command += "-find " + find + " -replace " + replace;
	UndoableCommand undo(command.c_str());
	
	if (selectedOnly) {
		if (GlobalSelectionSystem().Mode() != SelectionSystem::eComponent) {
			// Find & replace all the brush shaders
			Scene_ForEachSelectedBrush_ForEachFace(
				GlobalSceneGraph(), 
				FaceShaderReplacer(find, replace, counter)
			);
			
			Scene_forEachVisibleSelectedPatch(
				PatchShaderReplacer(find, replace, counter)
			);
		}
		
		// Search the single selected faces 
		Scene_ForEachSelectedBrushFace(
			GlobalSceneGraph(), 
			FaceShaderReplacer(find, replace, counter)
		);
	}
	else {
		Scene_ForEachBrush_ForEachFace(
			GlobalSceneGraph(), 
			FaceShaderReplacer(find, replace, counter)
		);
		// Search all patches
		Scene_forEachVisiblePatch(PatchShaderReplacer(find, replace, counter));
	}
	
	return counter;
}

	} // namespace algorithm
} // namespace selection
