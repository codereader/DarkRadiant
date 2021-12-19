#include "Curves.h"

#include "i18n.h"
#include "icurve.h"
#include "iundo.h"
#include "ieclass.h"
#include "itransformable.h"
#include "ientity.h"
#include "iorthoview.h"

#include "scenelib.h"
#include "selectionlib.h"
#include "gamelib.h"
#include "command/ExecutionNotPossible.h"

namespace selection
{

namespace algorithm
{

namespace
{
    const char* const GKEY_DEFAULT_CURVE_ENTITY = "/defaults/defaultCurveEntity";
    const char* const GKEY_CURVE_NURBS_KEY = "/defaults/curveNurbsKey";
    const char* const GKEY_CURVE_CATMULLROM_KEY = "/defaults/curveCatmullRomKey";
}

/**
 * greebo: Creates a new entity with an attached curve
 *
 * @key: The curve type: pass either "curve_CatmullRomSpline" or "curve_Nurbs".
 */
void createCurve(const std::string& key)
{
    UndoableCommand undo(std::string("createCurve: ") + key);

    // De-select everything before we proceed
    GlobalSelectionSystem().setSelectedAll(false);
    GlobalSelectionSystem().setSelectedAllComponents(false);

    std::string curveEClass = game::current::getValue<std::string>(GKEY_DEFAULT_CURVE_ENTITY);

    // Fallback to func_static, if nothing defined in the registry
    if (curveEClass.empty()) {
        curveEClass = "func_static";
    }

    // Find the default curve entity
    IEntityClassPtr entityClass = GlobalEntityClassManager().findOrInsert(
        curveEClass,
        true
    );

    // Create a new entity node deriving from this entityclass
    IEntityNodePtr curve(GlobalEntityModule().createEntity(entityClass));

    // Insert this new node into the scenegraph root
    GlobalSceneGraph().root()->addChildNode(curve);

    // Select this new curve node
    Node_setSelected(curve, true);

    // Set the model key to be the same as the name
    curve->getEntity().setKeyValue("model",
                                   curve->getEntity().getKeyValue("name"));

    // Initialise the curve using three pre-defined points
    curve->getEntity().setKeyValue(
        key,
        "3 ( 0 0 0  50 50 0  50 100 0 )"
    );

    ITransformablePtr transformable = scene::node_cast<ITransformable>(curve);
    if (transformable != NULL) {
        // Translate the entity to the center of the current workzone
        transformable->setTranslation(GlobalXYWndManager().getActiveViewOrigin());
        transformable->freezeTransform();
    }
}

void createCurveNURBS(const cmd::ArgumentList& args)
{
    createCurve(game::current::getValue<std::string>(GKEY_CURVE_NURBS_KEY));
}

void createCurveCatmullRom(const cmd::ArgumentList& args)
{
    createCurve(game::current::getValue<std::string>(GKEY_CURVE_CATMULLROM_KEY));
}

// A basic functor doing an action to the curve
class CurveNodeProcessor
{
public:
    virtual ~CurveNodeProcessor() {}
	virtual void operator() (CurveNode& curve) = 0;
};

// Appends a single control point to the visited curve
class CurveControlPointAppender :
	public CurveNodeProcessor
{
public:
	virtual void operator() (CurveNode& curve) {
		if (!curve.hasEmptyCurve()) {
			curve.appendControlPoints(1);
		}
	}
};

// Removes the selected control points from the visited curve
class CurveControlPointRemover :
	public CurveNodeProcessor
{
public:
	virtual void operator() (CurveNode& curve) {
		if (!curve.hasEmptyCurve()) {
			curve.removeSelectedControlPoints();
		}
	}
};

// Removes the selected control points from the visited curve
class CurveControlPointInserter :
	public CurveNodeProcessor
{
public:
	virtual void operator() (CurveNode& curve) {
		if (!curve.hasEmptyCurve()) {
			curve.insertControlPointsAtSelected();
		}
	}
};

// Removes the selected control points from the visited curve
class CurveConverter :
	public CurveNodeProcessor
{
public:
	virtual void operator() (CurveNode& curve) {
		if (!curve.hasEmptyCurve()) {
			curve.convertCurveType();
		}
	}
};

/** greebo: This visits all selected curves and	calls
 * 			the nominated Processor class using the
 * 			CurveNode as argument.
 */
class SelectedCurveVisitor :
	public SelectionSystem::Visitor
{
	CurveNodeProcessor& _processor;
public:
	SelectedCurveVisitor(CurveNodeProcessor& processor) :
		_processor(processor)
	{}

	void visit(const scene::INodePtr& node) const {
		// Try to cast the instance onto a CurveNode
		CurveNodePtr curve = Node_getCurve(node);
		if (curve != NULL) {
			// Call the processor
			_processor(*curve);
		}
	}
};

void appendCurveControlPoint(const cmd::ArgumentList& args)
{
	const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();

	if (info.entityCount == 0)
	{
		throw cmd::ExecutionNotPossible(
			_("Can't append curve point - no entities with curve selected.")
		);
	}

	UndoableCommand command("curveAppendControlPoint");

	// The functor object
	CurveControlPointAppender appender;

	// Traverse the selection applying the functor
	GlobalSelectionSystem().foreachSelected(
		SelectedCurveVisitor(appender)
	);
}

void removeCurveControlPoints(const cmd::ArgumentList& args)
{
	if (GlobalSelectionSystem().Mode() != SelectionSystem::eComponent ||
		GlobalSelectionSystem().ComponentMode() != selection::ComponentSelectionMode::Vertex)
	{
		throw cmd::ExecutionNotPossible(
			_("Can't remove curve points - must be in vertex editing mode.")
		);
	}

	const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();

	if (info.entityCount == 0)
	{
		throw cmd::ExecutionNotPossible(
			_("Can't remove curve points - no entities with curves selected.")
		);
	}

	UndoableCommand command("curveRemoveControlPoints");

	// The functor object
	CurveControlPointRemover remover;

	// Traverse the selection applying the functor
	GlobalSelectionSystem().foreachSelected(
		SelectedCurveVisitor(remover)
	);
}

void insertCurveControlPoints(const cmd::ArgumentList& args)
{
	if (GlobalSelectionSystem().Mode() != SelectionSystem::eComponent ||
		GlobalSelectionSystem().ComponentMode() != selection::ComponentSelectionMode::Vertex)
	{
		throw cmd::ExecutionNotPossible(
			_("Can't insert curve points - must be in vertex editing mode.")
		);
	}

	const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();

	if (info.entityCount == 0)
	{
		throw cmd::ExecutionNotPossible(
			_("Can't insert curve points - no entities with curves selected.")
		);
	}

	UndoableCommand command("curveInsertControlPoints");

	// The functor object
	CurveControlPointInserter inserter;

	// Traverse the selection applying the functor
	GlobalSelectionSystem().foreachSelected(
		SelectedCurveVisitor(inserter)
	);
}

void convertCurveTypes(const cmd::ArgumentList& args)
{
	const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();

	if (info.entityCount == 0)
	{
		throw cmd::ExecutionNotPossible(
			_("Can't convert curves - no entities with curves selected.")
		);
	}

	UndoableCommand command("curveConvertType");

	// The functor object
	CurveConverter converter;

	// Traverse the selection applying the functor
	GlobalSelectionSystem().foreachSelected(
		SelectedCurveVisitor(converter)
	);
}

} // namespace algorithm

} // namespace selection
