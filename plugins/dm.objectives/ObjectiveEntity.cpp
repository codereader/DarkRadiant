#include "ObjectiveEntity.h"
#include "ObjectiveKeyExtractor.h"
#include "TargetList.h"

#include "i18n.h"
#include "itextstream.h"
#include "iscenegraph.h"
#include "iundo.h"
#include "ientity.h"

#include "string/convert.h"
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/format.hpp>
#include <boost/regex.hpp>

namespace objectives {

	namespace
	{
		const std::string KV_SUCCESS_LOGIC("mission_logic_success");
		const std::string KV_FAILURE_LOGIC("mission_logic_failure");
		const int INVALID_LEVEL_INDEX = -9999;

		const std::string OBJ_COND_PREFIX("obj_condition_");
	}

// Constructor
ObjectiveEntity::ObjectiveEntity(const scene::INodePtr& node) :
	_entityNode(node)
{
	Entity* entity = Node_getEntity(node);
	assert(entity != NULL);

	// Use an ObjectiveKeyExtractor to populate the ObjectiveMap from the keys
	// on the entity
	ObjectiveKeyExtractor extractor(_objectives);
	entity->forEachKeyValue(extractor);

	// Parse the logic strings from the entity
	readMissionLogic(*entity);

	readObjectiveConditions(*entity);
}

void ObjectiveEntity::readObjectiveConditions(Entity& ent)
{
	_objConditions.clear(); // remove any previously parsed conditions

	Entity::KeyValuePairs condSpawnargs = ent.getKeyValuePairs(OBJ_COND_PREFIX);

	static const boost::regex objCondExpr(OBJ_COND_PREFIX + "(\\d+)_(.*)");

	for (Entity::KeyValuePairs::const_iterator kv = condSpawnargs.begin();
		 kv != condSpawnargs.end(); kv++)
	{
		boost::smatch results;

		if (!boost::regex_match(kv->first, results, objCondExpr))
		{
			continue; // No match, abort
		}

		int index = string::convert<int>(results[1]);

		// Valid indices are [1..infinity)
		if (index < 1) 
		{
			continue; // invalid index, continue
		}

		const ObjectiveConditionPtr& cond = getOrCreateObjectiveCondition(index);

		std::string postfix = results[2];

		if (postfix == "src_mission")
		{
			cond->sourceMission = string::convert<int>(kv->second);
		}
		else if (postfix == "src_obj")
		{
			cond->sourceObjective = string::convert<int>(kv->second);
		}
		else if (postfix == "src_state")
		{
			int val = string::convert<int>(kv->second);

			if (val >= Objective::INCOMPLETE && val < Objective::NUM_STATES)
			{
				cond->sourceState = static_cast<Objective::State>(val);
			}
			else
			{
				rWarning() << "Unsupported objective condition source state encountered: " 
					<< kv->second << std::endl;
			}
		}
		else if (postfix == "target_obj")
		{
			cond->targetObjective = string::convert<int>(kv->second);
		}
		else if (postfix == "type")
		{
			if (kv->second == "changestate")
			{
				cond->type = ObjectiveCondition::CHANGE_STATE;
			}
			else if (kv->second == "changevisibility")
			{
				cond->type = ObjectiveCondition::CHANGE_VISIBILITY;
			}
			else if (kv->second == "changemandatory")
			{
				cond->type = ObjectiveCondition::CHANGE_MANDATORY;
			}
			else
			{
				rWarning() << "Unsupported objective condition type encountered: " 
					<< kv->second << std::endl;
			}
		}
		else if (postfix == "value")
		{
			cond->value = string::convert<int>(kv->second);
		}
	}
}

void ObjectiveEntity::writeObjectiveConditions(Entity& ent)
{
	// No need to clear previous set of obj_condition_ spawnargs, 
	// as they've been removed by clearEntity() already

	// Spawnargs are numbered starting with 1 as first index
	std::size_t index = 1;

	// Go through all the conditions and save them. Skip invalid ones such that the
	// set of conditions will be "compressed" in terms of their indices.
	for (ObjectiveEntity::ConditionMap::const_iterator i = _objConditions.begin(); 
		 i != _objConditions.end(); ++i)
	{
		const ObjectiveCondition& cond = *i->second;

		if (!cond.isValid())
		{
			continue; // skip invalid conditions without increasing the index
		}

		std::string prefix = (boost::format(OBJ_COND_PREFIX + "%d_") % index).str();

		ent.setKeyValue(prefix + "src_mission", string::to_string(cond.sourceMission));
		ent.setKeyValue(prefix + "src_obj", string::to_string(cond.sourceObjective));
		ent.setKeyValue(prefix + "src_state", string::to_string(cond.sourceState));
		ent.setKeyValue(prefix + "target_obj", string::to_string(cond.targetObjective));

		std::string typeKey = prefix + "type";

		switch (cond.type)
		{
		case ObjectiveCondition::CHANGE_STATE:
			ent.setKeyValue(typeKey, "changestate");
			break;
		case ObjectiveCondition::CHANGE_VISIBILITY:
			ent.setKeyValue(typeKey, "changevisibility");
			break;
		case ObjectiveCondition::CHANGE_MANDATORY:
			ent.setKeyValue(typeKey, "changemandatory");
			break;
		default:
			ent.setKeyValue(typeKey, ""); // empty value to be sure
			rWarning() << "Invalid objective condition type encountered on saving." << std::endl;
			break;
		};

		ent.setKeyValue(prefix + "value", string::to_string(cond.value));

		++index; // next index
	}
}

void ObjectiveEntity::readMissionLogic(Entity& ent)
{
	// Find the success logic strings
	Entity::KeyValuePairs successLogics = ent.getKeyValuePairs(KV_SUCCESS_LOGIC);

	for (Entity::KeyValuePairs::const_iterator kv = successLogics.begin();
		 kv != successLogics.end(); kv++)
	{
		std::string postfix = kv->first.substr(KV_SUCCESS_LOGIC.size());

		if (postfix.empty()) {
			// Empty postfix means that we've found the default logic
			LogicPtr logic = getMissionLogic(-1);
			logic->successLogic = kv->second;
		}
		else if (boost::algorithm::starts_with(postfix, "_diff_")) {
			// We seem to have a difficulty-related logic, get the level
			int level = string::convert<int>(postfix.substr(6), INVALID_LEVEL_INDEX);

			if (level == INVALID_LEVEL_INDEX) {
				rError() << "[ObjectivesEditor]: Cannot parse difficulty-specific " <<
					"logic strings: " << kv->second << std::endl;
				continue;
			}

			LogicPtr logic = getMissionLogic(level);
			logic->successLogic = kv->second;
		}
	}

	// Find the failure logic strings
	Entity::KeyValuePairs failureLogics = ent.getKeyValuePairs(KV_FAILURE_LOGIC);

	for (Entity::KeyValuePairs::const_iterator kv = failureLogics.begin();
		 kv != failureLogics.end(); kv++)
	{
		std::string postfix = kv->first.substr(KV_FAILURE_LOGIC.size());

		if (postfix.empty()) {
			// Empty postfix means that we've found the default logic
			LogicPtr logic = getMissionLogic(-1);
			logic->failureLogic = kv->second;
		}
		else if (boost::algorithm::starts_with(postfix, "_diff_")) {
			// We seem to have a difficulty-related logic, get the level
			int level = string::convert<int>(postfix.substr(6), INVALID_LEVEL_INDEX);

			if (level == INVALID_LEVEL_INDEX) {
				rError() << "[ObjectivesEditor]: Cannot parse difficulty-specific " <<
					"logic strings: " << kv->second << std::endl;
				continue;
			}

			LogicPtr logic = getMissionLogic(level);
			logic->failureLogic = kv->second;
		}
	}
}

void ObjectiveEntity::writeMissionLogic(Entity& ent)
{
	for (LogicMap::iterator i = _logics.begin(); i != _logics.end(); i++) {
		int index = i->first;

		if (index == -1) {
			// Default logic
			ent.setKeyValue(KV_SUCCESS_LOGIC, i->second->successLogic);
			ent.setKeyValue(KV_FAILURE_LOGIC, i->second->failureLogic);
		}
		else {
			// Difficulty-specific logic
			ent.setKeyValue(KV_SUCCESS_LOGIC + "_diff_" + string::to_string(index), i->second->successLogic);
			ent.setKeyValue(KV_FAILURE_LOGIC + "_diff_" + string::to_string(index), i->second->failureLogic);
		}
	}
}

// Delete the entity's world node
void ObjectiveEntity::deleteWorldNode() {
	// Try to convert the weak_ptr reference to a shared_ptr
	scene::INodePtr node = _entityNode.lock();

	if (node != NULL) {
		GlobalSceneGraph().root()->removeChildNode(node);
	}
}

// Add a new objective
void ObjectiveEntity::addObjective() {
	// Locate the first unused id
	int index = 1;
	while (_objectives.find(index) != _objectives.end())
		++index;

	// Insert a new Objective at this ID.
	Objective o;
	o.description = (boost::format(_("New objective %d")) % index).str();
	_objectives.insert(ObjectiveMap::value_type(index, o));
}

void ObjectiveEntity::moveObjective(int index, int delta) {
	// Calculate the target index
	int targetIndex = index + delta;

	if (targetIndex < getLowestObjIndex()) {
		targetIndex = getLowestObjIndex() -1;
	}

	// Constrain the obj index to sane values
	if (targetIndex < 0) {
		targetIndex = 0;
	}

	if (targetIndex > getHighestObjIndex()) {
		targetIndex = getHighestObjIndex() + 1;
	}

	if (targetIndex == index) return; // nothing to do

	// Try to look up the command indices in the conversation
	ObjectiveMap::iterator oldObj = _objectives.find(index);
	ObjectiveMap::iterator newObj = _objectives.find(targetIndex);

	if (oldObj == _objectives.end()) return; // invalid source objective

	if (newObj == _objectives.end()) {
		// no objective at the target index, just re-locate the source objective
		Objective temp(oldObj->second);

		_objectives.erase(oldObj);

		_objectives[targetIndex] = temp;
	}
	else {
		// Both source and target indices exist, swap them
		Objective temp(oldObj->second);

		_objectives[index] = _objectives[targetIndex];
		_objectives[targetIndex] = temp;
	}
}

void ObjectiveEntity::deleteObjective(int index) {
	// Look up the objective with the given index
	ObjectiveMap::iterator i = _objectives.find(index);

	if (i == _objectives.end()) {
		// not found, nothing to do
		return;
	}

	// Delete the found element
	_objectives.erase(i++);

	// Then iterate all the way to the highest index
	while (i != _objectives.end()) {
		// Decrease the index of this objective
		int newIndex = i->first - 1;
		// Copy the objective into a temporary object
		Objective temp = i->second;

		// Remove the old one
		_objectives.erase(i++);

		// Re-insert with new index
		_objectives.insert(
			ObjectiveMap::value_type(newIndex, temp)
		);
	}
}

// Test for targeting
bool ObjectiveEntity::isOnTargetList(const TargetList& list) const {
	// Try to convert the weak_ptr reference to a shared_ptr
	Entity* entity = Node_getEntity(_entityNode.lock());
	assert(entity != NULL);

	return list.isTargeted(entity);
}

LogicPtr ObjectiveEntity::getMissionLogic(int difficultyLevel) {
	// The usual game, look up and insert if not found
	LogicMap::iterator i = _logics.find(difficultyLevel);

	if (i == _logics.end()) {
		std::pair<LogicMap::iterator, bool> result = _logics.insert(
			LogicMap::value_type(difficultyLevel, LogicPtr(new Logic))
		);

		i = result.first;
	}

	// At this point, the iterator is pointing to something valid
	return i->second;
}

// Returns the full list of objective conditions by value
ObjectiveEntity::ConditionMap ObjectiveEntity::getObjectiveConditions() const
{
	return _objConditions;
}

// Replaces the existing set of objective conditions with this new one
void ObjectiveEntity::setObjectiveConditions(const ObjectiveEntity::ConditionMap& conditions)
{
	_objConditions = conditions;
}

std::size_t ObjectiveEntity::getNumObjectiveConditions() const
{
	return _objConditions.size();
}

const ObjectiveConditionPtr& ObjectiveEntity::getOrCreateObjectiveCondition(int index)
{
	ConditionMap::iterator i = _objConditions.find(index);

	if (i == _objConditions.end())
	{
		// Insert and get iterator to new object
		i = _objConditions.insert(ConditionMap::value_type(
			index, ObjectiveConditionPtr(new ObjectiveCondition))).first;
	}

	return i->second;
}

void ObjectiveEntity::clearObjectiveConditions()
{
	_objConditions.clear();
}

void ObjectiveEntity::populateListStore(const Glib::RefPtr<Gtk::ListStore>& store,
										const ObjectivesListColumns& columns) const
{
	for (ObjectiveMap::const_iterator i = _objectives.begin();
		 i != _objectives.end();
		 ++i)
	{
		std::string diffStr = "all";

		if (!i->second.difficultyLevels.empty()) {
			// clear the string first
			diffStr.clear();

			// Split the string and increase each index by 1 for display (Level 1 == 0)
			std::vector<std::string> parts;
			boost::algorithm::split(parts, i->second.difficultyLevels, boost::algorithm::is_any_of(" "));

			for (std::size_t d = 0; d < parts.size(); ++d) {
				diffStr += (diffStr.empty()) ? "" : " ";
				diffStr += string::to_string(string::convert<int>(parts[d]) + 1);
			}
		}

		Gtk::TreeModel::Row row = *store->append();

		row[columns.objNumber] = i->first;
		row[columns.description] = i->second.description;
		row[columns.difficultyLevel] = diffStr;
	}
}

// Write the Components from a single Objective to the underlying entity
void ObjectiveEntity::writeComponents(Entity* entity,
    const std::string& keyPrefix, const Objective& obj
)
{
    assert(entity != NULL);

    for (Objective::ComponentMap::const_iterator i = obj.components.begin();
         i != obj.components.end();
         ++i)
    {
        const Component& c = i->second;

        // Component prefix is like obj1_2_blah
		std::string prefix = keyPrefix + string::to_string(i->first) + "_";

        // Write out Component keyvals
        entity->setKeyValue(prefix + "state", c.isSatisfied() ? "1" : "0");
        entity->setKeyValue(prefix + "not", c.isInverted() ? "1" : "0");
        entity->setKeyValue(prefix + "irreversible", c.isIrreversible() ? "1": "0");
        entity->setKeyValue(prefix + "player_responsible", c.isPlayerResponsible() ? "1" : "0");
        entity->setKeyValue(prefix + "type", c.getType().getName());

		entity->setKeyValue(prefix + "clock_interval",
			c.getClockInterval() > 0 ? string::to_string(c.getClockInterval()) : "");

        // Write out Specifier keyvals
		for (int i = Specifier::FIRST_SPECIFIER; i < Specifier::MAX_SPECIFIERS; i++)
		{
			// The specifier index of the spawnargs is starting from 1, not 0
			std::string indexStr = string::to_string(i + 1);

			SpecifierPtr spec = c.getSpecifier(static_cast<Specifier::SpecifierNumber>(i));

			if (spec != NULL) {
				entity->setKeyValue(prefix + "spec" + indexStr, spec->getType().getName());
				entity->setKeyValue(prefix + "spec_val" + indexStr, spec->getValue());
			}
		}

		// Export the component arguments
		entity->setKeyValue(prefix + "args", c.getArgumentString());
    }
}

void ObjectiveEntity::clearEntity(Entity* entity) {
	// Get all keyvalues matching the "obj" prefix.
	Entity::KeyValuePairs keyValues = entity->getKeyValuePairs("obj");

	for (Entity::KeyValuePairs::const_iterator i = keyValues.begin();
		 i != keyValues.end(); ++i)
	{
		// Set the spawnarg to empty, which is equivalent to a removal
		entity->setKeyValue(i->first, "");
	}
}

// Write out Objectives to entity keyvals
void ObjectiveEntity::writeToEntity()
{
	UndoableCommand cmd("saveObjectives");

	// Try to convert the weak_ptr reference to a shared_ptr
	Entity* entity = Node_getEntity(_entityNode.lock());
	assert(entity != NULL);

	// greebo: Remove all objective-related spawnargs first
	clearEntity(entity);

	for (ObjectiveMap::const_iterator i = _objectives.begin();
		 i != _objectives.end();
		 ++i)
	{
		// Obtain the Objective and construct the key prefix from the index
		const Objective& o = i->second;
		std::string prefix = "obj" + string::to_string(i->first) + "_";

		// Set the entity keyvalues
		entity->setKeyValue(prefix + "desc", o.description);
		entity->setKeyValue(prefix + "ongoing", o.ongoing ? "1" : "0");
		entity->setKeyValue(prefix + "visible", o.visible ? "1" : "0");
		entity->setKeyValue(prefix + "mandatory", o.mandatory ? "1" : "0");
		entity->setKeyValue(prefix + "irreversible",
							 o.irreversible ? "1" : "0");
		entity->setKeyValue(prefix + "state", string::to_string(o.state));

		// Write an empty "objN_difficulty" value when this objective applies to all levels
		entity->setKeyValue(prefix + "difficulty", o.difficultyLevels);

		entity->setKeyValue(prefix + "enabling_objs", o.enablingObjs);

		entity->setKeyValue(prefix + "script_complete", o.completionScript);
		entity->setKeyValue(prefix + "script_failed", o.failureScript);

		entity->setKeyValue(prefix + "target_complete", o.completionTarget);
		entity->setKeyValue(prefix + "target_failed", o.failureTarget);

		entity->setKeyValue(prefix + "logic_success", o.logic.successLogic);
		entity->setKeyValue(prefix + "logic_failure", o.logic.failureLogic);

        // Write the Components for this Objective
        writeComponents(entity, prefix, o);
	}

	// Export the mission success/failure logic
	writeMissionLogic(*entity);

	// Export objective conditions
	writeObjectiveConditions(*entity);
}

} // namespace objectives
