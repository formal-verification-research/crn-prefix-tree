/**
* Here's a demo main.c file which shows you how to use the IndexableBitVector class.
* Note that it's templated, which means that you will have to choose a StateType.
* This type simply refers to the type of the state index, generally `uint32_t`
*
* Some notes:
*   1) This assumes that you've got a working CMakeLists.txt that links against Storm
*   2) You should edit this file to include whatever lookup datastructure you wish to test.
* */

#include <iostream>
#include <cstdint>
#include <string>
#include <chrono>
#include <vector>
#include <queue>
#include <cassert>

#include "memMan.h"
#include "IndexableBitVector.h"
#include "Trie.h"

#include <storm/api/storm.h>
#include <storm-parsers/api/storm-parsers.h>
#include <storm-parsers/parser/PrismParser.h>
#include <storm/storage/prism/Program.h>
#include <storm/storage/jani/Property.h>
#include <storm/generator/PrismNextStateGenerator.h>
#include <storm/utility/initialize.h>

#include "util.h"

// from https://lemire.me/blog/2022/11/10/measuring-the-memory-usage-of-your-c-program/
void print_pages() {
    static size_t pagesize = sysconf(_SC_PAGESIZE);
    int64_t bytes = getCurrentRSS();
    assert((bytes % pagesize) == 0);
    size_t pages = bytes / pagesize;
    std::cout << "page size: " << pagesize << "\t";
    std::cout << "bytes: " << bytes << "\t";
    std::cout << "pages: " << pages << std::endl;
}

typedef stamina::core::vectormap::Trie Trie;

