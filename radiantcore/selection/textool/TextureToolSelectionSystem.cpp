#include "TextureToolSelectionSystem.h"

#include "igrid.h"
#include "ishaders.h"
#include "iundo.h"
#include "itextstream.h"
#include "iradiant.h"
#include "module/StaticModule.h"
#include "messages/TextureChanged.h"
#include "../textool/TextureToolRotateManipulator.h"
#include "../textool/TextureToolDragManipulator.h"
#include "selection/SelectionPool.h"
#include "string/case_conv.h"
#include "math/Matrix3.h"
#include "selection/algorithm/Texturing.h"
#include "command/ExecutionFailure.h"

namespace textool
{

const std::string& TextureToolSelectionSystem::getName() const
{
    static std::string _name(MODULE_TEXTOOL_SELECTIONSYSTEM);
    return _name;
}

const StringSet& TextureToolSelectionSystem::getDependencies() const
{
    static StringSet _dependencies{ MODULE_TEXTOOL_SCENEGRAPH,
        MODULE_COMMANDSYSTEM, MODULE_RADIANT_CORE };
    return _dependencies;
}

void TextureToolSelectionSystem::initialiseModule(const IApplicationContext& ctx)
{
    _selectionMode = SelectionMode::Surface;

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
    GlobalCommandSystem().addCommand("TexToolSelectRelated",
        std::bind(&TextureToolSelectionSystem::selectRelatedCmd, this, std::placeholders::_1));
    GlobalCommandSystem().addCommand("TexToolSnapToGrid",
        std::bind(&TextureToolSelectionSystem::snapSelectionToGridCmd, this, std::placeholders::_1));
    GlobalCommandSystem().addCommand("TexToolNormaliseItems",
        std::bind(&TextureToolSelectionSystem::normaliseSelectionCmd, this, std::placeholders::_1));
    GlobalCommandSystem().addCommand("TexToolMergeItems",
        std::bind(&TextureToolSelectionSystem::mergeSelectionCmd, this, std::placeholders::_1),
        { cmd::ARGTYPE_VECTOR2 | cmd::ARGTYPE_OPTIONAL });

    GlobalCommandSystem().addCommand("TexToolShiftSelected",
        std::bind(&TextureToolSelectionSystem::shiftSelectionCmd, this, std::placeholders::_1),
        { cmd::ARGTYPE_VECTOR2 });
    GlobalCommandSystem().addCommand("TexToolScaleSelected",
        std::bind(&TextureToolSelectionSystem::scaleSelectionCmd, this, std::placeholders::_1),
        { cmd::ARGTYPE_VECTOR2 });
    GlobalCommandSystem().addCommand("TexToolRotateSelected",
        std::bind(&TextureToolSelectionSystem::rotateSelectionCmd, this, std::placeholders::_1),
        { cmd::ARGTYPE_DOUBLE });

    GlobalCommandSystem().addCommand("TexToolFlipS",
        std::bind(&TextureToolSelectionSystem::flipHorizontallyCmd, this, std::placeholders::_1));
    GlobalCommandSystem().addCommand("TexToolFlipT",
        std::bind(&TextureToolSelectionSystem::flipVerticallyCmd, this, std::placeholders::_1));

    _unselectListener = GlobalRadiantCore().getMessageBus().addListener(
        radiant::IMessage::Type::UnselectSelectionRequest,
        radiant::TypeListener<selection::UnselectSelectionRequest>(
            sigc::mem_fun(this, &TextureToolSelectionSystem::handleUnselectRequest)));
}

void TextureToolSelectionSystem::shutdownModule()
{
    clearComponentSelection();
    clearSelection();

    GlobalRadiantCore().getMessageBus().removeListener(_unselectListener);

    _sigSelectionChanged.clear();
    _sigSelectionModeChanged.clear();
    _sigActiveManipulatorChanged.clear();
    _manipulators.clear();
}

SelectionMode TextureToolSelectionSystem::getSelectionMode() const
{
    return _selectionMode;
}

void TextureToolSelectionSystem::setSelectionMode(SelectionMode mode)
{
    if (mode != _selectionMode)
    {
        _selectionMode = mode;
        _sigSelectionModeChanged.emit(_selectionMode);

        _manipulationPivot.setUserLocked(false);
        _manipulationPivot.setNeedsRecalculation(true);
    }
}

void TextureToolSelectionSystem::toggleSelectionMode(SelectionMode mode)
{
    // Switch back to Surface mode if toggling a non-default mode again
    if (mode == _selectionMode && mode != SelectionMode::Surface)
    {
        // Toggle back to Surface mode
        toggleSelectionMode(SelectionMode::Surface);
    }
    else
    {
        // setMode will only do something if we're not already in the target mode
        setSelectionMode(mode);
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
    if (getSelectionMode() == SelectionMode::Surface)
    {
        foreachSelectedNode(functor);
    }
    else
    {
        foreachSelectedComponentNode(functor);
    }
}

std::size_t TextureToolSelectionSystem::countSelected()
{
    std::size_t count = 0;

    foreachSelectedNode([&](const INode::Ptr& node)
    {
        ++count;
        return true;
    });

    return count;
}

std::size_t TextureToolSelectionSystem::countSelectedComponentNodes()
{
    std::size_t count = 0;

    foreachSelectedComponentNode([&](const INode::Ptr& node)
    {
        ++count;
        return true;
    });

    return count;
}

sigc::signal<void>& TextureToolSelectionSystem::signal_selectionChanged()
{
    return _sigSelectionChanged;
}

void TextureToolSelectionSystem::clearSelection()
{
    foreachSelectedNode([&](const INode::Ptr& node)
    {
        node->setSelected(false);
        return true;
    });
}

void TextureToolSelectionSystem::clearComponentSelection()
{
    foreachSelectedComponentNode([&](const INode::Ptr& node)
    {
        auto componentSelectable = std::dynamic_pointer_cast<IComponentSelectable>(node);

        if (componentSelectable)
        {
            componentSelectable->clearComponentSelection();
        }

        return true;
    });
}

void TextureToolSelectionSystem::handleUnselectRequest(selection::UnselectSelectionRequest& request)
{
    if (getSelectionMode() == SelectionMode::Vertex)
    {
        if (countSelectedComponentNodes() > 0)
        {
            clearComponentSelection();
        }
        else // no selection, just switch modes
        {
            setSelectionMode(SelectionMode::Surface);
        }

        request.setHandled(true);
    }
    else
    {
        if (countSelected() > 0)
        {
            clearSelection();
            request.setHandled(true);
        }
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

void TextureToolSelectionSystem::toggleManipulatorMode(selection::IManipulator::Type manipulatorType)
{
    toggleManipulatorModeById(getManipulatorIdForType(manipulatorType));
}

Matrix4 TextureToolSelectionSystem::getPivot2World()
{
    // For now, we recalculate the whole thing every time it is requested
    _manipulationPivot.setNeedsRecalculation(true);

    return _manipulationPivot.getMatrix4();
}

void TextureToolSelectionSystem::onManipulationStart()
{
    // Save the pivot state now that the transformation is starting
    _manipulationPivot.beginOperation();

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
    _manipulationPivot.endOperation();

    radiant::TextureChangedMessage::Send();
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

void TextureToolSelectionSystem::selectPoint(SelectionTest& test, selection::SelectionSystem::EModifier modifier)
{
    if (modifier == selection::SelectionSystem::eReplace)
    {
        if (getSelectionMode() == SelectionMode::Vertex)
        {
            clearComponentSelection();
        }
        else
        {
            clearSelection();
        }
    }

    selection::SelectionPool selectionPool;

    performSelectionTest(selectionPool, test);

    if (selectionPool.empty()) return;

    auto bestSelectable = *selectionPool.begin();

    switch (modifier)
    {
    case selection::SelectionSystem::eToggle:
        bestSelectable.second->setSelected(!bestSelectable.second->isSelected());
        break;

    case selection::SelectionSystem::eReplace:
        bestSelectable.second->setSelected(true);
        break;

    case selection::SelectionSystem::eCycle:
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

void TextureToolSelectionSystem::selectArea(SelectionTest& test, selection::SelectionSystem::EModifier modifier)
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
        if (getSelectionMode() == SelectionMode::Surface)
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

void TextureToolSelectionSystem::onNodeSelectionChanged(ISelectable& selectable)
{
    _sigSelectionChanged.emit();
}

void TextureToolSelectionSystem::onComponentSelectionChanged(ISelectable& selectable)
{
    _sigSelectionChanged.emit();
}

void TextureToolSelectionSystem::selectRelatedCmd(const cmd::ArgumentList& args)
{
    // Accumulate all selected nodes in a copied list, we're going to alter the selection
    std::vector<INode::Ptr> nodes;

    foreachSelectedNodeOfAnyType([&](const INode::Ptr& node)
    {
        nodes.push_back(node);
        return true;
    });

    for (const auto& node : nodes)
    {
        if (getSelectionMode() == textool::SelectionMode::Surface)
        {
            node->expandSelectionToRelated();
        }
        else
        {
            auto componentSelectable = std::dynamic_pointer_cast<IComponentSelectable>(node);

            if (componentSelectable)
            {
                componentSelectable->expandComponentSelectionToRelated();
            }
        }
    }
}

void TextureToolSelectionSystem::snapSelectionToGridCmd(const cmd::ArgumentList& args)
{
    UndoableCommand cmd("snapTexcoordsToGrid");

    foreachSelectedNodeOfAnyType([&](const INode::Ptr& node)
    {
        node->beginTransformation();

        if (getSelectionMode() == textool::SelectionMode::Surface)
        {
            node->snapto(GlobalGrid().getGridSize(grid::Space::Texture));
        }
        else
        {
            auto componentSnappable = std::dynamic_pointer_cast<ComponentSnappable>(node);

            if (componentSnappable)
            {
                componentSnappable->snapComponents(GlobalGrid().getGridSize(grid::Space::Texture));
            }
        }

        node->commitTransformation();
        return true;
    });

    radiant::TextureChangedMessage::Send();
}

void TextureToolSelectionSystem::mergeSelectionCmd(const cmd::ArgumentList& args)
{
    if (getSelectionMode() != SelectionMode::Vertex)
    {
        rWarning() << "This command can only be executed in Vertex manipulation mode" << std::endl;
        return;
    }

    AABB selectionBounds;

    // An optional argument will define the center
    if (args.size() == 1)
    {
        auto center = args[0].getVector2();
        selectionBounds.includePoint({ center.x(), center.y(), 0 });
    }
    else
    {
        // Calculate the center based on the component selection
        foreachSelectedComponentNode([&](const INode::Ptr& node)
        {
            auto componentSelectable = std::dynamic_pointer_cast<IComponentSelectable>(node);
            if (!componentSelectable) return true;

            selectionBounds.includeAABB(componentSelectable->getSelectedComponentBounds());

            return true;
        });
    }

    if (selectionBounds.isValid())
    {
        UndoableCommand cmd("mergeSelectedTexcoords");

        foreachSelectedNodeOfAnyType([&](const INode::Ptr& node)
        {
            node->beginTransformation();

            auto componentTransformable = std::dynamic_pointer_cast<IComponentTransformable>(node);

            if (componentTransformable)
            {
                componentTransformable->mergeComponentsWith({ selectionBounds.origin.x(), selectionBounds.origin.y() });
            }

            node->commitTransformation();
            return true;
        });

        radiant::TextureChangedMessage::Send();
    }
}

void TextureToolSelectionSystem::flipSelected(int axis)
{
    if (getSelectionMode() != SelectionMode::Surface)
    {
        rWarning() << "This command can only be executed in Surface manipulation mode" << std::endl;
        return;
    }

    // Calculate the center based on the selection
    selection::algorithm::TextureBoundsAccumulator accumulator;
    foreachSelectedNode(accumulator);

    if (!accumulator.getBounds().isValid())
    {
        return;
    }

    // Move center to origin, flip around the specified axis, and move back
    Vector2 flipCenter(accumulator.getBounds().origin.x(), accumulator.getBounds().origin.y());

    UndoableCommand cmd("flipSelectedTexcoords " + string::to_string(axis));

    selection::algorithm::TextureFlipper flipper(flipCenter, axis);
    foreachSelectedNode(flipper);
}

void TextureToolSelectionSystem::flipVerticallyCmd(const cmd::ArgumentList& args)
{
    flipSelected(1);
}

void TextureToolSelectionSystem::flipHorizontallyCmd(const cmd::ArgumentList& args)
{
    flipSelected(0);
}

void TextureToolSelectionSystem::normaliseSelectionCmd(const cmd::ArgumentList& args)
{
    if (getSelectionMode() != SelectionMode::Surface)
    {
        rWarning() << "This command can only be executed in Surface manipulation mode" << std::endl;
        return;
    }

    // Calculate the center based on the selection
    selection::algorithm::TextureBoundsAccumulator accumulator;
    foreachSelectedNode(accumulator);

    if (!accumulator.getBounds().isValid())
    {
        return;
    }

    Vector2 normaliseCenter(accumulator.getBounds().origin.x(), accumulator.getBounds().origin.y());

    UndoableCommand cmd("normaliseTexcoords");

    selection::algorithm::TextureNormaliser normaliser(normaliseCenter);
    foreachSelectedNode(normaliser);
}

void TextureToolSelectionSystem::shiftSelectionCmd(const cmd::ArgumentList& args)
{
    UndoableCommand cmd("shiftTexcoords");

    if (args.empty()) return;

    auto translation = args[0].getVector2();
    auto transform = Matrix3::getTranslation(translation);

    foreachSelectedNodeOfAnyType([&](const INode::Ptr& node)
    {
        node->beginTransformation();

        if (getSelectionMode() == textool::SelectionMode::Surface)
        {
            node->transform(transform);
        }
        else
        {
            auto componentTransformable = std::dynamic_pointer_cast<IComponentTransformable>(node);

            if (componentTransformable)
            {
                componentTransformable->transformComponents(transform);
            }
        }

        node->commitTransformation();
        return true;
    });

    radiant::TextureChangedMessage::Send();
}

void TextureToolSelectionSystem::scaleSelectionCmd(const cmd::ArgumentList& args)
{
    if (getSelectionMode() != SelectionMode::Surface)
    {
        rWarning() << "This command can only be executed in Surface manipulation mode" << std::endl;
        return;
    }

    UndoableCommand cmd("scaleTexcoords");

    if (args.size() < 1)
    {
        return;
    }

    auto scale = args[0].getVector2();

    if (scale.x() == 0 || scale.y() == 0)
    {
        throw cmd::ExecutionFailure("Scale factor cannot be zero");
    }

    // Calculate the center based on the selection
    selection::algorithm::TextureBoundsAccumulator accumulator;
    foreachSelectedNode(accumulator);

    if (!accumulator.getBounds().isValid())
    {
        return;
    }

    Vector2 pivot{ accumulator.getBounds().origin.x(), accumulator.getBounds().origin.y() };
    selection::algorithm::TextureScaler scaler(pivot, scale);
    foreachSelectedNode(scaler);
}

void TextureToolSelectionSystem::rotateSelectionCmd(const cmd::ArgumentList& args)
{
    if (getSelectionMode() != SelectionMode::Surface)
    {
        rWarning() << "This command can only be executed in Surface manipulation mode" << std::endl;
        return;
    }

    UndoableCommand cmd("rotateTexcoords");

    if (args.size() < 1)
    {
        return;
    }

    auto angle = degrees_to_radians(args[0].getDouble());

    // Calculate the center based on the selection
    selection::algorithm::TextureBoundsAccumulator accumulator;
    foreachSelectedNode(accumulator);

    if (!accumulator.getBounds().isValid())
    {
        return;
    }

    auto material = GlobalMaterialManager().getMaterial(GlobalTextureToolSceneGraph().getActiveMaterial());
    auto texture = material->getEditorImage();
    auto aspectRatio = static_cast<float>(texture->getWidth()) / texture->getHeight();

    Vector2 pivot{ accumulator.getBounds().origin.x(), accumulator.getBounds().origin.y() };
    selection::algorithm::TextureRotator rotator(pivot, angle, aspectRatio);
    foreachSelectedNode(rotator);
}

module::StaticModuleRegistration<TextureToolSelectionSystem> _textureToolSelectionSystemModule;

}
