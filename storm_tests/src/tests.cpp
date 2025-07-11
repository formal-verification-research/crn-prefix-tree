#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

#include <cstdlib>

#include <vector>

#include <storm/api/storm.h>
#include <storm-parsers/api/storm-parsers.h>
#include <storm-parsers/parser/PrismParser.h>
#include <storm/storage/prism/Program.h>
#include <storm/storage/jani/Property.h>
#include <storm/generator/PrismNextStateGenerator.h>
#include <storm/utility/initialize.h>

#include "IndexableBitVector.h"
#include "Trie.h"
#include "util.h"

#define NUM_STATES 50000
#define MAX_LEN 10

typedef stamina::core::vectormap::Trie Trie;

namespace bt = boost::unit_test;

static std::vector<CompressedState> states;

const CompressedState &
vecToCompressedState(std::vector<uint32_t> v, bool insertIntoStates = true) {
	uint16_t sliceSize = 8 * sizeof(uint32_t);
	CompressedState state(sliceSize * v.size());
	for (uint_fast64_t i = 0; i < v.size(); ++i) {
		uint_fast64_t bitIndex = i * sliceSize;
		state.setFromInt(bitIndex, sliceSize, v[i]);
	}
	if (insertIntoStates) { states.push_back(state); }
	return states[states.size() - 1];
}

std::vector<uint32_t>
createRandomVector(uint16_t size, uint32_t mx = 100) {
	std::vector<uint32_t> v;
	// std::cout << "vector: <";
	for (uint16_t i = 0; i < size; ++i) {
		v.push_back(rand() % mx);
		// std::cout << v[v.size()-1] << ",";
	}
	// std::cout << ">" << std::endl;
	return v;
}

State
createRandomState(uint16_t size = 5) {
	State::setSliceSize(8 * sizeof(uint32_t));
	return State(
			vecToCompressedState(
				createRandomVector(size)));
}

State
createUniqueRandomState(
	uint16_t size
	, storm::storage::sparse::StateStorage<uint32_t> & classicStateStorage
	, uint64_t stateId
) {
	uint16_t sliceSize = 8 * sizeof(uint32_t);
	CompressedState s(sliceSize * size);
	do {
		std::vector<uint32_t> rVec = createRandomVector(size);
		for (uint_fast64_t i = 0; i < size; ++i) {
			uint_fast64_t bitIndex = i * sliceSize;
			s.setFromInt(bitIndex, sliceSize, rVec[i]);
		}
	} while (!classicStateStorage.stateToId.contains(s));
	classicStateStorage.stateToId.findOrAdd(s, stateId);
	states.push_back(s);
	return State(states[states.size() - 1]);
}

/**
 * Broad test sweep which tests most of the Trie's features
 * */
BOOST_AUTO_TEST_CASE( mainTestSweep ) {
	uint32_t test_state_index = 0;
	uint32_t new_state_index = 0;
	uint32_t len_states = rand() % MAX_LEN + 1;
	Trie stateStorage(0, 0);
	for (int i = 0; i < NUM_STATES; i++) {
		/*random vector here*/
		State idxableState = createRandomState(len_states);
		if (!stateStorage.contains(idxableState, 0)) {
			new_state_index = stateStorage.insert(idxableState, 0);
			test_state_index++;
			BOOST_TEST(new_state_index == test_state_index
					, "Trie-generated index (" << new_state_index
					<< ") and manual incrementing index (" << test_state_index
					<< ") should match!");
			BOOST_TEST(stateStorage.contains(idxableState, 0)
					, "State with id " << test_state_index
					<< " should be contained in the trie!");
			BOOST_TEST(stateStorage.get(idxableState, 0) < test_state_index
					, "Landon, please fill out this test comment because idk what this tests");
		}
		else {
			BOOST_TEST(stateStorage.get(idxableState, 0) < test_state_index
					, "Stored index must be less than the number of vectors we've created!");
		}
	}
	// Clean up after ourselves
	states.clear();
}

/**
 * Tests that the inserted states are in fact, inserted with the right ID
 * */