void doExploration(Settings & settings) {

	print_pages();

	// First, you will need to load the model file
	std::string filename = settings.filename; // argc >= 2 ? std::string(argv[1]) : "default.prism";
	std::string propFileName = settings.propFileName; //argc >= 3 ? std::string(argv[2]) : "default.csl";
	int maxNumToExplore = settings.maxNumToExplore; //argc >= 4 ? atoi(argv[3]) : 1000;

	storm::utility::setUp();
	storm::settings::initializeAll("main", "main");

	// To load the model file, we will use the static method storm::parser::PrismParser::parse
	// You'll also need to use std::make_shared to turn it into a shared pointer
	auto modelFile = std::make_shared<storm::prism::Program>(
		storm::parser::PrismParser::parse(filename, true)
	);

	auto labels = modelFile->getLabels();

	// You will also need to load the properties file because NextStateGenerator requires
	// knowledge of the properties.
	auto propertiesVector = std::make_shared<std::vector<storm::jani::Property>>(
		storm::api::parsePropertiesForPrismProgram(propFileName, *modelFile)
	);

	// You need a vector of the largest subformulae in the list of
	// CSL properties. This is how the NextStateGenerator "labels"
	// the states so that the probabilities vector can be turned into
	// an actual Pmin and Pmax.
	std::vector<std::shared_ptr<storm::logic::Formula const>> fv;
	for (auto & prop : *propertiesVector) {
		auto formula = prop.getFilter().getFormula();
		fv.push_back(formula);
	}

	// Create a BuilderOptions based on the vector of formulae
	storm::builder::BuilderOptions options(fv);

	// Now, we have to create a next state generator. This is how storm
	// takes a state and gives you its successors. The type parameters for
	// the generator are the probability type, and the state index type respectively
	auto generator = std::make_shared<storm::generator::PrismNextStateGenerator<double, uint32_t>>(
		*modelFile // Obviously, it has to be conscious of the model file
		, options // then, it also has to know about the options you set
	);

	// Now, after all that work, we have some information about the states.
	// IndexableBitVector now has to be conscious of that information.
	State::setVariableInformation(generator->getVariableInformation());

	// Finally, we can create indexable state objects from CompressedState (BitVector) objects
	// This will be done in our stateToIdCallback, since that's where the lookup times occur

	// This is where you would put
	//   a) your prefix tree
	//   b) the R-tree, X-tree, etc.
	//   c) Storm's BitVectorHashMap
	auto ordering = settings.orderingToArray(); //
	Trie stateStorage(0, 0, ordering);

	// A simple exploration queue
	std::deque<CompressedState> explorationQueue;

	// Some vectors to store lookup and insertion times
	std::vector<LookupTime> lookupTimes;
	std::vector<InsertTime> insertTimes;

	// A variable that stores the state counts. We need this because (compressed) states
	// MUST be assigned indexes on creation.
	uint32_t stateCnt = 0;
	uint32_t rejectedStates = 0;

	storm::generator::CompressedState * oldState = nullptr;

	// Create a lambda (closure) that returns the last value of stateCnt and then increments it.
	// This is our "stateToIdCallback", which is crucial for how Storm does state expansion.
	// It is called for each new state in both NextStateGenerator::getInitialStates, and in
	// NextStateGenerator::expand() for a particular state.
	auto const stateToIdCallback = std::function<uint32_t (const storm::generator::CompressedState &)>([&](const CompressedState & state) {
		
		// Create an indexable state object for use with a prefix tree.
		State idxableState(state);

		// Check that only the first numVarsToAllow variables are allowed to change
		if (oldState != nullptr) {
			State idxOldState(*oldState);
			
			uint32_t numVarsToAllow = 3;

			for (int i = numVarsToAllow; i < idxOldState.length(); i++) {
				if (idxOldState[i] != idxableState[i]) {
					rejectedStates++;
					return (uint32_t) -1;
				}
			}
		}

		// You can get each species' value using the [] operator on idxableState
		for (auto speciesValue : idxableState) {
			// This is just provided to show you how to use them when implementing your prefix tree
		}

		// std::cout << "State: " << idxableState;
		// idxableState.printIntegerVariables();
		// std::cout << std::endl;

		// Measure lookup time here. If using Storm's bit vector hashmap
		// you should pass that in instead of idxableState
		auto startTime = std::chrono::high_resolution_clock::now();
		bool stateExists = stateStorage.contains(idxableState, 0);
		auto lookupTime = std::chrono::high_resolution_clock::now() - startTime;

		// Store this lookup time in our vector
		// Note that some datastructures may not provide a `contains` function,
		// and instead you can tell if it's in the set by the `get()` being equal
		// to the `end()` function or not.
		lookupTimes.push_back(LookupTime(lookupTime, stateCnt, stateExists));
		// If lookup was successful, we don't need to go any further
		if (stateExists) {
			return stateStorage.get(idxableState, 0);
		}
		// This could be encoded in an invariant if in Rust or dafny
		// assert(stateStorage.getNumberOfStates() == stateCnt);
		// std::cout << "stateStorage.getNumberOfStates()=" << stateStorage.getNumberOfStates() << ", stateCnt=" << stateCnt << std::endl;
		// Enqueue new states to be explored
		explorationQueue.push_back(state);

		uint32_t idx = stateCnt++;

		// Time insertion
		startTime = std::chrono::high_resolution_clock::now();
		uint32_t newStateIndex = stateStorage.insert(idxableState, 0) - 1;
		auto insertTime = std::chrono::high_resolution_clock::now() - startTime;

		// Store our insertion times in the vector defined above
		insertTimes.push_back(InsertTime(insertTime, stateCnt));

		assert(idx == newStateIndex);
		// std::cout << "idx=" << idx << ", newStateIndex=" << newStateIndex << std::endl;
		// If not in the state storage, return the last value of stateCnt
		return idx;
	});

	auto initStateIndexes = generator->getInitialStates(stateToIdCallback);

	while (!explorationQueue.empty() && stateCnt <= maxNumToExplore) {
		// All this loop needs to do is expand and enqueue the next states
		auto curState = explorationQueue.front();
		explorationQueue.pop_front();
		// Load the state to expand its successors
		generator->load(curState);

		// for filtering purposes
		oldState = &curState;

		// Expand the state (this enqueues its successors)
		auto behavior = generator->expand(stateToIdCallback);

		// Deterministic models should only have one choice
		for (auto const & choice : behavior) {
			for (auto const & stateProbabilityPair : choice) {
				// I don't know if you need these, but this is how you get
				// the index and probability of each successor.
				auto successorIdx        = stateProbabilityPair.first;
				auto propensityOrProbability = stateProbabilityPair.second;
				// If you need the state values here, you
				// will have to maintain an array of state references
				// with a state's index in that array corresponding
				// to it's index assigned in stateToIdCallback.
				// That is not shown here for brevity sake.
			}
		}
	}

	// You probably want to store the results from your test here
	// either you can print them to the screen, or store them to a file.
	// Either way, it will be easier to create graphs for them in matplotlib
	// in python rather than directly in here.


	std::cout << "\n\n"<< std::endl;
	std::cout << std::setprecision(15) << std::fixed;

	std::cout << "rejectedStates " << rejectedStates << std::endl;

	std::cout << "lookupTimes.size()" << lookupTimes.size() << std::endl;

	double totalTime = 0.0f;
	int totalCount = 0;

	// std::cout << "duration\tsetSize\twasInSet" << std::endl;
	for (const LookupTime& i : lookupTimes) {
		// std::cout << std::setprecision(15) << std::fixed;
		// std::cout << i.duration.count() << "\t" << i.setSize << "\t" << i.wasInSet << std::endl;
		totalTime += i.duration.count();
		totalCount++;
	}

	std::cout << "\n\n"<< std::endl;
	std::cout << "total time: " << totalTime << std::endl;
	std::cout << "total Count: " << totalCount << std::endl;
	std::cout << "average time: " << totalTime / (double)totalCount << std::endl;
	print_pages();


	totalTime = 0.0f;
	totalCount = 0;

	std::cout << "\n\n"<< std::endl;
	std::cout << "insertTimes.size()" << insertTimes.size() << std::endl;

	// std::cout << "duration\tsetSize" << std::endl;
	for (const InsertTime& i : insertTimes) {
		// std::cout << std::setprecision(15) << std::fixed;
		// std::cout << i.duration.count() << "\t" << i.setSize << std::endl;
		totalTime += i.duration.count();
		totalCount++;
	}

	std::cout << "\n\n"<< std::endl;
	std::cout << "total time: " << totalTime << std::endl;
	std::cout << "total Count: " << totalCount << std::endl;
	std::cout << "average time: " << totalTime / (double)totalCount << std::endl;
	print_pages();

	std::cout << "\n\n"<< std::endl;
}

int main(int argc, char ** argv) {

	return EXIT_SUCCESS;
}

