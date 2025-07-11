#ifndef PYTRIE_H
#define PYTRIE_H

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "util.h"
#include "IndexableBitVector.h"

namespace py = pybind11;

PYBIND11_MODULE(pypmctrie, m) {
    m.doc() = "State Trie Ordering module"; // optional module docstring
	m.def("doExploration", &doExploration, "Actually does the Trie tests");
	py::class_<Settings>(m, "Settings")
		.def(pybind11::init<std::vector<std::string>&, std::string, std::string, int>()) // may have to change to include params for constructor
		.def_readwrite("filename", &Settings::filename)
		.def_readwrite("propFileName", &Settings::propFileName)
		.def_readwrite("maxNumToExplore", &Settings::maxNumToExplore);
		//.def_readwrite("ordering", Settings::ordering);
}

/**
 * from pmctrie import *
 *
 * settings = Settings(species_list, filename, propFileName, 10000)
 * doExploration(settings)
 */

#endif // PYTRIE_H
