#ifndef STATE_REPRESENTATION_UTIL_H
#define STATE_REPRESENTATION_UTIL_H

#include <iostream>
#include <chrono>
#include <vector>

#include "IndexableBitVector.h"

// Some typedefs to make our lives easier. stamina::core::vectormap::IndexableBitVector<uint32_t>
// will just be called `State`, and storm::storage::BitVector will be called
// `CompressedState`
typedef stamina::core::vectormap::IndexableBitVector<uint32_t> State;
typedef storm::storage::BitVector CompressedState;
typedef std::pair<State, uint32_t> point;

// Change this if you want to have more tests
// const uint32_t MAX_NUMBER_STATES_TO_EXPLORE = 3;

// Points to store for lookup and insertion times
class LookupTime {
public:
	const std::chrono::duration<double> duration;
	const uint32_t setSize;
	const bool wasInSet;
	/**
	 * LookupTime contains both a duration and a set size. Obviously,
	 * larger set sizes will tend to have longer lookup times, but also
	 * if something is not in the set, it is likely to have a longer
	 * lookup time. This is applicable to hash sets, tries, and R-trees.
	 *
	 * @param duration The time it took to lookup the item
	 * @param setSize The set size at which the lookup was requested
	 * @param wasInSet Was the item in the set after lookup?
	 * */
	LookupTime(
		const std::chrono::duration<double> duration
		, const uint32_t setSize
		, const bool wasInSet
	) : duration(duration)
		, setSize(setSize)
		, wasInSet(wasInSet)
	{ /* Intentionally left empty */ }


	std::string toString()
	{
		return (std::to_string(duration.count()));
	}
};

class InsertTime {
public:
	const std::chrono::duration<double> duration;
	const uint32_t setSize;
	/**
	 * InsertTime only contains a duration and a set size. Generally you
	 * can assume that as setSize increases, insertion will take longer.
	 *
	 * @param duration Duration to insert
	 * @param setSize The size of the set at insertion.
	 * */
	InsertTime(
		const std::chrono::duration<double> duration
		, const uint32_t setSize
	) : duration(duration)
		, setSize(setSize)
	{ /* Intentionally left empty */ }

	std::ostream& operator<<(std::ostream& os)
	{
		os << duration.count();
		return os;
	}
};

class Settings {
public:
	std::vector<std::string> & ordering;
	std::string filename;
	std::string propFileName;
	int maxNumToExplore;

	Settings(
		std::vector<std::string> & ordering
		, std::string filename
		, std::string propFileName
		, int maxNumToExplore
	) :
		ordering(ordering)
		, filename(filename)
		, propFileName(propFileName)
		, maxNumToExplore(maxNumToExplore)
	{ /* Intentionally left empty */}

	uint32_t * orderingToArray() {
		uint32_t * orderArray = new uint32_t[ordering.size()];
		for (size_t i = 0; i < this->ordering.size(); ++i) {
			std::string species = this->ordering[i];
			auto idx = stamina::core::vectormap::IndexableBitVector<uint32_t>::indexFromString(species);
			assert(idx >= 0);
			orderArray[i] = idx;
		}
		return orderArray;
	}
};

void doPreprocessing();
void doExploration(Settings & settings);

#endif // STATE_REPRESENTATION_UTIL_H
