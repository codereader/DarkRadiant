#ifndef MODELPREVIEWRENDERER_H_
#define MODELPREVIEWRENDERER_H_

#include "irenderable.h"

#include <vector>

namespace ui
{

/**
 * Renderer implementation used for the Model Preview. This is a simpler version
 * of the CamRenderer used for the 3D view, without handling of selected
 * objects.
 */
class ModelPreviewRenderer
: public Renderer
{
	// Internal class used for sorting shaders
	struct StateType {
		
		// The Shader itself
		ShaderPtr shader;
		
		// Pointer to the LightList
		const LightList* lights;
		
		// Constructor
		StateType()
		: lights(NULL)
		{ }
	};
	
	// Shader stack
	std::vector<StateType> _shaderStack;
	
	// Global OpenGL state
	RenderStateFlags _globalState;
	
	// Viewer location
	const Vector3& _viewer;
	
public:
	
	/**
	 * Main constructor, sets the global state and the viewer location.
	 */
	ModelPreviewRenderer(RenderStateFlags globalState, const Vector3& viewer)
	: _shaderStack(1),
	  _globalState(globalState), 
	  _viewer(viewer)
	{ }
	
	/**
	 * Set current shader.
	 */
	void SetState(ShaderPtr shader, EStyle style) {
	    if(style == eFullMaterials) {
	      _shaderStack.back().shader = shader;
	    }
	}
	
	/**
	 * Return render style.
	 */
	const EStyle getStyle() const {
		return eFullMaterials;
	}
	
	/**
	 * Push shader.
	 */
	void PushState() {
		_shaderStack.push_back(_shaderStack.back());
	}
	
	/**
	 * Pop shader.
	 */
	void PopState() {
		_shaderStack.pop_back();
	}
	
	/**
	 * Set highlighting mode. This Renderer does not support highlighting so 
	 * this is a NOP.
	 */
	void Highlight(EHighlightMode mode, bool enable) { }
	
	/**
	 * Set the LightList to use for lighting mode rendering.
	 */
	void setLights(const LightList& lights) {
		_shaderStack.back().lights = &lights;
	}
	
	/**
	 * Submit a Renderable object to be rendered.
	 */
	void addRenderable(const OpenGLRenderable& renderable, const Matrix4& world)
	{
		// Pass the Renderable to the current Shader
		_shaderStack.back().shader->addRenderable(renderable, 
												  world,
												  _shaderStack.back().lights);
	}
	
	/**
	 * Render all Shaders.
	 */
	void render(const Matrix4& modelView, const Matrix4& projection) {
		GlobalShaderCache().render(
				_globalState, modelView, projection, _viewer
		);
	}

};

}

#endif /*MODELPREVIEWRENDERER_H_*/
