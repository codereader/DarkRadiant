#include "SelectedNodeList.h"

const scene::INodePtr& SelectedNodeList::ultimate() {
	if (size() == 0) {
		return end()->first; // return invalid iterator
	}
	
	unsigned long latestTime(0);
	iterator latest;
	
	for (iterator i = begin(); i != end(); i++) {
		if (i->second > latestTime) {
			latestTime = i->second;
			latest = i;
		}
	}
	
	return latest->first;
}

const scene::INodePtr& SelectedNodeList::ultimate() const {
	if (size() == 0) {
		return end()->first; // return invalid iterator
	}
	
	unsigned long latestTime(0);
	const_iterator latest;
	
	for (const_iterator i = begin(); i != end(); i++) {
		if (i->second > latestTime) {
			latestTime = i->second;
			latest = i;
		}
	}
	
	return latest->first;
}

const scene::INodePtr& SelectedNodeList::penultimate() const {
	if (size() <= 1) {
		return end()->first; // return invalid iterator
	}
	
	unsigned long ultimateTime(0);
	unsigned long penUltimateTime(0);
	const_iterator ultimate;
	const_iterator penUltimate;
	
	for (const_iterator i = begin(); i != end(); i++) {
		if (i->second > ultimateTime) {
			penUltimateTime = ultimateTime;
			penUltimate = ultimate;
			
			ultimateTime = i->second;
			ultimate = i;
		}
		else if (i->second > penUltimateTime) {
			penUltimateTime = i->second;
			penUltimate = i;
		}
	}
	
	return penUltimate->first;
}

const scene::INodePtr& SelectedNodeList::penultimate() {
	if (size() <= 1) {
		return end()->first; // return invalid iterator
	}
	
	unsigned long ultimateTime(0);
	unsigned long penUltimateTime(0);
	iterator ultimate;
	iterator penUltimate;
	
	for (iterator i = begin(); i != end(); i++) {
		if (i->second > ultimateTime) {
			penUltimateTime = ultimateTime;
			penUltimate = ultimate;
			
			ultimateTime = i->second;
			ultimate = i;
		}
		else if (i->second > penUltimateTime) {
			penUltimateTime = i->second;
			penUltimate = i;
		}
	}
	
	return penUltimate->first;
}

void SelectedNodeList::append(const scene::INodePtr& selected) {
	time++;
	MapType::insert(value_type(selected, time));
}

void SelectedNodeList::erase(const scene::INodePtr& selected) {
	iterator last;
	unsigned long lastTime(0);
	
	// Lookup the instance selected last
	for (iterator it = MapType::find(selected);
		 it != upper_bound(selected) && it != end();
		 it++)
	{
		if (it->second > lastTime) {
			last = it;
			lastTime = it->second;
		}
	}
	
	assert(last != end());
	
	// Remove the element selected last, leave the others
	MapType::erase(last);
}

unsigned long SelectedNodeList::time = 1;
