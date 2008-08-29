#ifndef OPENGLSTATEBUCKETADD_H_
#define OPENGLSTATEBUCKETADD_H_

#include "OpenGLShaderPass.h"

/**
 * Functor class which populates an OpenGLShaderPass.
 */
class OpenGLStateBucketAdd
{
  OpenGLShaderPass& m_bucket;
  const OpenGLRenderable& m_renderable;
  const Matrix4& m_modelview;
public:
  typedef const RendererLight& first_argument_type;

  OpenGLStateBucketAdd(OpenGLShaderPass& bucket, const OpenGLRenderable& renderable, const Matrix4& modelview) :
    m_bucket(bucket), m_renderable(renderable), m_modelview(modelview)
  {
  }
  void operator()(const RendererLight& light)
  {
    m_bucket.addRenderable(m_renderable, m_modelview, &light);
  }
};

#endif /*OPENGLSTATEBUCKETADD_H_*/
