#include "SREntity.h"

#include "iuimanager.h"
#include "iregistry.h"
#include "itextstream.h"
#include "ieclass.h"
#include "igame.h"
#include "entitylib.h"
#include "gtkutil/TreeModel.h"

#include "SRPropertyLoader.h"
#include "SRPropertyRemover.h"
#include "SRPropertySaver.h"

#include <iostream>

	namespace {
		const char* const GKEY_STIM_PROPERTIES =
			"/stimResponseSystem/properties//property";
	}

SREntity::SREntity(Entity* source, StimTypes& stimTypes) :
	_stimStore(Gtk::ListStore::create(getColumns())),
	_responseStore(Gtk::ListStore::create(getColumns())),
	_stimTypes(stimTypes)
{
	loadKeys();
	load(source);
}

int SREntity::getHighestId()
{
	int id = 0;

	for (StimResponseMap::iterator i = _list.begin(); i != _list.end(); ++i)
	{
		if (i->first > id) {
			id = i->first;
		}
	}
	return id;
}

int SREntity::getHighestIndex()
{
	int index = 0;

	for (StimResponseMap::iterator i = _list.begin(); i != _list.end(); ++i)
	{
		if (i->second.getIndex() > index) {
			index = i->second.getIndex();
		}
	}
	return index;
}

void SREntity::load(Entity* source)
{
	// Clear all the items from the liststore
	_stimStore->clear();
	_responseStore->clear();

	if (source == NULL) {
		return;
	}

	// Get the entity class to scan the inherited values
	IEntityClassPtr eclass = GlobalEntityClassManager().findOrInsert(
		source->getKeyValue("classname"), true
	);

	// Instantiate a visitor class with the list of possible keys
	// and the target list where all the S/Rs are stored
	// Warning messages are stored in the <_warnings> string
	SRPropertyLoader visitor(_keys, _list, _warnings);
	eclass->forEachClassAttribute(boost::ref(visitor));

	// Scan the entity itself after the class has been searched
	source->forEachKeyValue(visitor);

	// Populate the liststore
	updateListStores();
}

void SREntity::remove(int id)
{
	StimResponseMap::iterator found = _list.find(id);

	if (found != _list.end() && !found->second.inherited()) {
		_list.erase(found);
		updateListStores();
	}
}

int SREntity::duplicate(int fromId) {
	StimResponseMap::iterator found = _list.find(fromId);

	if (found != _list.end()) {
		int id = getHighestId() + 1;
		int index = getHighestIndex() + 1;

		// Copy the object to the new id
		_list[id] = found->second;
		// Set the index and the inheritance status
		_list[id].setInherited(false);
		_list[id].setIndex(index);

		// Rebuild the liststores
		updateListStores();

		return id;
	}

	return -1;
}

void SREntity::updateListStores()
{
	// Clear all the items from the liststore
	_stimStore->clear();
	_responseStore->clear();

	// Now populate the liststore
	for (StimResponseMap::iterator i = _list.begin(); i!= _list.end(); ++i)
	{
		int id = i->first;
		StimResponse& sr = i->second;

		Gtk::TreeModel::Row row = (sr.get("class") == "S") ?
			*_stimStore->append() : *_responseStore->append();

		// Store the ID into the liststore
		row[getColumns().id] = id;

		writeToListRow(row, sr);
	}
}

int SREntity::add() {
	int id = getHighestId() + 1;
	int index = getHighestIndex() + 1;

	// Create a new StimResponse object
	_list[id] = StimResponse();
	// Set the index and the inheritance status
	_list[id].setInherited(false);
	_list[id].setIndex(index);
	_list[id].set("class", "S");

	return id;
}

void SREntity::cleanEntity(Entity* target) {
	// Clean the entity from all the S/R spawnargs
	SRPropertyRemover remover(target, _keys);
	target->forEachKeyValue(remover);

	// scope ends here, SRPropertyRemover's destructor
	// will now delete the keys
}

void SREntity::save(Entity* target) {
	if (target == NULL) {
		return;
	}

	// Remove the S/R spawnargs from the entity
	cleanEntity(target);

	// Setup the saver object
	SRPropertySaver saver(target, _keys);
	for (StimResponseMap::iterator i = _list.begin(); i != _list.end(); ++i)
	{
		saver.visit(i->second);
	}
}

Gtk::TreeModel::iterator SREntity::getIterForId(const Glib::RefPtr<Gtk::ListStore>& targetStore, int id)
{
	// Setup the selectionfinder to search for the id string
	gtkutil::TreeModel::SelectionFinder finder(id, getColumns().id.index());

	targetStore->foreach_iter(sigc::mem_fun(finder, &gtkutil::TreeModel::SelectionFinder::forEach));

	return finder.getIter();
}

void SREntity::writeToListRow(const Gtk::TreeModel::Row& row, StimResponse& sr)
{
	StimType stimType = _stimTypes.get(sr.get("type"));

	std::string stimTypeStr = stimType.caption;
	stimTypeStr += (sr.inherited()) ? " (inherited) " : "";

	std::string classIcon = (sr.get("class") == "R") ? ICON_RESPONSE : ICON_STIM;
	classIcon += (sr.inherited()) ? SUFFIX_INHERITED : "";
	classIcon += (sr.get("state") != "1") ? SUFFIX_INACTIVE : "";
	classIcon += SUFFIX_EXTENSION;

	const SRListColumns& cols = getColumns();

	row[cols.index] = sr.getIndex();
	row[cols.srClass] = GlobalUIManager().getLocalPixbufWithMask(classIcon);
	row[cols.caption] = stimTypeStr;
	row[cols.icon] = GlobalUIManager().getLocalPixbufWithMask(stimType.icon);
	row[cols.inherited] = sr.inherited();
	row[cols.colour] = sr.inherited() ? "#707070" : "#000000";
}

void SREntity::setProperty(int id, const std::string& key, const std::string& value)
{
	// First, propagate the SR set() call
	StimResponse& sr = get(id);
	sr.set(key, value);

	const Glib::RefPtr<Gtk::ListStore>& targetStore =
		(sr.get("class") == "S") ? _stimStore : _responseStore;

	Gtk::TreeModel::iterator iter = getIterForId(targetStore, id);

	if (!iter)
	{
		rError() << "Cannot find S/R ID in liststore: " << id << std::endl;
		return;
	}

	writeToListRow(*iter, sr);
}

StimResponse& SREntity::get(int id)
{
	StimResponseMap::iterator i = _list.find(id);

	if (i != _list.end()) {
		return i->second;
	}
	else {
		return _emptyStimResponse;
	}
}

const SRListColumns& SREntity::getColumns()
{
	static SRListColumns _columns;
	return _columns;
}

const Glib::RefPtr<Gtk::ListStore>& SREntity::getStimStore()
{
	return _stimStore;
}

const Glib::RefPtr<Gtk::ListStore>& SREntity::getResponseStore()
{
	return _responseStore;
}

// static key loader
void SREntity::loadKeys()
{
	xml::NodeList propList = GlobalGameManager().currentGame()->getLocalXPath(GKEY_STIM_PROPERTIES);

	for (std::size_t i = 0; i < propList.size(); ++i)
	{
		// Create a new key and set the key name / class string
		SRKey newKey;
		newKey.key = propList[i].getAttributeValue("name");
		newKey.classes = propList[i].getAttributeValue("classes");

		// Add the key to the list
		_keys.push_back(newKey);
	}
}
