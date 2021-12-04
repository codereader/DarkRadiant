#pragma once

#include <functional>
#include <map>
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
    SelectionMode _selectionMode;

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

    SelectionMode getSelectionMode() const override;
    void setSelectionMode(SelectionMode mode) override;
    void toggleSelectionMode(SelectionMode mode) override;
    sigc::signal<void, SelectionMode>& signal_selectionModeChanged() override;

    void foreachSelectedNode(const std::function<bool(const INode::Ptr&)>& functor) override;
    void foreachSelectedComponentNode(const std::function<bool(const INode::Ptr&)>& functor) override;

    std::size_t countSelected() override;
    std::size_t countSelectedComponentNodes() override;

    sigc::signal<void>& signal_selectionChanged() override;

    void clearSelection() override;
    void clearComponentSelection() override;

    void selectPoint(SelectionTest& test, selection::SelectionSystem::EModifier modifier) override;
    void selectArea(SelectionTest& test, selection::SelectionSystem::EModifier modifier) override;

    // Returns the ID of the registered manipulator
    std::size_t registerManipulator(const selection::ITextureToolManipulator::Ptr& manipulator) override;
    void unregisterManipulator(const selection::ITextureToolManipulator::Ptr& manipulator) override;

    selection::IManipulator::Type getActiveManipulatorType() override;
    const selection::ITextureToolManipulator::Ptr& getActiveManipulator() override;
    void setActiveManipulator(std::size_t manipulatorId) override;
    void setActiveManipulator(selection::IManipulator::Type manipulatorType) override;
    void toggleManipulatorMode(selection::IManipulator::Type manipulatorType) override;

    sigc::signal<void, selection::IManipulator::Type>& signal_activeManipulatorChanged() override;

    Matrix4 getPivot2World() override;
    void onManipulationStart() override;
    void onManipulationChanged() override;
    void onManipulationFinished() override;
    void onManipulationCancelled() override;

    void onNodeSelectionChanged(ISelectable& selectable) override;
    void onComponentSelectionChanged(ISelectable& selectable) override;

private:
    void handleUnselectRequest(selection::UnselectSelectionRequest& request);

    // Internally switches between the selection modes and iterates over the corresponding collection
    void foreachSelectedNodeOfAnyType(const std::function<bool(const INode::Ptr&)>& functor);

    void toggleManipulatorModeCmd(const cmd::ArgumentList& args);
    void toggleManipulatorModeById(std::size_t manipId);
    std::size_t getManipulatorIdForType(selection::IManipulator::Type type);

    void toggleSelectionModeCmd(const cmd::ArgumentList& args);
    void selectRelatedCmd(const cmd::ArgumentList& args);
    void snapSelectionToGridCmd(const cmd::ArgumentList& args);
    void mergeSelectionCmd(const cmd::ArgumentList& args);
    void flipHorizontallyCmd(const cmd::ArgumentList& args);
    void flipVerticallyCmd(const cmd::ArgumentList& args);
    void normaliseSelectionCmd(const cmd::ArgumentList& args);
    void shiftSelectionCmd(const cmd::ArgumentList& args);
    void scaleSelectionCmd(const cmd::ArgumentList& args);
    void rotateSelectionCmd(const cmd::ArgumentList& args);

    void flipSelected(int axis);
    void performSelectionTest(Selector& selector, SelectionTest& test);
};

}
