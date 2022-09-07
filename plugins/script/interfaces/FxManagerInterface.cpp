#include "FxManagerInterface.h"

namespace script
{

ScriptFxDeclaration FxManagerInterface::findFx(const std::string& name)
{
    return ScriptFxDeclaration(GlobalFxManager().findFx(name));
}

void FxManagerInterface::registerInterface(py::module& scope, py::dict& globals) 
{
    py::class_<ScriptFxAction> fxAction(scope, "FxAction");

    // FxAction::Type
    py::enum_<fx::IFxAction::Type>(fxAction, "Type")
        .value("Undefined", fx::IFxAction::Type::Undefined)
        .value("Light", fx::IFxAction::Type::Light)
        .value("Particle", fx::IFxAction::Type::Particle)
        .value("Decal", fx::IFxAction::Type::Decal)
        .value("Model", fx::IFxAction::Type::Model)
        .value("Sound", fx::IFxAction::Type::Sound)
        .value("Shake", fx::IFxAction::Type::Shake)
        .value("AttachLight", fx::IFxAction::Type::AttachLight)
        .value("AttachEntity", fx::IFxAction::Type::AttachEntity)
        .value("Launch", fx::IFxAction::Type::Launch)
        .value("Shockwave", fx::IFxAction::Type::Shockwave)
        .export_values();

    // Add the Fx Action methods
    fxAction.def(py::init<const fx::IFxAction::Ptr&>())
        .def("getActionType", &ScriptFxAction::getActionType)
        .def("getName", &ScriptFxAction::getName)
        .def("getDelayInSeconds", &ScriptFxAction::getDelayInSeconds)
        .def("getDurationInSeconds", &ScriptFxAction::getDurationInSeconds)
        .def("getIgnoreMaster", &ScriptFxAction::getIgnoreMaster)
        .def("getShakeTimeInSeconds", &ScriptFxAction::getShakeTimeInSeconds)
        .def("getShakeAmplitude", &ScriptFxAction::getShakeAmplitude)
        .def("getShakeDistance", &ScriptFxAction::getShakeDistance)
        .def("getShakeFalloff", &ScriptFxAction::getShakeFalloff)
        .def("getShakeImpulse", &ScriptFxAction::getShakeImpulse)
        .def("getNoShadows", &ScriptFxAction::getNoShadows)
        .def("getFireSiblingAction", &ScriptFxAction::getFireSiblingAction)
        .def("getRandomDelay", &ScriptFxAction::getRandomDelay)
        .def("getRotate", &ScriptFxAction::getRotate)
        .def("getTrackOrigin", &ScriptFxAction::getTrackOrigin)
        .def("getRestart", &ScriptFxAction::getRestart)
        .def("getFadeInTimeInSeconds", &ScriptFxAction::getFadeInTimeInSeconds)
        .def("getFadeOutTimeInSeconds", &ScriptFxAction::getFadeOutTimeInSeconds)
        .def("getDecalSize", &ScriptFxAction::getDecalSize)
        .def("getOffset", &ScriptFxAction::getOffset)
        .def("getAxis", &ScriptFxAction::getAxis)
        .def("getAngle", &ScriptFxAction::getAngle)
        .def("getUseLight", &ScriptFxAction::getUseLight)
        .def("getUseModel", &ScriptFxAction::getUseModel)
        .def("getAttachLight", &ScriptFxAction::getAttachLight)
        .def("getAttachEntity", &ScriptFxAction::getAttachEntity)
        .def("getLaunchProjectileDef", &ScriptFxAction::getLaunchProjectileDef)
        .def("getLightMaterialName", &ScriptFxAction::getLightMaterialName)
        .def("getLightRgbColour", &ScriptFxAction::getLightRgbColour)
        .def("getLightRadius", &ScriptFxAction::getLightRadius)
        .def("getModelName", &ScriptFxAction::getModelName)
        .def("getDecalMaterialName", &ScriptFxAction::getDecalMaterialName)
        .def("getParticleTrackVelocity", &ScriptFxAction::getParticleTrackVelocity)
        .def("getSoundShaderName", &ScriptFxAction::getSoundShaderName)
        .def("getShockwaveDefName", &ScriptFxAction::getShockwaveDefName);

	// Add the Fx Declaration interface
	py::class_<ScriptFxDeclaration, ScriptDeclaration> fxDeclaration(scope, "Fx");
    fxDeclaration.def(py::init<const fx::IFxDeclaration::Ptr&>());
    fxDeclaration.def("isNull", &ScriptFxDeclaration::isNull);
    fxDeclaration.def("getBindTo", &ScriptFxDeclaration::getBindTo);
    fxDeclaration.def("getNumActions", &ScriptFxDeclaration::getNumActions);
    fxDeclaration.def("getAction", &ScriptFxDeclaration::getAction);

	// Add the FxManager module declaration to the given python namespace
	py::class_<FxManagerInterface> fxManager(scope, "FxManager");

	// Add both overloads to createEntity
    fxManager.def("findFx", &FxManagerInterface::findFx);

	// Now point the Python variable "GlobalFxManager" to this instance
	globals["GlobalFxManager"] = this;
}

}
