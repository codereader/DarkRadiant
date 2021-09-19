#include "TextureToolSelectionSystem.h"

#include "itextstream.h"
#include "module/StaticModule.h"
#include "../textool/TextureToolRotateManipulator.h"
#include "../textool/TextureToolDragManipulator.h"
#include "selection/SelectionPool.h"
#include "string/case_conv.h"

namespace textool
{

const std::string& TextureToolSelectionSystem::getName() const
{
    static std::string _name(MODULE_TEXTOOL_SELECTIONSYSTEM);
    return _name;
}

const StringSet& TextureToolSelectionSystem::getDependencies() const
{
    static StringSet _dependencies{ MODULE_TEXTOOL_SCENEGRAPH, MODULE_COMMANDSYSTEM };
    return _dependencies;
}

void TextureToolSelectionSystem::initialiseModule(const IApplicationContext& ctx)
{
    rMessage() << getName() << "::initialiseModule called." << std::endl;

    _mode = SelectionMode::Surface;

    _manipulationPivot.setFromMatrix(Matrix4::getIdentity());
    registerManipulator(std::make_shared<TextureToolRotateManipulator>(_manipulationPivot));
    registerManipulator(std::make_shared<TextureToolDragManipulator>());

    _defaultManipulatorType = selection::IManipulator::Drag;
    setActiveManipulator(_defaultManipulatorType);

    GlobalCommandSystem().addCommand("ToggleTextureToolManipulatorMode",
        std::bind(&TextureToolSelectionSystem::toggleManipulatorModeCmd, this, std::placeholders::_1),
        { cmd::ARGTYPE_STRING });
    GlobalCommandSystem().addCommand("ToggleTextureToolSelectionMode",
        std::bind(&TextureToolSelectionSystem::toggleSelectionModeCmd, this, std::placeholders::_1),
        { cmd::ARGTYPE_STRING });
}

void TextureToolSelectionSystem::shutdownModule()
{
    _sigActiveManipulatorChanged.clear();
    _manipulators.clear();
}

SelectionMode TextureToolSelectionSystem::getMode() const
{
    return _mode;
}

void TextureToolSelectionSystem::setMode(SelectionMode mode)
{
    if (mode != _mode)
    {
        _mode = mode;
        _sigSelectionModeChanged.emit(_mode);
    }
}

void TextureToolSelectionSystem::toggleSelectionMode(SelectionMode mode)
{
    // Switch back to Surface mode if toggling a non-default mode again
    if (mode == _mode && mode != SelectionMode::Surface)
    {
        // Toggle back to Surface mode
        toggleSelectionMode(SelectionMode::Surface);
    }
    else
    {
        // setMode will only do something if we're not already in the target mode
        setMode(mode);
    }
}

sigc::signal<void, SelectionMode>& TextureToolSelectionSystem::signal_selectionModeChanged()
{
    return _sigSelectionModeChanged;
}

void TextureToolSelectionSystem::toggleManipulatorModeCmd(const cmd::ArgumentList& args)
{
    if (args.size() != 1)
    {
        rWarning() << "Usage: ToggleTextureToolManipulatorMode <manipulator>" << std::endl;
        rWarning() << " with <manipulator> being one of the following: " << std::endl;
        rWarning() << "      Drag" << std::endl;
        rWarning() << "      Rotate" << std::endl;
        return;
    }

    auto manip = string::to_lower_copy(args[0].getString());

    if (manip == "drag")
    {
        toggleManipulatorModeById(getManipulatorIdForType(selection::IManipulator::Drag));
    }
    else if (manip == "rotate")
    {
        toggleManipulatorModeById(getManipulatorIdForType(selection::IManipulator::Rotate));
    }
}

void TextureToolSelectionSystem::toggleSelectionModeCmd(const cmd::ArgumentList& args)
{
    if (args.size() != 1)
    {
        rWarning() << "Usage: ToggleTextureToolSelectionMode <mode>" << std::endl;
        rWarning() << " with <mode> being one of the following: " << std::endl;
        rWarning() << "      Surface" << std::endl;
        rWarning() << "      Vertex" << std::endl;
        return;
    }

    auto manip = string::to_lower_copy(args[0].getString());

    if (manip == "surface")
    {
        toggleSelectionMode(SelectionMode::Surface);
    }
    else if (manip == "vertex")
    {
        toggleSelectionMode(SelectionMode::Vertex);
    }
}

void TextureToolSelectionSystem::foreachSelectedNode(const std::function<bool(const INode::Ptr&)>& functor)
{
    GlobalTextureToolSceneGraph().foreachNode([&](const INode::Ptr& node)
    {
        if (node->isSelected())
        {
            return functor(node);
        }

        return true;
    });
}

void TextureToolSelectionSystem::foreachSelectedComponentNode(const std::function<bool(const INode::Ptr&)>& functor)
{
    GlobalTextureToolSceneGraph().foreachNode([&](const INode::Ptr& node)
    {
        auto componentSelectable = std::dynamic_pointer_cast<IComponentSelectable>(node);

        if (componentSelectable && componentSelectable->hasSelectedComponents())
        {
            return functor(node);
        }

        return true;
    });
}

void TextureToolSelectionSystem::foreachSelectedNodeOfAnyType(const std::function<bool(const INode::Ptr&)>& functor)
{
    if (getMode() == SelectionMode::Surface)
    {
        foreachSelectedNode(functor);
    }
    else
    {
        foreachSelectedComponentNode(functor);
    }
}

std::size_t TextureToolSelectionSystem::registerManipulator(const selection::ITextureToolManipulator::Ptr& manipulator)
{
    std::size_t newId = 1;

    while (_manipulators.count(newId) > 0)
    {
        ++newId;

        if (newId == std::numeric_limits<std::size_t>::max())
        {
            throw std::runtime_error("Out of manipulator IDs");
        }
    }

    _manipulators.emplace(newId, manipulator);

    manipulator->setId(newId);

    if (!_activeManipulator)
    {
        _activeManipulator = manipulator;
    }

    return newId;
}

void TextureToolSelectionSystem::unregisterManipulator(const selection::ITextureToolManipulator::Ptr& manipulator)
{
    for (auto i = _manipulators.begin(); i != _manipulators.end(); ++i)
    {
        if (i->second == manipulator)
        {
            i->second->setId(0);
            _manipulators.erase(i);
            return;
        }
    }
}

selection::IManipulator::Type TextureToolSelectionSystem::getActiveManipulatorType()
{
    return _activeManipulator->getType();
}

const selection::ITextureToolManipulator::Ptr& TextureToolSelectionSystem::getActiveManipulator()
{
    return _activeManipulator;
}

void TextureToolSelectionSystem::setActiveManipulator(std::size_t manipulatorId)
{
    auto found = _manipulators.find(manipulatorId);

    if (found == _manipulators.end())
    {
        rError() << "Cannot activate non-existent manipulator ID " << manipulatorId << std::endl;
        return;
    }

    _activeManipulator = found->second;

    // Release the user lock when switching manipulators
    _manipulationPivot.setUserLocked(false);
    _manipulationPivot.updateFromSelection();
}

void TextureToolSelectionSystem::setActiveManipulator(selection::IManipulator::Type manipulatorType)
{
    for (const auto& pair : _manipulators)
    {
        if (pair.second->getType() == manipulatorType)
        {
            _activeManipulator = pair.second;

            // Release the user lock when switching manipulators
            _manipulationPivot.setUserLocked(false);
            _manipulationPivot.updateFromSelection();
            return;
        }
    }

    rError() << "Cannot activate non-existent manipulator by type " << manipulatorType << std::endl;
}

std::size_t TextureToolSelectionSystem::getManipulatorIdForType(selection::IManipulator::Type type)
{
    for (const auto& pair : _manipulators)
    {
        if (pair.second->getType() == type)
        {
            return pair.first;
        }
    }

    return 0;
}

void TextureToolSelectionSystem::toggleManipulatorModeById(std::size_t manipId)
{
    std::size_t defaultManipId = getManipulatorIdForType(_defaultManipulatorType);

    if (defaultManipId == 0)
    {
        return;
    }

    // Switch back to the default mode if we're already in <mode>
    if (_activeManipulator->getId() == manipId && defaultManipId != manipId)
    {
        toggleManipulatorModeById(defaultManipId);
    }
    else if (_activeManipulator->getId() != manipId) // switch, if we're not in <mode> yet
    {
        setActiveManipulator(manipId);
        _sigActiveManipulatorChanged.emit(getActiveManipulatorType());
    }
}

Matrix4 TextureToolSelectionSystem::getPivot2World()
{
    _manipulationPivot.updateFromSelection();

    return _manipulationPivot.getMatrix4();
}

void TextureToolSelectionSystem::onManipulationStart()
{
    foreachSelectedNodeOfAnyType([&](const INode::Ptr& node)
    {
        node->beginTransformation();
        return true;
    });
}

void TextureToolSelectionSystem::onManipulationChanged()
{
}

void TextureToolSelectionSystem::onManipulationFinished()
{
    foreachSelectedNodeOfAnyType([&](const INode::Ptr& node)
    {
        node->commitTransformation();
        return true;
    });

    getActiveManipulator()->setSelected(false);
}

void TextureToolSelectionSystem::onManipulationCancelled()
{
    foreachSelectedNodeOfAnyType([&](const INode::Ptr& node)
    {
        node->revertTransformation();
        return true;
    });
}

sigc::signal<void, selection::IManipulator::Type>& TextureToolSelectionSystem::signal_activeManipulatorChanged()
{
    return _sigActiveManipulatorChanged;
}

void TextureToolSelectionSystem::selectPoint(SelectionTest& test, SelectionSystem::EModifier modifier)
{
    selection::SelectionPool selectionPool;

    performSelectionTest(selectionPool, test);

    if (selectionPool.empty()) return;

    auto bestSelectable = *selectionPool.begin();

    switch (modifier)
    {
    case SelectionSystem::eToggle:
        bestSelectable.second->setSelected(!bestSelectable.second->isSelected());
        break;

    case SelectionSystem::eReplace:
        bestSelectable.second->setSelected(bestSelectable.second->isSelected());
        break;

    case SelectionSystem::eCycle:
        {
            // Cycle through the selection pool and activate the item right after the currently selected
            auto i = selectionPool.begin();

            while (i != selectionPool.end())
            {
                if (i->second->isSelected())
                {
                    // unselect the currently selected one
                    i->second->setSelected(false);

                    // check if there is a "next" item in the list, if not: select the first item
                    ++i;

                    if (i != selectionPool.end())
                    {
                        i->second->setSelected(true);
                    }
                    else
                    {
                        selectionPool.begin()->second->setSelected(true);
                    }
                    break;
                }

                ++i;
            }
        }
        break;
    }
}

void TextureToolSelectionSystem::selectArea(SelectionTest& test, SelectionSystem::EModifier modifier)
{
    selection::SelectionPool selectionPool;

    performSelectionTest(selectionPool, test);

    for (const auto& pair : selectionPool)
    {
        pair.second->setSelected(!pair.second->isSelected());
    }
}

void TextureToolSelectionSystem::performSelectionTest(Selector& selector, SelectionTest& test)
{
    GlobalTextureToolSceneGraph().foreachNode([&](const INode::Ptr& node)
    {
        if (getMode() == SelectionMode::Surface)
        {
            node->testSelect(selector, test);
        }
        else
        {
            auto componentSelectable = std::dynamic_pointer_cast<IComponentSelectable>(node);

            if (componentSelectable)
            {
                componentSelectable->testSelectComponents(selector, test);
            }
        }

        return true;
    });
}

module::StaticModule<TextureToolSelectionSystem> _textureToolSelectionSystemModule;

}
