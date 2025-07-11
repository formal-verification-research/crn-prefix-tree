#ifndef TRIE_H
#define TRIE_H

#include <map>
#include <memory>
#include <vector>
#include "IndexableBitVector.h"
#include <boost/container/flat_set.hpp>

namespace stamina {
	namespace core {
		namespace vectormap {
			// only works on ints now, maybe use templating later?
			class Trie {
			
				public:
					Trie(uint32_t max_index = 0, uint32_t index = 0, uint32_t * ordering = nullptr);
					~Trie();
					void printChildren();
					// TODO: Maybe accept indexableBitVector instead, need help with templating
					uint32_t get(IndexableBitVector<uint32_t> stateVector, uint16_t pos = 0);
					bool contains(IndexableBitVector<uint32_t> stateVector, uint16_t pos = 0);
					uint32_t insert(IndexableBitVector<uint32_t> stateVector, uint16_t pos = 0);
					uint32_t getNumberOfStates();
			
				private:
					uint32_t index;
					std::map<uint32_t, std::shared_ptr<Trie>> children;
					uint32_t max_index;
					uint32_t * ordering;
			};
		}
	}
}

#endif
