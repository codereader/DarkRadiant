#pragma once

#include "itexturetoolmodel.h"

namespace textool
{

class TextureToolSelectionSystem :
    public ITextureToolSelectionSystem
{
private:
    std::map<std::size_t, selection::ITextureToolManipulator::Ptr> _manipulators;

    // The currently active manipulator
    selection::ITextureToolManipulator::Ptr _activeManipulator;
    selection::IManipulator::Type _defaultManipulatorType;

    sigc::signal<void, selection::IManipulator::Type> _sigActiveManipulatorChanged;
    Matrix4 _pivot2World;

public:
    const std::string& getName() const override;
    const StringSet& getDependencies() const override;
    void initialiseModule(const IApplicationContext& ctx) override;
    void shutdownModule() override;

    void foreachSelectedNode(const std::function<bool(const INode::Ptr&)>& functor) override;

    // Returns the ID of the registered manipulator
    std::size_t registerManipulator(const selection::ITextureToolManipulator::Ptr& manipulator) override;
    void unregisterManipulator(const selection::ITextureToolManipulator::Ptr& manipulator) override;

    selection::IManipulator::Type getActiveManipulatorType() override;
    const selection::ITextureToolManipulator::Ptr& getActiveManipulator() override;
    void setActiveManipulator(std::size_t manipulatorId) override;
    void setActiveManipulator(selection::IManipulator::Type manipulatorType) override;

    sigc::signal<void, selection::IManipulator::Type>& signal_activeManipulatorChanged() override;

    Matrix4 getPivot2World() override;
    void onManipulationStart() override;
    void onManipulationChanged() override;
    void onManipulationFinished() override;
    void onManipulationCancelled() override;
};

}