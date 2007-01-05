#ifndef MANIPULATABLES_H_
#define MANIPULATABLES_H_

#include "math/Vector3.h"
#include "math/matrix.h"
#include "math/quaternion.h"

#include "render.h"

#include "TransformationVisitors.h"

struct FlatShadedVertex {
  Vertex3f vertex;
  Colour4b colour;
  Normal3f normal;

  FlatShadedVertex() {}
};

// greebo: These three are needed within a manipulatable. For example, a TranslateAxis manipulatable
// has a member variable that is Translatable. Upon Transform() the member is translated.

// ================== Abstract Base Classes =================================

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

// A manipulatable object, as the name states.
class Manipulatable {
  public:
  	virtual void Construct(const Matrix4& device2manip, const float x, const float y) = 0;
  	
  	// greebo: An abstract Transform() method, the implementation has to decide which operations 
  	// are actually called. This may be a translation, rotation, or anything else.
  	virtual void Transform(const Matrix4& manip2object, const Matrix4& device2manip, const float x, const float y) = 0;
};

// ==========================================================================

/* greebo: The following are specialised manipulatables that provide the methods as described in the ABC.
 * They basically prepare and constraing the transformations of the three base movements above (Translatable, etc.)
 * 
 * So, for example, the TranslateAxis class takes care that the Translatable is only moved in the axis directions.
 * The necessary device pointer >> translation vector calculations are performed within Transform()  
 */

class RotateFree : public Manipulatable {
  Vector3 _start;
  Rotatable& _rotatable;
public:
  RotateFree(Rotatable& rotatable): _rotatable(rotatable) {}
  void Construct(const Matrix4& device2manip, const float x, const float y);
  void Transform(const Matrix4& manip2object, const Matrix4& device2manip, const float x, const float y);
};

class RotateAxis : public Manipulatable {
  Vector3 _axis;
  Vector3 _start;
  Rotatable& _rotatable;
public:
  RotateAxis(Rotatable& rotatable): _rotatable(rotatable) {}
  void Construct(const Matrix4& device2manip, const float x, const float y);
  
  /// \brief Converts current position to a normalised vector orthogonal to axis.
  void Transform(const Matrix4& manip2object, const Matrix4& device2manip, const float x, const float y);
  
  void SetAxis(const Vector3& axis) {
    _axis = axis;
  }
};

class TranslateAxis : public Manipulatable {
  Vector3 _start;
  Vector3 _axis;
  Translatable& _translatable;
public:
  TranslateAxis(Translatable& translatable): _translatable(translatable) {}
  void Construct(const Matrix4& device2manip, const float x, const float y);  
  void Transform(const Matrix4& manip2object, const Matrix4& device2manip, const float x, const float y);
  
  void SetAxis(const Vector3& axis) {
    _axis = axis;
  }
};

class TranslateFree : public Manipulatable {
private:
  Vector3 _start;
  Translatable& _translatable;
public:
  TranslateFree(Translatable& translatable): _translatable(translatable) {}
  void Construct(const Matrix4& device2manip, const float x, const float y);
  void Transform(const Matrix4& manip2object, const Matrix4& device2manip, const float x, const float y);
};


class ScaleAxis : public Manipulatable {
private:
  Vector3 _start;
  Vector3 _axis;
  Scalable& _scalable;
public:
  ScaleAxis(Scalable& scalable): _scalable(scalable) {}
  void Construct(const Matrix4& device2manip, const float x, const float y);
  void Transform(const Matrix4& manip2object, const Matrix4& device2manip, const float x, const float y);
  
  void SetAxis(const Vector3& axis) {
    _axis = axis;
  }
};

class ScaleFree : public Manipulatable {
private:
  Vector3 _start;
  Scalable& _scalable;
public:
  ScaleFree(Scalable& scalable): _scalable(scalable) {}
  void Construct(const Matrix4& device2manip, const float x, const float y);
  void Transform(const Matrix4& manip2object, const Matrix4& device2manip, const float x, const float y);
};

// ========= Translatables ===============================================

class ResizeTranslatable : public Translatable {
  void translate(const Vector3& translation) {
    Scene_Translate_Component_Selected(GlobalSceneGraph(), translation);
  }
};

class DragTranslatable : public Translatable {
  void translate(const Vector3& translation) {
    if(GlobalSelectionSystem().Mode() == SelectionSystem::eComponent) {
      Scene_Translate_Component_Selected(GlobalSceneGraph(), translation);
    }
    else {
      Scene_Translate_Selected(GlobalSceneGraph(), translation);
    }
  }
};

// ===============================================================================

void transform_local2object(Matrix4& object, const Matrix4& local, const Matrix4& local2object);
void translation_local2object(Vector3& object, const Vector3& local, const Matrix4& local2object);


#endif /*MANIPULATABLES_H_*/
