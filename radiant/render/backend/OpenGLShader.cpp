#include "OpenGLShader.h"
#include "GLProgramFactory.h"
#include "OpenGLStateBucketAdd.h"
#include "OpenGLStateMap.h"
#include "render/OpenGLShaderCache.h"

#include "iuimanager.h"
#include "ishaders.h"
#include "ifilter.h"
#include "irender.h"
#include "generic/callback.h"
#include "texturelib.h"

namespace {

/*	
 * Convert a blend factor from one format to another. TODO: either refactor
 * into class method or remove entirely.
 */
inline GLenum convertBlendFactor(BlendFactor factor)
{
  switch(factor)
  {
  case BLEND_ZERO:
    return GL_ZERO;
  case BLEND_ONE:
    return GL_ONE;
  case BLEND_SRC_COLOUR:
    return GL_SRC_COLOR;
  case BLEND_ONE_MINUS_SRC_COLOUR:
    return GL_ONE_MINUS_SRC_COLOR;
  case BLEND_SRC_ALPHA:
    return GL_SRC_ALPHA;
  case BLEND_ONE_MINUS_SRC_ALPHA:
    return GL_ONE_MINUS_SRC_ALPHA;
  case BLEND_DST_COLOUR:
    return GL_DST_COLOR;
  case BLEND_ONE_MINUS_DST_COLOUR:
    return GL_ONE_MINUS_DST_COLOR;
  case BLEND_DST_ALPHA:
    return GL_DST_ALPHA;
  case BLEND_ONE_MINUS_DST_ALPHA:
    return GL_ONE_MINUS_DST_ALPHA;
  case BLEND_SRC_ALPHA_SATURATE:
    return GL_SRC_ALPHA_SATURATE;
  }
  return GL_ZERO;
}
	
} // namespace

void OpenGLShader::destroy() {
	// Clear the shaderptr, so that the shared_ptr reference count is decreased 
    m_shader = IShaderPtr();

    for(Passes::iterator i = m_passes.begin(); i != m_passes.end(); ++i)
    {
      delete *i;
    }
    m_passes.clear();
}

void OpenGLShader::addRenderable(const OpenGLRenderable& renderable, 
					   			 const Matrix4& modelview, 
					   			 const LightList* lights)
{
	// Iterate over the list of OpenGLStateBuckets, bumpmap and non-bumpmap
	// buckets are handled differently.
    for(Passes::iterator i = m_passes.begin(); i != m_passes.end(); ++i)
    {
      if(((*i)->state().m_state & RENDER_BUMP) != 0)
      {
        if(lights != 0)
        {
          OpenGLStateBucketAdd add(*(*i), renderable, modelview);
          lights->forEachLight(makeCallback1(add));
        }
      }
      else
      {
        (*i)->addRenderable(renderable, modelview);
      }
    }
}

void OpenGLShader::incrementUsed() {
    if(++m_used == 1 && m_shader != 0)
    { 
      m_shader->SetInUse(true);
    }
}

void OpenGLShader::decrementUsed() {
    if(--m_used == 0 && m_shader != 0)
    {
      m_shader->SetInUse(false);
    }
}

void OpenGLShader::realise(const std::string& name) {
    construct(name);

    if (m_shader != NULL) {
		// greebo: Check the filtersystem whether we're filtered
		m_shader->setVisible(GlobalFilterSystem().isVisible("texture", name));

		if (m_used != 0) {
			m_shader->SetInUse(true);
		}
    }
    
    for(Passes::iterator i = m_passes.begin(); i != m_passes.end(); ++i) {
    	render::getOpenGLShaderCache().insertSortedState(
			OpenGLStates::value_type(OpenGLStateReference((*i)->state()), 
									 *i));
    }

    m_observers.realise();
}

void OpenGLShader::unrealise() {
    m_observers.unrealise();

    for(Passes::iterator i = m_passes.begin(); i != m_passes.end(); ++i) {
    	render::getOpenGLShaderCache().eraseSortedState(
    		OpenGLStateReference((*i)->state())
    	);
    }

    destroy();
}

Texture& OpenGLShader::getTexture() const {
    return *(m_shader->getTexture());
}

unsigned int OpenGLShader::getFlags() const {
    return m_shader->getFlags();
}

