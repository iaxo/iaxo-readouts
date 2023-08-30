import REST
import ROOT

import numpy as np
import matplotlib.pyplot as plt
import tqdm as tqdm
import matplotlib.colors as mcolors

file = ROOT.TFile("fullReadout.root")

readout = file.Get("fullReadout")

plane_index = 0  # this is the micromegas readout plane

side_limit = 30.5
step = 0.01

x = np.arange(-side_limit, side_limit, step)
y = np.arange(-side_limit, side_limit, step)

X, Y = np.meshgrid(x, y)

Z = np.zeros((len(x), len(y)))

micromegas_plane_id = 0

# use tqdm to show a progress bar
for i in tqdm.tqdm(range(len(x))):
    for j in range(len(y)):
        position = ROOT.TVector3(x[i], y[j], 0)
        daq_id, module_id, channel_id = readout.GetHitsDaqChannelAtReadoutPlane(position, micromegas_plane_id)
        Z[i][j] = channel_id

# Draw Z as a color map, excluding values of -1

Z[Z == -1] = np.nan

# count the unique values in Z (non nan)
unique, counts = np.unique(Z[~np.isnan(Z)], return_counts=True)

# print the number of unique values
print(f"number of unique values: {len(unique)}")
# Create two separate colormaps for the two ranges
num_colors_range1 = 60  # 0 to 119
num_colors_range2 = 60  # 120 to 239
tab20_range1 = plt.cm.get_cmap("tab20", num_colors_range1)
tab20_range2 = plt.cm.get_cmap("tab20", num_colors_range2)
colormap_range1 = mcolors.ListedColormap([tab20_range1(i % num_colors_range1) for i in range(num_colors_range1)])
colormap_range2 = mcolors.ListedColormap([tab20_range2(i % num_colors_range2) for i in range(num_colors_range2)])

# Combine the two colormaps into a single colormap
combined_colormap = mcolors.ListedColormap(list(colormap_range1.colors) + list(colormap_range2.colors))

# Define the boundaries for the two ranges
boundaries = [-0.5, 119.5, 239.5]  # Adjusted to have 0 in the middle

# Normalize the values based on the boundaries
norm = mcolors.BoundaryNorm(boundaries, combined_colormap.N)

plt.figure(figsize=(14, 12))
plt.pcolormesh(X, Y, Z, cmap=combined_colormap, norm=norm)

cbar = plt.colorbar(ticks=np.arange(0, 240, 10), format="%d", fraction=0.046, pad=0.04)  # Tick every 5 IDs
cbar.set_label("Channel ID", fontsize=14)

plt.xlabel("X [mm]", fontsize=14)
plt.ylabel("Y [mm]", fontsize=14)

plt.xlim(-side_limit, side_limit)
plt.ylim(-side_limit, side_limit)

plt.gca().set_aspect("equal", adjustable="box")
plt.tight_layout()

plt.savefig("micromegasReadout.pdf")
