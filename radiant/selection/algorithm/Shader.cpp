#include "Shader.h"

#include "iselection.h"
#include "iscenegraph.h"
#include "brush/FaceInstance.h"
#include "brush/BrushVisit.h"
#include "brush/TextureProjection.h"
#include "patch/PatchSceneWalk.h"

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

class FaceGetTexdef
{
	TextureProjection& _projection;
	mutable bool _done;
public:
	FaceGetTexdef(TextureProjection& projection) : 
		_projection(projection), 
		_done(false) 
	{}
			
	void operator()(Face& face) const {
		if (!_done) {
			_done = true;
			face.GetTexdef(_projection);
		}
	}
};

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

int selectedFaceCount() {
	return static_cast<int>(g_SelectedFaceInstances.size());
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
	
}

void scaleTextureRight() {
	
}

void scaleTextureUp() {
	
}

void scaleTextureDown() {
	
}

void rotateTextureClock() {
	
}

void rotateTextureCounter() {
	
}

	} // namespace algorithm
} // namespace selection
