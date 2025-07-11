import os
import subprocess

ordersOfMagnitude = 12
models = [
    "Circuit0x8E_000to011",
    "Circuit0x8E_000to101",
    "Circuit0x8E_010to100",
    "Circuit0x8E_010to111",
    "Circuit0x8E_011to000",
    "Circuit0x8E_011to101",
    "Circuit0x8E_100to010",
    "Circuit0x8E_100to111",
    "Circuit0x8E_101to000",
    "Circuit0x8E_101to011",
    "Circuit0x8E_111to010",
    "Circuit0x8E_111to100",
    "Circuit0x8E_LHF_000to011",
    "Circuit0x8E_TI_100to111",
    "Circuit0x8E_TI_101to000",
    "Circuit0x8E_TI_101to011",
    "Circuit0x8E_TI_111to010",
    "Circuit0x8E_TI_111to100",
    "EnzymaticFutileCycle",
    "Majority_10_10",
    "Majority",
    "ModifiedYeastPolarization",
    "polling_T_10_N_12",
    "polling_T_10_N_16",
    "polling_T_10_N_20",
    "ReversibleIsomerization",
    "SimplifiedMotilityRegulation",
    "SingleSpeciesProductionDegradation",
    "Speed_Independent_10_10",
    "Speed_Independent",
    "Toggle_10_10",
    "Toggle",
    "Toggle_Switch_RBA",
    "Toggle_Switch"
]

numTests = 1
if not os.path.exists("results"):
    os.makedirs("results")

for i in range(ordersOfMagnitude):
    for m in models:
        if not os.path.exists("results/" + m):
            os.makedirs("results/" + m)
        if not os.path.exists("results/" + m + "/" + str(numTests)):
            os.makedirs("results/" + m + "/" + str(numTests))
        with open("results/" + m + "/" + str(numTests) + "/printout.txt", "w") as p:
            with open("results/" + m + "/" + str(numTests) + "/errors.txt", "w") as e:
                print("./build/trieTest", "models/" + m + ".sm", "models/" + m + ".csl", str(numTests), "... ", end="")
                subprocess.call(["./build/trieTest", "models/" + m + ".sm", "models/" + m + ".csl", str(numTests)], stderr=e, stdout=p, timeout=3600)
                print("\tcomplete")
    numTests *= 10

