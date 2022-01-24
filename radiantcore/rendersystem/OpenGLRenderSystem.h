#pragma once

#include "irender.h"
#include <sigc++/connection.h>
#include <map>
#include "imodule.h"
#include "backend/OpenGLStateManager.h"
#include "backend/OpenGLShader.h"
#include "backend/OpenGLStateLess.h"
#include "backend/TextRenderer.h"

namespace render
{

class GLProgramFactory;
typedef std::shared_ptr<GLProgramFactory> GLProgramFactoryPtr;

/**
 * \brief
 * Implementation of RenderSystem.
 */
class OpenGLRenderSystem
: public RenderSystem,
  public OpenGLStateManager
{
	// Map of named Shader objects
    std::map<std::string, OpenGLShaderPtr> _shaders;

	// whether this module has been realised
	bool _realised;

	bool _shaderProgramsAvailable;

    // The GL program manager to acquire vfps
    GLProgramFactoryPtr _glProgramFactory;

    // Current shader program in use
    ShaderProgram _currentShaderProgram;

	// Map of OpenGLState references, with access functions.
	OpenGLStates _state_sorted;

    using FontKey = std::pair<IGLFont::Style, std::size_t>;
    std::map<FontKey, std::shared_ptr<TextRenderer>> _textRenderers;

	// Render time
	std::size_t _time;

	sigc::signal<void> _sigExtensionsInitialised;

	sigc::connection _materialDefsLoaded;
	sigc::connection _materialDefsUnloaded;
	sigc::connection _sharedContextCreated;
	sigc::connection _sharedContextDestroyed;

public:

	/**
	 * Main constructor.
	 */
	OpenGLRenderSystem();

	virtual ~OpenGLRenderSystem();

    /* RenderSystem implementation */

    ITextRenderer::Ptr captureTextRenderer(IGLFont::Style style, std::size_t size) override;

	ShaderPtr capture(const std::string& name) override;
    ShaderPtr capture(BuiltInShaderType type) override;
	void render(RenderViewType renderViewType, RenderStateFlags globalstate,
                const Matrix4& modelview,
                const Matrix4& projection,
                const Vector3& viewer,
                const VolumeTest& view) override;
	void realise() override;
	void unrealise() override;

    GLProgramFactory& getGLProgramFactory();

	std::size_t getTime() const override;
	void setTime(std::size_t milliSeconds) override;

    ShaderProgram getCurrentShaderProgram() const override;
    void setShaderProgram(ShaderProgram prog) override;

	void extensionsInitialised() override;
	sigc::signal<void> signal_extensionsInitialised() override;

	bool shaderProgramsAvailable() const override;
	void setShaderProgramsAvailable(bool available) override;

	typedef std::set<Renderable*> Renderables;
	Renderables m_renderables;
	mutable bool m_traverseRenderablesMutex;

    /* OpenGLStateManager implementation */
	void insertSortedState(const OpenGLStates::value_type& val) override;
	void eraseSortedState(const OpenGLStates::key_type& key) override;

	// renderables
	void attachRenderable(Renderable& renderable) override;
	void detachRenderable(Renderable& renderable) override;
	void forEachRenderable(const RenderableCallback& callback) const override;

	// RegisterableModule implementation
    virtual const std::string& getName() const override;
    virtual const StringSet& getDependencies() const override;
    virtual void initialiseModule(const IApplicationContext& ctx) override;
    virtual void shutdownModule() override;

private:
    ShaderPtr capture(const std::string& name, const std::function<OpenGLShaderPtr()>& createShader);
};
typedef std::shared_ptr<OpenGLRenderSystem> OpenGLRenderSystemPtr;

} // namespace render

