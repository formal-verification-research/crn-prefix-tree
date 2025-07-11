import os
import sys

sys.path.append('bounding')

sys.path.insert(0, os.getcwd() + "/build")

import pypmctrie
import bounds

crnFile, inputFile, propertiesFile, maxStatesToExplore = sys.argv[1::]
maxStatesToExplore = int(maxStatesToExplore)

# do the yices stuff
speciesList = bounds.get_bounds(crnFile)

s = pypmctrie.Settings(speciesList, inputFile, propertiesFile, maxStatesToExplore)

pypmctrie.doExploration(s)

