#include "Trie.h"

#include <map>
#include <iostream>
#include <vector>
#include "IndexableBitVector.h"

namespace stamina {
namespace core {
namespace vectormap {

Trie::Trie(uint32_t max_index, uint32_t index) {
	this->max_index = max_index;
	this->index = index;
}

// assumes there has already been a contains() check
uint32_t
Trie::get(IndexableBitVector<uint32_t> stateVector, uint16_t pos) {

	if (stateVector.length() == pos + 1) return this->index;

	uint32_t const searchFor = stateVector[pos];
	return this->children[searchFor]->get(stateVector, pos+1);
}

bool
Trie::contains(IndexableBitVector<uint32_t> stateVector, uint16_t pos) {

	if (stateVector.length() == pos) return true;

	const uint32_t searchFor = stateVector[pos];
	if (this->children.find(searchFor) == this->children.end()) {
		return false;
	}
	else {
		return this->children[searchFor]->contains(stateVector, pos+1);
	}
}

// assumes we already did a contains() check; will overwrite otherwise
// ^^ This is a problem and is causing some unit tests to fail.
uint32_t
Trie::insert(IndexableBitVector<uint32_t> stateVector, uint16_t pos) {

	assert(pos < stateVector.length());

	uint32_t searchFor = stateVector[pos];

	auto newTrie = std::make_shared<Trie>(Trie(this->max_index, this->max_index)); // TODO: Set the properties here
	this->children[searchFor] = newTrie;

	if (stateVector.length() - 1 == pos) {
		return ++this->max_index;
	}

	// std::cout << "-- before rec: this->max_index=" << this->max_index << std::endl;

	this->max_index = this->children[searchFor]->insert(stateVector, pos+1);

	// std::cout << "Inserted node " << this->children[searchFor]->index << " with children " << std::endl;
	// this->printChildren();

	// std::cout << "-- after rec: this->max_index=" << this->max_index << std::endl;

	return this->max_index;

}

void
Trie::printChildren() {
	std::map<uint32_t, std::shared_ptr<Trie>>::iterator iter = this->children.begin();
	while (iter != children.end()) {
		std::cout << "---- Value: " << iter->first << ", Points to Trie: " << iter->second->index << std::endl;
		iter++;
	}
}

uint32_t
Trie::getNumberOfStates() {
	return this->max_index;
}

} // namespace vectormap
} // namespace core
} // namespace stamina
