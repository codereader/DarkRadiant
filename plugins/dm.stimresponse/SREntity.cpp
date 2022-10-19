#include "SREntity.h"

#include "iregistry.h"
#include "itextstream.h"
#include "ieclass.h"
#include "igame.h"
#include "entitylib.h"

#include "SRPropertyLoader.h"
#include "SRPropertyRemover.h"
#include "SRPropertySaver.h"

#include <iostream>
#include "wxutil/Bitmap.h"
#include "wxutil/Icon.h"

namespace
{
	const char* const GKEY_STIM_PROPERTIES = "/stimResponseSystem/properties//property";
}

SREntity::SREntity(Entity* source, StimTypes& stimTypes) :
	_stimStore(new wxutil::TreeModel(getColumns(), true)),
	_responseStore(new wxutil::TreeModel(getColumns(), true)),
	_stimTypes(stimTypes)
{
	loadKeys();
	load(source);
}

int SREntity::getHighestIndex()
{
	int index = 0;

	for (const auto& i : _list)
	{
		if (i.getIndex() > index)
		{
			index = i.getIndex();
		}
	}

	return index;
}

int SREntity::getHighestInheritedIndex()
{
	int index = 0;

	for (const auto& i : _list)
	{
		if (i.inherited() && i.getIndex() > index)
		{
			index = i.getIndex();
		}
	}

	return index;
}

void SREntity::load(Entity* source)
{
	// Clear all the items from the liststore
	_stimStore->Clear();
	_responseStore->Clear();

	if (source == nullptr)
	{
		return;
	}

	// Get the entity class to scan the inherited values
	IEntityClassPtr eclass = GlobalEntityClassManager().findOrInsert(
		source->getKeyValue("classname"), true
	);

	// Instantiate a visitor class with the list of possible keys
	// and the target list where all the S/Rs are stored
	// Warning messages are stored in the <_warnings> string
	SRPropertyLoader visitor(_keys, *this, _warnings);
	eclass->forEachAttribute(std::ref(visitor));

	// Scan the entity itself after the class has been searched
    source->forEachKeyValue([&](const std::string& key, const std::string& value)
    {
        visitor.visitKeyValue(key, value);
    });

	// Populate the liststore
	updateListStores();
}

void SREntity::remove(int index)
{
	auto found = findByIndex(index);

	if (found == _list.end() || found->inherited())
	{
		return;
	}

	_list.erase(found);

	// Re-arrange the S/R indices. Otherwise he parser in the game engine
	// walks up the indices and stops on the first missing index
	// not noticing there might be higher indices to follow (#5193)
	int highestInheritedIndex = getHighestInheritedIndex();
	int nextIndex = highestInheritedIndex + 1;

	// Iterate over all non-inherited objects and assign new indices
	for (auto i = _list.begin(); i != _list.end(); ++i)
	{
		if (!i->inherited())
		{
			// Found a non-inherited stim, assign the index
			i->setIndex(nextIndex++);
		}
	}

	updateListStores();
}

int SREntity::duplicate(int fromIndex)
{
	auto found = findByIndex(fromIndex);

	if (found != _list.end())
	{
		int index = getHighestIndex() + 1;

		// Copy the object to the new id
		_list.push_back(StimResponse(*found));

		// Set the index and the inheritance status
		_list.back().setInherited(false);
		_list.back().setIndex(index);

		// Rebuild the liststores
		updateListStores();

		return index;
	}

	return -1;
}

void SREntity::updateListStores()
{
	// Clear all the items from the liststore
	_stimStore->Clear();
	_responseStore->Clear();

	// Now populate the liststore
	for (auto& sr : _list)
	{
		auto row = sr.get("class") == "S" ? _stimStore->AddItem() : _responseStore->AddItem();

		// Store the index into the liststore
		row[getColumns().index] = sr.getIndex();

		writeToListRow(row, sr);

		row.SendItemAdded();
	}
}

int SREntity::add()
{
	return add(getHighestIndex() + 1).getIndex();
}

StimResponse& SREntity::add(int index)
{
	// Create a new StimResponse object
	_list.push_back(StimResponse());

	// Set the index and the inheritance status
	_list.back().setInherited(false);
	_list.back().setIndex(index);
	_list.back().set("class", "S");

	return _list.back();
}

