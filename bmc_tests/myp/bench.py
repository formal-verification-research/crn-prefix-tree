import os
import subprocess

ordersOfMagnitude = 12
models = ["ModifiedYeastPolarization"]

numTests = 10
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
                print("python3", "test.py", "models/" + m + ".crn", "models/" + m + ".sm", "models/" + m + ".csl", str(numTests))
                # subprocess.call(["python3", "../test.py", "../models/" + m + ".crn", "../models/" + m + ".sm", "../models/" + m + ".csl", str(numTests)], stderr=subprocess.PIPE, stdout=subprocess.PIPE, timeout=5)
                subprocess.call(["python3", "test.py", "models/" + m + ".crn", "models/" + m + ".sm", "models/" + m + ".csl", str(numTests)], stderr=e, stdout=p, timeout=3600)
                print("\tcomplete")
    numTests *= 10
