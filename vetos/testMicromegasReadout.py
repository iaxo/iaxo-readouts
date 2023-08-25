import REST
import ROOT

import numpy as np
import matplotlib.pyplot as plt

file = ROOT.TFile("fullReadout.root")

readout = file.Get("fullReadout")

plane_index = 0  # this is the micromegas readout plane

# iterate over an x,y grid of 30x30 every 0.1

side_limit = 35
step = 0.1

x = np.arange(-side_limit, side_limit, step)
y = np.arange(-side_limit, side_limit, step)

X, Y = np.meshgrid(x, y)

Z = np.zeros((len(x), len(y)))

for i in range(len(x)):
    for j in range(len(y)):
        position = ROOT.TVector3(x[i], y[j], 0)
        daqId, moduleId, channelId = readout.GetHitsDaqChannelAtReadoutPlane((x[i], y[j], 0), plane_index)
        Z[i][j] = daqId
        # print("x: ", x[i], " y: ", y[j], " daqId: ", daqId)

# Draw Z as a color map, excluding values of -1

Z[Z == -1] = np.nan

# count the unique values in Z (non nan)
unique, counts = np.unique(Z[~np.isnan(Z)], return_counts=True)
# print the number of unique values
print(f"number of unique values: {len(unique)}")

plt.figure(figsize=(12, 12))

# plt.pcolormesh(X, Y, Z, cmap='jet')
# plot a scatter but show the daqs as text
plt.scatter(X, Y, c=Z, cmap="jet", s=1)
plt.ylabel("y [mm]", fontsize=14)
plt.xlabel("x [mm]", fontsize=14)

plt.xlim(-side_limit, side_limit)
plt.ylim(-side_limit, side_limit)

plt.tight_layout()
plt.colorbar()

plt.savefig("micromegasReadout.png")

plt.show()
