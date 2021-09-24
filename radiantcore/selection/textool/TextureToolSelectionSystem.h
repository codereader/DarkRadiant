#pragma once

#include <functional>
#include "itexturetoolmodel.h"
#include "icommandsystem.h"
#include "TextureToolManipulationPivot.h"
#include "messages/UnselectSelectionRequest.h"

namespace textool
{

class TextureToolSelectionSystem :
    public ITextureToolSelectionSystem
{
private:
    SelectionMode _mode;

    std::map<std::size_t, selection::ITextureToolManipulator::Ptr> _manipulators;

    // The currently active manipulator
    selection::ITextureToolManipulator::Ptr _activeManipulator;
    selection::IManipulator::Type _defaultManipulatorType;

    sigc::signal<void, selection::IManipulator::Type> _sigActiveManipulatorChanged;
    sigc::signal<void, SelectionMode> _sigSelectionModeChanged;

    sigc::signal<void> _sigSelectionChanged;

    TextureToolManipulationPivot _manipulationPivot;

    std::size_t _unselectListener;

public:
    const std::string& getName() const override;
    const StringSet& getDependencies() const override;
    void initialiseModule(const IApplicationContext& ctx) override;
    void shutdownModule() override;

    SelectionMode getMode() const override;
    void setMode(SelectionMode mode) override;
    sigc::signal<void, SelectionMode>& signal_selectionModeChanged() override;

    void foreachSelectedNode(const std::function<bool(const INode::Ptr&)>& functor) override;
    void foreachSelectedComponentNode(const std::function<bool(const INode::Ptr&)>& functor) override;

    std::size_t countSelected() override;
    std::size_t countSelectedComponentNodes() override;

    sigc::signal<void>& signal_selectionChanged() override;

    void clearSelection() override;
    void clearComponentSelection() override;

    void selectPoint(SelectionTest& test, SelectionSystem::EModifier modifier) override;
    void selectArea(SelectionTest& test, SelectionSystem::EModifier modifier) override;

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

    void onNodeSelectionChanged(ISelectable& selectable) override;

private:
    void handleUnselectRequest(selection::UnselectSelectionRequest& request);

    // Internally switches between the selection modes and iterates over the corresponding collection
    void foreachSelectedNodeOfAnyType(const std::function<bool(const INode::Ptr&)>& functor);

    void toggleManipulatorModeCmd(const cmd::ArgumentList& args);
    void toggleManipulatorModeById(std::size_t manipId);
    std::size_t getManipulatorIdForType(selection::IManipulator::Type type);

    void toggleSelectionMode(SelectionMode mode);
    void toggleSelectionModeCmd(const cmd::ArgumentList& args);

    void performSelectionTest(Selector& selector, SelectionTest& test);
};

}
