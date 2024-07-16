import pandas as pd
import matplotlib.pyplot as plt

# df = pd.read_csv("10_5_results.csv", header=None, names=["Binary", "Time"])
df = pd.read_csv("10_15_results.csv", header=None, names=["Binary", "Time"])

pkgmain_data = df[df["Binary"] == "pkgmain"]
pkgmain_parallel_data = df[df["Binary"] == "pkgmain_parallel"]

# avg_pkgmain = pkgmain_data["Time"].mean()
# avg_pkgmain_parallel = pkgmain_parallel_data["Time"].mean()
# print(avg_pkgmain)
# print(avg_pkgmain_parallel)
# 15.020031079700004
# 13.585047593799999

plt.figure(figsize=(10, 6))
plt.plot(pkgmain_data.index, pkgmain_data["Time"], label='pkgmain', marker='o')
plt.plot(pkgmain_parallel_data.index, pkgmain_parallel_data["Time"], 
        label='pkgmain_parallel', marker='o')
plt.xlabel('Iteration')
plt.ylabel('Time (seconds)')

# Comments for creating the graph of the other result
# plt.title('Benchmark Times for 50 test iterations')
plt.title('Benchmark Times for 150 test iterations')
plt.legend()
plt.grid(True)
# plt.savefig('10_5.png')
plt.savefig('10_15.png')
plt.show()

