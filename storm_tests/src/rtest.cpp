/**
 * A test file which tests access times for the R-tree data structure
 * in boost. R tree queries the closest points
 * */

#include <iostream>
#include <cstdint>
#include <string>
#include <chrono>
#include <vector>
#include <queue>

#include "IndexableBitVector.h"

#include <storm/api/storm.h>
#include <storm-parsers/api/storm-parsers.h>
#include <storm-parsers/parser/PrismParser.h>
#include <storm/storage/prism/Program.h>
#include <storm/storage/jani/Property.h>
#include <storm/generator/PrismNextStateGenerator.h>
#include <storm/utility/initialize.h>

// Boost headers for the R-tree
#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>

#include "util.h"

namespace bgi = boost::geometry::index;

int main(int argc, char ** argv) {
	
	// First, you will need to load the model file
	std::string filename     = argc >= 2 ? std::string(argv[1]) : "default.prism";
	std::string propFilename = argc >= 3 ? std::string(argv[2]) : "default.csl";

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
		storm::api::parsePropertiesForPrismProgram(propFilename, *modelFile)
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
	// Currently, this R-tree uses the quadratic balancer. There is also
	// a linear balancer and an R* tree balancer we can use. To do those
	// swap out bgi::quadratic<16> with bgi::linear<16> or bgi::rstar<16?
	// respectively
	bgi::rtree<point, bgi::quadratic<16>> stateMap;

	// A simple exploration queue
	std::deque<CompressedState> explorationQueue;

	// Some vectors to store lookup and insertion times
	std::vector<LookupTime> lookupTimes;
	std::vector<InsertTime> insertTimes;
	
	// A variable that stores the state counts. We need this because (compressed) states
	// MUST be assigned indexes on creation.
	uint32_t stateCnt = 0;
	
	// Create a lambda (closure) that returns the last value of stateCnt and then increments it.
	// This is our "stateToIdCallback", which is crucial for how Storm does state expansion.
	// It is called for each new state in both NextStateGenerator::getInitialStates, and in 
	// NextStateGenerator::expand() for a particular state.
	auto const stateToIdCallback = std::function<uint32_t (const storm::generator::CompressedState &)>([&](const CompressedState & state) {
		// Create an indexable state object for use with a prefix tree.
		State idxableState(state);
		
		std::cout << "State: " << idxableState;
		idxableState.printIntegerVariables();
		std::cout << std::endl;

		// Measure lookup time here. If using Storm's bit vector hashmap
		// you should pass that in instead of idxableState
		auto startTime = std::chrono::high_resolution_clock::now();
		std::vector<point> closestStates;
		// The R-tree will query the closest states and get the closest one to the
		// given value. The state exists if the closest one is identical to the query 
		// state, else it is new.
		stateMap.query(bgi::nearest(idxableState, 1), std::back_inserter(closestStates));
		bool stateExists = closestStates[0].first == idxableState;
		auto lookupTime = std::chrono::high_resolution_clock::now() - startTime;
		// Store this lookup time in our vector
		// Note that some datastructures may not provide a `contains` function, 
		// and instead you can tell if it's in the set by the `get()` being equal
		// to the `end()` function or not.
		lookupTimes.push_back(LookupTime(lookupTime, stateCnt, stateExists));
		// If lookup was successful, we don't need to go any further
		if (stateExists) {
		 	return closestStates[0].second;
		}
		// This could be encoded in an invariant if in Rust or dafny
		// assert(stateStorage.getNumberOfStates() == stateCnt);
		// Enqueue new states to be explored
		explorationQueue.push_back(state);

		uint32_t idx = stateCnt++;
		// Time insertion
		startTime = std::chrono::high_resolution_clock::now();
		stateMap.insert(std::make_pair(idxableState, idx));
		auto insertTime = std::chrono::high_resolution_clock::now() = startTime;

		// Store our insertion times in the vector defined above
		// insertTimes.push_back(InsertTime(insertTime, stateCnt));
		
		// If not in the state storage, return the last value of stateCnt
		return idx;
	});

	auto initStateIndexes = generator->getInitialStates(stateToIdCallback);

	//TODO: Fix this
	int maxNumToExplore = 1000;

	while (!explorationQueue.empty() && stateCnt <= maxNumToExplore) {
		// All this loop needs to do is expand and enqueue the next states
		auto curState = explorationQueue.front();
		explorationQueue.pop_front();
		// Load the state to expand its successors
		generator->load(curState);

		// Expand the state (this enqueues its successors)
		auto behavior = generator->expand(stateToIdCallback);

		// Deterministic models should only have one choice
		for (auto const & choice : behavior) {
			for (auto const & stateProbabilityPair : choice) {
				// I don't know if you need these, but this is how you get
				// the index and probability of each successor.
				auto successorIdx            = stateProbabilityPair.first;
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

	return EXIT_SUCCESS;
}

