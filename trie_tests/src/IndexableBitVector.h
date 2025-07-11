#ifndef STAMINA_CORE_VECTORMAP_INDEXABLEBITVECTOR_H
#define STAMINA_CORE_VECTORMAP_INDEXABLEBITVECTOR_H

#include <storm/storage/BitVector.h>
#include <storm/generator/VariableInformation.h>

#include <algorithm>
#include <iostream>

namespace stamina {
	namespace core {
		namespace vectormap {
			typedef storm::storage::BitVector CompressedState;

			const uint16_t USE_ACTUAL_STATE_SIZE = 0;

			/**
			 * Simple wrapper class for storm::storage::BitVector that makes
			 * it easier to treat as a vector.
			 * */
			template <typename StateType>
			class IndexableBitVector {
			public:
				class iterator : public std::iterator<
								std::input_iterator_tag // iterator_category
								, StateType             // value_type
								, StateType             // difference_type
								, const StateType *     // pointer
								, StateType             // reference
								> {
				public:
					explicit iterator(uint32_t idx = 0, IndexableBitVector<StateType> * parent = nullptr)
						: idx(idx)
						, parent(parent)
						{ /* Intentionally left empty */ }
					iterator & operator++() {
						idx = parent->length() > idx ? idx + 1 : 0;
						return *this;
					}
					iterator operator++(int) {
						iterator ret = *this;
						++(*this);
						return ret;
					}
					bool operator==(iterator other) { return this->idx == other.idx && this->parent == other.parent; }
					bool operator!=(iterator other) { return !(*this == other); }
					StateType operator* () {
						return this->parent->get(this->idx);
					}
				private:
					uint32_t idx;
					IndexableBitVector<StateType> * parent;
				};

				/**
				 * Sets the variable information from the generator object
				 *
				 * @param vInfo The variable info to set
				 * */
				static void setVariableInformation(
					storm::generator::VariableInformation vInfo
				) {
					IndexableBitVector<StateType>::variableInformation = vInfo;
					IndexableBitVector<StateType>::hasOnlyIntVariables =
						variableInformation.booleanVariables.size() == 0
						&& variableInformation.locationVariables.size() == 0;
					IndexableBitVector<StateType>::initialized = true;
				}

				/**
				 * Sets the slice size for the bit vector (i.e., how many *bytes* per element).
				 * If zero, or `stamina::core::vectormap::USE_ACTUAL_STATE_SIZE`, the IndexableBitVector
				 * will use the number of *bytes* in `std::sizeof<StateType>`.
				 *
				 * @param sliceSize The number of *bytes* per element. *Not bits per element!*
				 * */
				static void setSliceSize(uint16_t sliceSize) {
					IndexableBitVector<StateType>::sliceSize = sliceSize;
				}

				/**
				 * Constructs an IndexableBitVector with a
				 * reference to a storm BitVector.
				 *
				 * @param state State reference to hold.
				 * */
				IndexableBitVector(const CompressedState & state)
					: state(state)
				{ /* Intentionally left empty */ }

				IndexableBitVector(const IndexableBitVector<StateType> & other)
					:state(other.state)
				{ /* Intentionally left empty */ }

				// Overloads for the `[]` operators
				StateType operator[](uint32_t idx) const {
					return this->get(idx);
				}
				
				StateType get(uint32_t idx) const {
					// std::cout << "idx=" << idx << ", sliceSize=" << (int) IndexableBitVector<StateType>::sliceSize << ", this->length=" << this->length() << std::endl;
					uint16_t bitWidth = IndexableBitVector<StateType>::sliceSize == 0
										// TODO: do we know they will all be the same width?
										? IndexableBitVector<StateType>::variableInformation.integerVariables[0].bitWidth
										: IndexableBitVector<StateType>::sliceSize;
					assert(idx < this->length());
					uint_fast64_t bitOffset = idx * bitWidth;
					return (StateType) state.getAsInt(bitOffset + 1, bitWidth - 1);
				}

				// Allows the for (auto val : myIndexableBitVector) syntax
				// TODO: handle when empty
				iterator begin() { return iterator(0, this); }
				// Turns out end() is not dereferenceable
				iterator end() { return iterator(this->length(), this); }

				/**
				 * Whether or not there are no elements to the BitVector
				 *
				 * @return If the vector length is `0`, returns `true`.
				 * */
				bool empty() { return this->length() == 0; }

				/**
				 * Returns the length of the IndexableBitVector. This is one higher
				 * than the highest legal index due to zero indexing.
				 *
				 * @return Length of IndexableBitVector
				 * */
				std::size_t length() const {
					uint16_t denominator = IndexableBitVector<StateType>::sliceSize;
					if (denominator == 0) {
						denominator = 8 * sizeof(StateType);
					}
					return state.size() / denominator;
				}

				/**
				 * Creates a std::vector containing each of the elements
				 * in the BitVector, assuming uniform size
				 *
				 * @return The `std::vector`
				 * */
				std::vector<StateType> toStdVector() const {
					std::vector<StateType> v;
					for (auto & elem : *this) {
						v.append(elem);
					}
					return v;
				}

				/**
				 * Prints just the integer variables
				 * */
				void printIntegerVariables() const {
					for (auto var : IndexableBitVector<StateType>::variableInformation.integerVariables) {
						uint_fast64_t bitOffset = var.bitOffset;
						uint16_t bitWidth = var.bitWidth;
						uint_fast64_t value = this->state.getAsInt(bitOffset + 1, bitWidth - 1);
						std::cout << "(" << var.getName() << ") " << value << ",";
					}
					std::cout << std::endl;
				}

				// Operator overloads
				bool operator==(IndexableBitVector<StateType> & other) { return this->state == other.state; }
				bool operator!=(IndexableBitVector<StateType> & other) { return this->state != other.state; }

				// This can be public since we're not editing it
				const CompressedState & state;

			private:
				// If zero, uses the size of StateType and only gets the
				// integer variables
				inline static uint16_t sliceSize = USE_ACTUAL_STATE_SIZE;
				inline static storm::generator::VariableInformation variableInformation;
				inline static bool hasOnlyIntVariables = false;
				inline static bool initialized = false;
			};

			template <typename StateType>
			std::ostream & operator<<(std::ostream & str, IndexableBitVector<StateType> & v) {
				for (auto elem : v) {
					str << elem << ", ";
				}
				str << std::endl;
				return str;
			}
		} // namespace vectormap
	} // namespace core
} // namespace stamina
#endif // STAMINA_CORE_VECTORMAP_INDEXABLEBITVECTOR_H
