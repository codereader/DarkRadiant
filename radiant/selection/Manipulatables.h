#ifndef MANIPULATABLES_H_
#define MANIPULATABLES_H_

#include "iscenegraph.h"
#include "math/Vector3.h"
#include "math/Matrix4.h"
#include "math/Quaternion.h"

#include "render.h"

#include "TransformationVisitors.h"
#include "ManipulatorComponent.h"
#include "Translatable.h"
#include "Rotatable.h"
#include "Scalable.h"

/* greebo: The following are specialised manipulatables that provide the methods as described in the ABC.
 * They basically prepare and constraing the transformations of the three base movements above (Translatable, etc.)
 *
 * So, for example, the TranslateAxis class takes care that the Translatable is only moved in the axis directions.
 * The necessary device pointer >> translation vector calculations are performed within Transform()
 */

class RotateFree : public ManipulatorComponent {
  Vector3 _start;
  Rotatable& _rotatable;
public:
  RotateFree(Rotatable& rotatable): _rotatable(rotatable) {}
  void Construct(const Matrix4& device2manip, const float x, const float y);
  void Transform(const Matrix4& manip2object, const Matrix4& device2manip, const float x, const float y);
};

class RotateAxis : public ManipulatorComponent {
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

class TranslateAxis : public ManipulatorComponent {
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

class TranslateFree : public ManipulatorComponent {
private:
  Vector3 _start;
  Translatable& _translatable;
public:
  TranslateFree(Translatable& translatable): _translatable(translatable) {}
  void Construct(const Matrix4& device2manip, const float x, const float y);
  void Transform(const Matrix4& manip2object, const Matrix4& device2manip, const float x, const float y);
};


class ScaleAxis : public ManipulatorComponent {
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

class ScaleFree : public ManipulatorComponent {
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