void SREntity::cleanEntity(Entity* target)
{
	// Clean the entity from all the S/R spawnargs
	SRPropertyRemover remover(target, _keys);
    target->forEachKeyValue([&](const std::string& key, const std::string& value)
    {
        remover.visitKeyValue(key, value);
    });

	// scope ends here, SRPropertyRemover's destructor
	// will now delete the keys
}

void SREntity::save(Entity* target)
{
	if (target == nullptr)
	{
		return;
	}

	// Remove the S/R spawnargs from the entity
	cleanEntity(target);

	// Setup the saver object
	SRPropertySaver saver(target, _keys);

	for (auto& i : _list)
	{
		saver.visit(i);
	}
}

wxDataViewItem SREntity::getIterForIndex(wxutil::TreeModel& targetStore, int index)
{
	return targetStore.FindInteger(index, getColumns().index);
}

void SREntity::writeToListRow(wxutil::TreeModel::Row& row, StimResponse& sr)
{
	StimType stimType = _stimTypes.get(sr.get("type"));

	std::string stimTypeStr = stimType.caption;
	stimTypeStr += (sr.inherited()) ? " (inherited) " : "";

	std::string classIcon = (sr.get("class") == "R") ? ICON_RESPONSE : ICON_STIM;
	classIcon += (sr.inherited()) ? SUFFIX_INHERITED : "";
	classIcon += (sr.get("state") != "1") ? SUFFIX_INACTIVE : "";
	classIcon += SUFFIX_EXTENSION;

	wxutil::Icon icon(wxutil::GetLocalBitmap(stimType.icon));

	wxDataViewItemAttr colour;
	colour.SetColour(sr.inherited() ? wxColor(112,112,112) : wxColor(0,0,0));

	const SRListColumns& cols = getColumns();

	row[cols.index] = sr.getIndex();
	row[cols.index].setAttr(colour);
	row[cols.srClass] = wxVariant(wxutil::GetLocalBitmap(classIcon));
	row[cols.caption] = wxVariant(wxDataViewIconText(stimTypeStr, icon));
	row[cols.caption].setAttr(colour);
	row[cols.inherited] = sr.inherited();
}

void SREntity::setProperty(int index, const std::string& key, const std::string& value)
{
	// First, propagate the SR set() call
	StimResponse& sr = get(index);
	sr.set(key, value);

	wxutil::TreeModel::Ptr targetStore = (sr.get("class") == "S") ? _stimStore : _responseStore;

	wxDataViewItem item = getIterForIndex(*targetStore, index);

	if (!item.IsOk())
	{
		rError() << "Cannot find S/R index in liststore: " << index << std::endl;
		return;
	}

	wxutil::TreeModel::Row row(item, *targetStore);
	writeToListRow(row, sr);
	row.SendItemChanged();
}

StimResponse& SREntity::get(int index)
{
	auto i = findByIndex(index);

	return i != _list.end() ? *i : _emptyStimResponse;
}

const SRListColumns& SREntity::getColumns()
{
	static SRListColumns _columns;
	return _columns;
}

wxutil::TreeModel::Ptr SREntity::getStimStore()
{
	return _stimStore;
}

wxutil::TreeModel::Ptr SREntity::getResponseStore()
{
	return _responseStore;
}

// static key loader
void SREntity::loadKeys()
{
	xml::NodeList propList = GlobalGameManager().currentGame()->getLocalXPath(GKEY_STIM_PROPERTIES);

	for (const auto& node : propList)
	{
		// Create a new key and set the key name / class string
		SRKey newKey;
		newKey.key = node.getAttributeValue("name");
		newKey.classes = node.getAttributeValue("classes");

		// Add the key to the list
		_keys.push_back(newKey);
	}
}

SREntity::StimsAndResponses::iterator SREntity::findByIndex(int index)
{
	for (StimsAndResponses::iterator i = _list.begin(); i != _list.end(); ++i)
	{
		if (i->getIndex() == index)
		{
			return i;
		}
	}

	return _list.end();
}

