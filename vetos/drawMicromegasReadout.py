import REST
import ROOT

import numpy as np
import matplotlib.pyplot as plt
import tqdm as tqdm
import matplotlib.colors as mcolors
from matplotlib.colors import LinearSegmentedColormap

file = ROOT.TFile("fullReadout.root")

readout = file.Get("fullReadout")

plane_index = 0  # this is the micromegas readout plane

readout_size = 60.0
side_limit = readout_size / 2 + 0.5
step = 0.02

print(f"side limit: {side_limit}, step: {step}")

x = np.arange(-side_limit, side_limit, step)
y = np.arange(-side_limit, side_limit, step)

X, Y = np.meshgrid(x, y)

Z = np.zeros((len(x), len(y)))

micromegas_plane_id = 0

channel_id_to_daq_id = {}
# use tqdm to show a progress bar
for i in tqdm.tqdm(range(len(x))):
    for j in range(len(y)):
        position = ROOT.TVector3(x[i], y[j], 0)
        daq_id, module_id, channel_id = readout.GetHitsDaqChannelAtReadoutPlane(position, micromegas_plane_id)
        Z[i][j] = channel_id
        channel_id_to_daq_id[channel_id] = daq_id

# Draw Z as a color map, excluding values of -1

Z[Z == -1] = np.nan

# count the unique values in Z (non nan)
unique, counts = np.unique(Z[~np.isnan(Z)], return_counts=True)

# print the number of unique values
print(f"number of unique values: {len(unique)}")

# save X, Y, Z to disk in a single file
np.savez_compressed('data.npz', X=X, Y=Y, Z=Z, channel_id_to_daq_id=channel_id_to_daq_id)

# save the channel to daq mapping to disk
np.save("channel_to_daq.npy", channel_id_to_daq_id)


def custom_colormap():
    colors = [
        (0.0, 'blue'),
        (0.5, 'yellow'),
        (0.5, 'magenta'),
        (1.0, 'green')
    ]
    cmap = LinearSegmentedColormap.from_list('custom_colormap', colors, N=240)
    return cmap


cmap = custom_colormap()

plt.figure(figsize=(14, 12))
plt.pcolormesh(X, Y, Z, cmap=cmap)

cbar = plt.colorbar(ticks=np.arange(0, 240, 10), format="%d", fraction=0.046, pad=0.04)  # Tick every 5 IDs
cbar.set_label("Channel", fontsize=14)

plt.xlabel("X [mm]", fontsize=14)
plt.ylabel("Y [mm]", fontsize=14)

plt.xlim(-side_limit, side_limit)
plt.ylim(-side_limit, side_limit)

plt.gca().set_aspect("equal", adjustable="box")
plt.tight_layout()

# save as a very high quality png
plt.savefig("readout.png", dpi=600)
