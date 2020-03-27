#include "SREntity.h"

#include "iuimanager.h"
#include "iregistry.h"
#include "itextstream.h"
#include "ieclass.h"
#include "igame.h"
#include "entitylib.h"

#include "SRPropertyLoader.h"
#include "SRPropertyRemover.h"
#include "SRPropertySaver.h"

#include <iostream>
#include <wx/artprov.h>

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
		if (i.second.getIndex() > index)
		{
			index = i.second.getIndex();
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
	SRPropertyLoader visitor(_keys, _list, _warnings);
	eclass->forEachClassAttribute(std::ref(visitor));

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
	auto found = _list.find(index);

	if (found == _list.end() || found->second.inherited())
	{
		return;
	}

	_list.erase(found);

	// Re-arrange the S/R indices. The parser in the game engine
	// walks up the indices and stops on the first missing index
	// not noticing there might be higher indices to follow (#5193)
	// TODO

	updateListStores();
}

int SREntity::duplicate(int fromIndex) 
{
	auto found = _list.find(fromIndex);

	if (found != _list.end())
	{
		int index = getHighestIndex() + 1;

		// Copy the object to the new id
		_list[index] = found->second;
		// Set the index and the inheritance status
		_list[index].setInherited(false);
		_list[index].setIndex(index);

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
	for (auto& i : _list)
	{
		int index = i.first;
		StimResponse& sr = i.second;

		wxutil::TreeModel::Row row = (sr.get("class") == "S") ?
			_stimStore->AddItem() : _responseStore->AddItem();

		// Store the ID into the liststore
		row[getColumns().index] = index;

		writeToListRow(row, sr);

		row.SendItemAdded();
	}
}

int SREntity::add()
{
	int index = getHighestIndex() + 1;

	// Create a new StimResponse object
	_list[index] = StimResponse();

	// Set the index and the inheritance status
	_list[index].setInherited(false);
	_list[index].setIndex(index);
	_list[index].set("class", "S");

	return index;
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
		saver.visit(i.second);
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

	wxBitmap iconBmp = wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + stimType.icon);
	wxIcon icon;
	icon.CopyFromBitmap(iconBmp);

	wxDataViewItemAttr colour;
	colour.SetColour(sr.inherited() ? wxColor(112,112,112) : wxColor(0,0,0));

	const SRListColumns& cols = getColumns();

	row[cols.index] = sr.getIndex();
	row[cols.index] = colour;
	row[cols.srClass] = wxVariant(wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + classIcon));
	row[cols.caption] = wxVariant(wxDataViewIconText(stimTypeStr, icon));
	row[cols.caption] = colour;
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
	auto i = _list.find(index);

	return i != _list.end() ? i->second : _emptyStimResponse;
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
