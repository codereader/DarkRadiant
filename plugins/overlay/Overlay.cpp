#include "ioverlay.h"

#include "iregistry.h"
#include "preferencesystem.h"
#include "irender.h"
#include "renderable.h"

#include "math/matrix.h"

#include <iostream>

namespace {
	const std::string RKEY_OVERLAY_VISIBLE = "user/ui/xyview/overlay/visible";
}

class Overlay : 
	public IOverlay,
	public RegistryKeyObserver,
	public PreferenceConstructor,
	public Renderable,
	public OpenGLRenderable
{
public:
	// Radiant Module stuff
	typedef IOverlay Type;
	STRING_CONSTANT(Name, "*");

	// Return the static instance
	IOverlay* getTable() {
		return this;
	}

private:
	
	// The shadername to be drawn
	std::string _imageName;
	
	// TRUE, if the overlay is visible
	bool _visible;
	
	// A bool to prevent registry keychanged loops
	bool _callbackActive;
	
	Shader* _renderState;

public:
	
	Overlay() :
		_imageName(""),
		_visible(false),
		_renderState(NULL)
	{
		std::cout << "Overlay module initialised\n";
		
		// Watch the relevant registry keys		
		GlobalRegistry().addKeyObserver(this, RKEY_OVERLAY_VISIBLE);

		// greebo: Register this class in the preference system so that the constructPreferencePage() gets called.
		GlobalPreferenceSystem().addConstructor(this);
		
		_imageName = "textures/darkmod/gravel/floor/gravel_001";
		
		// Load the values from the registry
		keyChanged();
	}
	
	~Overlay() {
		std::cout << "Overlay module destructed\n";
		releaseShader();
	}
	
	void show(bool shown) {
		if (_renderState != NULL) {
			_visible = shown;
		}
		else {
			std::cout << "Can't show image, renderstate is NULL!\n";
		}
		
		// Prevent callback loops
		_callbackActive = true;
		
		// Store the state into the registry, to reflect the actual state
		GlobalRegistry().set(RKEY_OVERLAY_VISIBLE, _visible ? "1" : "0");
		
		_callbackActive = false;
	}
	
	// Sets the name of the image that should be loaded
	void setImage(const std::string& imageName) {
		releaseShader();
		_imageName = imageName;
		captureShader();
	}
	
	void setTransparency(const float& transparency) {}
	
	void setImageScale(const float& scale) {}
	void setImagePosition(const unsigned int& x, const unsigned int& y) {}

	// RegistryKeyObserver implementation, gets called upon key change
	void keyChanged() {
		if (_callbackActive) {
			return;
		}
		
		show(GlobalRegistry().get(RKEY_OVERLAY_VISIBLE) == "1");
	}
	
	// PreferenceConstructor implementation, add the preference settings
	void constructPreferencePage(PreferenceGroup& group) {
		PreferencesPage* page(group.createPage("Overlay", "Orthoview Background Overlay"));
	
		page->appendCheckBox("", "Show Background Image", RKEY_OVERLAY_VISIBLE);
	}
	
	// OpenGLRenderable implementation, back-end render method
	void render(RenderStateFlags state) const {
		std::cout << "Render called\n";
	}
	
	// Renderable implementation
	/** Submit renderable geometry when rendering takes place in Solid mode.
	 */
	void renderSolid(Renderer& renderer, const VolumeTest& volume) const {
		std::cout << "renderSolid called\n";
		if (_visible) {
			renderer.SetState(_renderState, Renderer::eWireframeOnly);
			renderer.SetState(_renderState, Renderer::eFullMaterials);
			renderer.addRenderable(*this, Matrix4::getIdentity());
		}
	}

	// Renderable implementation
	/** Submit renderable geometry when rendering takes place in Wireframe
	 * mode.
	 */
	void renderWireframe(Renderer& renderer, const VolumeTest& volume) const {
		std::cout << "renderWireframe called\n";
	}
	
private:

	void captureShader() {
		if (_renderState == NULL) {
			std::cout << "Capturing shader: " << _imageName.c_str() << std::endl;
			_renderState = GlobalShaderCache().capture(_imageName.c_str());
		}
		else {
			std::cout << "Shader already captured!\n";
		}
	}
	
	void releaseShader() {
		if (_renderState != NULL) {
			std::cout << "Releasing shader: " << _imageName.c_str() << std::endl;
			GlobalShaderCache().release(_imageName.c_str());
			_renderState = NULL;
		}
	}

}; // class Overlay


/* Overlay dependencies class. 
 */
class OverlayDependencies :
	public GlobalRegistryModuleRef,
	public GlobalPreferenceSystemModuleRef,
	public GlobalShaderCacheModuleRef
{
};

/* Required code to register the module with the ModuleServer.
 */
#include "modulesystem/singletonmodule.h"

typedef SingletonModule<Overlay, OverlayDependencies> OverlayModule;

// Static instance of the OverlayModule
OverlayModule _theOverlayModule;

extern "C" void RADIANT_DLLEXPORT Radiant_RegisterModules(ModuleServer& server)
{
  initialiseModule(server);
  _theOverlayModule.selfRegister();
}

