#ifndef MANIPULATABLES_H_
#define MANIPULATABLES_H_

#include "generic/vector.h"
#include "render.h"
#include "math/matrix.h"
#include "math/quaternion.h"
#include "grid.h"

struct FlatShadedVertex {
  Vertex3f vertex;
  Colour4b colour;
  Normal3f normal;

  FlatShadedVertex() {}
};

// ================== Abstract Base Classes =================================

class Manipulatable {
  public:
  	virtual void Construct(const Matrix4& device2manip, const float x, const float y) = 0;
  	virtual void Transform(const Matrix4& manip2object, const Matrix4& device2manip, const float x, const float y) = 0;
};

class Rotatable {
  public:
  	virtual void rotate(const Quaternion& rotation) = 0;
};

class Translatable {
  public:
  	virtual void translate(const Vector3& translation) = 0;
};

class Scalable {
  public:
  	virtual void scale(const Vector3& scaling) = 0;
};

// ==========================================================================

class RotateFree : public Manipulatable
{
  Vector3 m_start;
  Rotatable& m_rotatable;
public:
  RotateFree(Rotatable& rotatable): m_rotatable(rotatable) {}
  void Construct(const Matrix4& device2manip, const float x, const float y);
  void Transform(const Matrix4& manip2object, const Matrix4& device2manip, const float x, const float y);
};

class RotateAxis : public Manipulatable
{
  Vector3 m_axis;
  Vector3 m_start;
  Rotatable& m_rotatable;
public:
  RotateAxis(Rotatable& rotatable): m_rotatable(rotatable) {}
  void Construct(const Matrix4& device2manip, const float x, const float y);
  
  /// \brief Converts current position to a normalised vector orthogonal to axis.
  void Transform(const Matrix4& manip2object, const Matrix4& device2manip, const float x, const float y);
  
  void SetAxis(const Vector3& axis) {
    m_axis = axis;
  }
};

class TranslateAxis : public Manipulatable
{
  Vector3 m_start;
  Vector3 m_axis;
  Translatable& m_translatable;
public:
  TranslateAxis(Translatable& translatable): m_translatable(translatable) {}
  void Construct(const Matrix4& device2manip, const float x, const float y);  
  void Transform(const Matrix4& manip2object, const Matrix4& device2manip, const float x, const float y);
  
  void SetAxis(const Vector3& axis) {
    m_axis = axis;
  }
};

class TranslateFree : public Manipulatable
{
private:
  Vector3 m_start;
  Translatable& m_translatable;
public:
  TranslateFree(Translatable& translatable): m_translatable(translatable) {}
  void Construct(const Matrix4& device2manip, const float x, const float y);
  void Transform(const Matrix4& manip2object, const Matrix4& device2manip, const float x, const float y);
};


class ScaleAxis : public Manipulatable
{
private:
  Vector3 m_start;
  Vector3 m_axis;
  Scalable& m_scalable;
public:
  ScaleAxis(Scalable& scalable): m_scalable(scalable) {}
  void Construct(const Matrix4& device2manip, const float x, const float y);
  void Transform(const Matrix4& manip2object, const Matrix4& device2manip, const float x, const float y);
  
  void SetAxis(const Vector3& axis) {
    m_axis = axis;
  }
};

class ScaleFree : public Manipulatable
{
private:
  Vector3 m_start;
  Scalable& m_scalable;
public:
  ScaleFree(Scalable& scalable): m_scalable(scalable) {}
  void Construct(const Matrix4& device2manip, const float x, const float y);
  void Transform(const Matrix4& manip2object, const Matrix4& device2manip, const float x, const float y);
};

void transform_local2object(Matrix4& object, const Matrix4& local, const Matrix4& local2object);
void translation_local2object(Vector3& object, const Vector3& local, const Matrix4& local2object);


#endif /*MANIPULATABLES_H_*/