// Append a default shader pass onto the back of the state list
OpenGLState& OpenGLShader::appendDefaultPass() {
    m_passes.push_back(new OpenGLShaderPass);
    OpenGLState& state = m_passes.back()->state();
    return state;
}

void OpenGLShader::construct(const std::string& name)
{
	// Retrieve the highlight colour from the colourschemes (once)
	static Vector3 highLightColour = ColourSchemes().getColour("selected_brush_camera");
	
  OpenGLState& state = appendDefaultPass();
  switch(name[0])
  {
  case '(': // fill shader
    sscanf(name.c_str(), "(%lf %lf %lf)", &state.m_colour[0], &state.m_colour[1], &state.m_colour[2]);
    state.m_colour[3] = 1.0f;
    state.m_state = RENDER_FILL|RENDER_LIGHTING|RENDER_DEPTHTEST|RENDER_CULLFACE|RENDER_COLOURWRITE|RENDER_DEPTHWRITE;
    state.m_sort = OpenGLState::eSortFullbright;
    break;

  case '[':
    sscanf(name.c_str(), "[%lf %lf %lf]", &state.m_colour[0], &state.m_colour[1], &state.m_colour[2]);
    state.m_colour[3] = 0.5f;
    state.m_state = RENDER_FILL|RENDER_LIGHTING|RENDER_DEPTHTEST|RENDER_CULLFACE|RENDER_COLOURWRITE|RENDER_DEPTHWRITE|RENDER_BLEND;
    state.m_sort = OpenGLState::eSortTranslucent;
    break;

  case '<': // wireframe shader
    sscanf(name.c_str(), "<%lf %lf %lf>", &state.m_colour[0], &state.m_colour[1], &state.m_colour[2]);
    state.m_colour[3] = 1;
    state.m_state = RENDER_DEPTHTEST|RENDER_COLOURWRITE|RENDER_DEPTHWRITE;
    state.m_sort = OpenGLState::eSortFullbright;
    state.m_depthfunc = GL_LESS;
    state.m_linewidth = 1;
    state.m_pointsize = 1;
    break;

	case '$':
		try {
      		state = GlobalOpenGLStateLibrary().find(name);
			break;
		}
		catch (std::runtime_error e) {
			// Not found, continue with this case
		}
    if (name == "$POINT")
    {
      state.m_state = RENDER_COLOURARRAY|RENDER_COLOURWRITE|RENDER_DEPTHWRITE;
      state.m_sort = OpenGLState::eSortControlFirst;
      state.m_pointsize = 4;
    }
    else if (name == "$SELPOINT")
    {
      state.m_state = RENDER_COLOURARRAY|RENDER_COLOURWRITE|RENDER_DEPTHWRITE;
      state.m_sort = OpenGLState::eSortControlFirst + 1;
      state.m_pointsize = 4;
    }
    else if (name == "$BIGPOINT")
    {
      state.m_state = RENDER_COLOURARRAY|RENDER_COLOURWRITE|RENDER_DEPTHWRITE;
      state.m_sort = OpenGLState::eSortControlFirst;
      state.m_pointsize = 6;
    }
    else if (name == "$PIVOT")
    {
      state.m_state = RENDER_COLOURARRAY|RENDER_COLOURWRITE|RENDER_DEPTHTEST|RENDER_DEPTHWRITE;
      state.m_sort = OpenGLState::eSortGUI1;
      state.m_linewidth = 2;
      state.m_depthfunc = GL_LEQUAL;

      OpenGLState& hiddenLine = appendDefaultPass();
      hiddenLine.m_state = RENDER_COLOURARRAY|RENDER_COLOURWRITE|RENDER_DEPTHTEST|RENDER_LINESTIPPLE;
      hiddenLine.m_sort = OpenGLState::eSortGUI0;
      hiddenLine.m_linewidth = 2;
      hiddenLine.m_depthfunc = GL_GREATER;
    }
    else if (name == "$LATTICE")
    {
      state.m_colour[0] = 1;
      state.m_colour[1] = 0.5;
      state.m_colour[2] = 0;
      state.m_colour[3] = 1;
      state.m_state = RENDER_COLOURWRITE|RENDER_DEPTHWRITE;
      state.m_sort = OpenGLState::eSortControlFirst;
    }
    else if (name == "$WIREFRAME")
    {
      state.m_state = RENDER_DEPTHTEST|RENDER_COLOURWRITE|RENDER_DEPTHWRITE;
      state.m_sort = OpenGLState::eSortFullbright;
    }
    else if (name == "$CAM_HIGHLIGHT")
    {
      state.m_colour[0] = highLightColour[0];
      state.m_colour[1] = highLightColour[1];
      state.m_colour[2] = highLightColour[2];
      state.m_colour[3] = 0.3f;
      state.m_state = RENDER_FILL|RENDER_DEPTHTEST|RENDER_CULLFACE|RENDER_BLEND|RENDER_COLOURWRITE|RENDER_DEPTHWRITE;
      state.m_sort = OpenGLState::eSortHighlight;
      state.m_depthfunc = GL_LEQUAL;
    }
    else if (name == "$CAM_OVERLAY")
    {
#if 0
      state.m_state = RENDER_CULLFACE|RENDER_COLOURWRITE|RENDER_DEPTHWRITE;
      state.m_sort = OpenGLState::eSortOverlayFirst;
#else
      state.m_state = RENDER_CULLFACE|RENDER_DEPTHTEST|RENDER_COLOURWRITE|RENDER_DEPTHWRITE|RENDER_OFFSETLINE;
      state.m_sort = OpenGLState::eSortOverlayFirst + 1;
      state.m_depthfunc = GL_LEQUAL;

      OpenGLState& hiddenLine = appendDefaultPass();
      hiddenLine.m_colour[0] = 0.75;
      hiddenLine.m_colour[1] = 0.75;
      hiddenLine.m_colour[2] = 0.75;
      hiddenLine.m_colour[3] = 1;
      hiddenLine.m_state = RENDER_CULLFACE|RENDER_DEPTHTEST|RENDER_COLOURWRITE|RENDER_OFFSETLINE|RENDER_LINESTIPPLE;
      hiddenLine.m_sort = OpenGLState::eSortOverlayFirst;
      hiddenLine.m_depthfunc = GL_GREATER;
      hiddenLine.m_linestipple_factor = 2;
#endif
    }
    else if (name == "$XY_OVERLAY")
    {
      Vector3 colorSelBrushes = ColourSchemes().getColour("selected_brush");
      state.m_colour[0] = colorSelBrushes[0];
      state.m_colour[1] = colorSelBrushes[1];
      state.m_colour[2] = colorSelBrushes[2];
      state.m_colour[3] = 1;
      state.m_state = RENDER_COLOURWRITE | RENDER_LINESTIPPLE;
      state.m_sort = OpenGLState::eSortOverlayFirst;
      state.m_linewidth = 2;
      state.m_linestipple_factor = 3;
    }
    else if (name == "$DEBUG_CLIPPED")
    {
      state.m_state = RENDER_COLOURARRAY | RENDER_COLOURWRITE | RENDER_DEPTHWRITE;
      state.m_sort = OpenGLState::eSortLast;
    }
    else if (name == "$POINTFILE")
    {
      state.m_colour[0] = 1;
      state.m_colour[1] = 0;
      state.m_colour[2] = 0;
      state.m_colour[3] = 1;
      state.m_state = RENDER_DEPTHTEST | RENDER_COLOURWRITE | RENDER_DEPTHWRITE;
      state.m_sort = OpenGLState::eSortFullbright;
      state.m_linewidth = 4;
    }
    else if (name == "$LIGHT_SPHERE")
    {
      state.m_colour[0] = .15f * .95f;
      state.m_colour[1] = .15f * .95f;
      state.m_colour[2] = .15f * .95f;
      state.m_colour[3] = 1;
      state.m_state = RENDER_CULLFACE | RENDER_DEPTHTEST | RENDER_BLEND | RENDER_FILL | RENDER_COLOURWRITE | RENDER_DEPTHWRITE;
      state.m_blend_src = GL_ONE;
      state.m_blend_dst = GL_ONE;
      state.m_sort = OpenGLState::eSortTranslucent;
    }
    else if (name == "$Q3MAP2_LIGHT_SPHERE")
    {
      state.m_colour[0] = .05f;
      state.m_colour[1] = .05f;
      state.m_colour[2] = .05f;
      state.m_colour[3] = 1;
      state.m_state = RENDER_CULLFACE | RENDER_DEPTHTEST | RENDER_BLEND | RENDER_FILL;
      state.m_blend_src = GL_ONE;
      state.m_blend_dst = GL_ONE;
      state.m_sort = OpenGLState::eSortTranslucent;
    }
    else if (name == "$WIRE_OVERLAY")
    {
#if 0
      state.m_state = RENDER_COLOURARRAY | RENDER_COLOURWRITE | RENDER_DEPTHWRITE | RENDER_DEPTHTEST | RENDER_OVERRIDE;
      state.m_sort = OpenGLState::eSortOverlayFirst;
#else
      state.m_state = RENDER_COLOURARRAY | RENDER_COLOURWRITE | RENDER_DEPTHWRITE | RENDER_DEPTHTEST | RENDER_OVERRIDE;
      state.m_sort = OpenGLState::eSortGUI1;
      state.m_depthfunc = GL_LEQUAL;

      OpenGLState& hiddenLine = appendDefaultPass();
      hiddenLine.m_state = RENDER_COLOURARRAY | RENDER_COLOURWRITE | RENDER_DEPTHWRITE | RENDER_DEPTHTEST | RENDER_OVERRIDE | RENDER_LINESTIPPLE;
      hiddenLine.m_sort = OpenGLState::eSortGUI0;
      hiddenLine.m_depthfunc = GL_GREATER;
#endif
    }
    else if (name == "$FLATSHADE_OVERLAY")
    {
      state.m_state = RENDER_CULLFACE | RENDER_LIGHTING | RENDER_SMOOTH | RENDER_SCALED | RENDER_COLOURARRAY | RENDER_FILL | RENDER_COLOURWRITE | RENDER_DEPTHWRITE | RENDER_DEPTHTEST | RENDER_OVERRIDE;
      state.m_sort = OpenGLState::eSortGUI1;
      state.m_depthfunc = GL_LEQUAL;

      OpenGLState& hiddenLine = appendDefaultPass();
      hiddenLine.m_state = RENDER_CULLFACE | RENDER_LIGHTING | RENDER_SMOOTH | RENDER_SCALED | RENDER_COLOURARRAY | RENDER_FILL | RENDER_COLOURWRITE | RENDER_DEPTHWRITE | RENDER_DEPTHTEST | RENDER_OVERRIDE | RENDER_POLYGONSTIPPLE;
      hiddenLine.m_sort = OpenGLState::eSortGUI0;
      hiddenLine.m_depthfunc = GL_GREATER;
    }
    else if (name == "$CLIPPER_OVERLAY")
    {
      Vector3 colorClipper = ColourSchemes().getColour("clipper");
      state.m_colour[0] = colorClipper[0];
      state.m_colour[1] = colorClipper[1];
      state.m_colour[2] = colorClipper[2];
      state.m_colour[3] = 1;
      state.m_state = RENDER_CULLFACE | RENDER_COLOURWRITE | RENDER_DEPTHWRITE | RENDER_FILL | RENDER_POLYGONSTIPPLE;
      state.m_sort = OpenGLState::eSortOverlayFirst;
    }
    else if (name == "$OVERBRIGHT")
    {
      const float lightScale = 2;
      state.m_colour[0] = lightScale * 0.5f;
      state.m_colour[1] = lightScale * 0.5f;
      state.m_colour[2] = lightScale * 0.5f;
      state.m_colour[3] = 0.5;
      state.m_state = RENDER_FILL|RENDER_BLEND|RENDER_COLOURWRITE|RENDER_SCREEN;
      state.m_sort = OpenGLState::eSortOverbrighten;
      state.m_blend_src = GL_DST_COLOR;
      state.m_blend_dst = GL_SRC_COLOR;
    }
    else
    {
      // default to something recognisable.. =)
      ERROR_MESSAGE("hardcoded renderstate not found");
      state.m_colour[0] = 1;
      state.m_colour[1] = 0;
      state.m_colour[2] = 1;
      state.m_colour[3] = 1;
      state.m_state = RENDER_COLOURWRITE | RENDER_DEPTHWRITE;
      state.m_sort = OpenGLState::eSortFirst;
    }
    break;
  default:
    // construction from IShader
    m_shader = QERApp_Shader_ForName(name);

    if (GlobalShaderCache().lightingSupported() 
    	&& GlobalShaderCache().lightingEnabled() 
    	&& m_shader->getBump() != 0 
    	&& m_shader->getBump()->texture_number != 0) // is a bump shader
    {
      state.m_state = RENDER_FILL | RENDER_CULLFACE | RENDER_TEXTURE | RENDER_DEPTHTEST | RENDER_DEPTHWRITE | RENDER_COLOURWRITE | RENDER_PROGRAM;
      state.m_colour[0] = 0;
      state.m_colour[1] = 0;
      state.m_colour[2] = 0;
      state.m_colour[3] = 1;
      state.m_sort = OpenGLState::eSortOpaque;

      state.m_program = render::GLProgramFactory::getProgram("depthFill").get();

      OpenGLState& bumpPass = appendDefaultPass();
      bumpPass.m_texture = m_shader->getDiffuse()->texture_number;
      bumpPass.m_texture1 = m_shader->getBump()->texture_number;
      bumpPass.m_texture2 = m_shader->getSpecular()->texture_number;

      bumpPass.m_state = RENDER_BLEND|RENDER_FILL|RENDER_CULLFACE|RENDER_DEPTHTEST|RENDER_COLOURWRITE|RENDER_SMOOTH|RENDER_BUMP|RENDER_PROGRAM;

      bumpPass.m_program = render::GLProgramFactory::getProgram("bumpMap").get();

      bumpPass.m_depthfunc = GL_LEQUAL;
      bumpPass.m_sort = OpenGLState::eSortMultiFirst;
      bumpPass.m_blend_src = GL_ONE;
      bumpPass.m_blend_dst = GL_ONE;
    }
    else
    {
      state.m_texture = m_shader->getTexture()->texture_number;

      state.m_state = RENDER_FILL|RENDER_TEXTURE|RENDER_DEPTHTEST|RENDER_COLOURWRITE|RENDER_LIGHTING|RENDER_SMOOTH;
      if((m_shader->getFlags() & QER_CULL) != 0)
      {
        if(m_shader->getCull() == IShader::eCullBack)
        {
          state.m_state |= RENDER_CULLFACE;
        }
      }
      else
      {
        state.m_state |= RENDER_CULLFACE;
      }
      if((m_shader->getFlags() & QER_ALPHATEST) != 0)
      {
        state.m_state |= RENDER_ALPHATEST;
        IShader::EAlphaFunc alphafunc;
        m_shader->getAlphaFunc(&alphafunc, &state.m_alpharef);
        switch(alphafunc)
        {
        case IShader::eAlways:
          state.m_alphafunc = GL_ALWAYS;
        case IShader::eEqual:
          state.m_alphafunc = GL_EQUAL;
        case IShader::eLess:
          state.m_alphafunc = GL_LESS;
        case IShader::eGreater:
          state.m_alphafunc = GL_GREATER;
        case IShader::eLEqual:
          state.m_alphafunc = GL_LEQUAL;
        case IShader::eGEqual:
          state.m_alphafunc = GL_GEQUAL;
        }
      }
      reinterpret_cast<Vector3&>(state.m_colour) = m_shader->getTexture()->color;
      state.m_colour[3] = 1.0f;
      
      if((m_shader->getFlags() & QER_TRANS) != 0)
      {
        state.m_state |= RENDER_BLEND;
        state.m_colour[3] = m_shader->getTrans();
        state.m_sort = OpenGLState::eSortTranslucent;
        BlendFunc blendFunc = m_shader->getBlendFunc();
        state.m_blend_src = convertBlendFactor(blendFunc.m_src);
        state.m_blend_dst = convertBlendFactor(blendFunc.m_dst);
        if(state.m_blend_src == GL_SRC_ALPHA || state.m_blend_dst == GL_SRC_ALPHA)
        {
          state.m_state |= RENDER_DEPTHWRITE;
        }
      }
      else
      {
        state.m_state |= RENDER_DEPTHWRITE;
        state.m_sort = OpenGLState::eSortFullbright;
      }
    }
  }
}