BOOST_AUTO_TEST_CASE( insertionTest ) {
	uint32_t state_count = 0;
	uint32_t len_states = rand() % MAX_LEN + 1;
	Trie stateStorage;
	storm::storage::sparse::StateStorage<uint32_t> classicStateStorage(len_states * 8 * sizeof(uint32_t));
	for (int i = 0; i < NUM_STATES; i++) {
		// Create a unique state and insert it
		uint32_t stateId = state_count++;
		State state = createUniqueRandomState(len_states, classicStateStorage, stateId);
		BOOST_TEST(!stateStorage.contains(state)
				, "State should not yet exist in the trie");
		stateStorage.insert(state);
		BOOST_TEST(stateStorage.contains(state)
				, "State with id " << stateId << " should be contained in the Trie!");
		uint32_t gottenStateId = stateStorage.get(state);
		BOOST_TEST(gottenStateId == stateId
				, "Should have gotten same state IDs!"
				<< gottenStateId << " ?== " << stateId);
	}
}

/**
 * Tests that the returned ID of a state stays the same between searches
 * */
BOOST_AUTO_TEST_CASE( multiSearchTest ) {
	uint32_t state_count = 0;
	uint32_t len_states = rand() % MAX_LEN + 1;
	uint32_t TIMES_TO_SEARCH = 5;
	Trie stateStorage;
	storm::storage::sparse::StateStorage<uint32_t> classicStateStorage(len_states * 8 * sizeof(uint32_t));
	for (int i = 0; i < NUM_STATES; i++) {
		// Create a unique state and insert it
		uint32_t stateId = state_count++;
		State state = createUniqueRandomState(len_states, classicStateStorage, stateId);
		stateStorage.insert(state);
		for (int j = 0; j < TIMES_TO_SEARCH; j++) {
			BOOST_TEST(stateStorage.contains(state)
				, "State with id " << stateId << " should be contained in the Trie!");
			uint32_t gottenStateId = stateStorage.get(state);
			BOOST_TEST(gottenStateId == stateId
				, "Should have gotten same state IDs!"
				<< gottenStateId << " ?== " << stateId);
		}
	}
}

/**
 * Tests that this is indeed a SET. You can only insert something once.
 * */
BOOST_AUTO_TEST_CASE( multiInsertTest ) {
	uint32_t state_count = 0;
	uint32_t len_states = rand() % MAX_LEN + 1;
	uint32_t TIMES_TO_INSERT = 5;
	Trie stateStorage;
	storm::storage::sparse::StateStorage<uint32_t> classicStateStorage(len_states * 8 * sizeof(uint32_t));
	for (int i = 0; i < NUM_STATES; i++) {
		// Create a unique state and insert it
		uint32_t stateId = state_count++;
		State state = createUniqueRandomState(len_states, classicStateStorage, stateId);
		stateStorage.insert(state);
		for (int j = 0; j < TIMES_TO_INSERT; j++) {
			BOOST_TEST(stateStorage.contains(state)
				, "State with id " << stateId << " should be contained in the Trie!");
			stateStorage.insert(state);
			uint32_t gottenStateId = stateStorage.get(state);
			BOOST_TEST(gottenStateId == stateId
				, "Should have gotten same state IDs!"
				<< gottenStateId << " ?== " << stateId);
		}
	}
}

/**
 * Tests that a state which does not exist in the set is reported as such.
 * */
BOOST_AUTO_TEST_CASE( dneTest ) {
	uint32_t state_count = 0;
	uint32_t len_states = rand() % MAX_LEN + 1;
	Trie stateStorage;
	storm::storage::sparse::StateStorage<uint32_t> classicStateStorage(len_states * 8 * sizeof(uint32_t));
	for (int i = 0; i < NUM_STATES; i++) {
		// Create a unique state and insert it
		uint32_t stateId = state_count++;
		State state = createUniqueRandomState(len_states, classicStateStorage, stateId);
		BOOST_TEST(!stateStorage.contains(state)
				, "State should not yet exist in the trie");
		stateStorage.insert(state);
	}
}
