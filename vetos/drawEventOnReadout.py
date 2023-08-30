import numpy as np
import matplotlib.pyplot as plt

# load the readout from disk
X, Y, Z, channel_id_to_daq_id = np.load("data.npz", allow_pickle=True).values()

# channel_id, energy
events = [
    (82, 2.31),
    (83, 2.31),
    (150, 3.21),
    (154, 3.21),
]

# event_data = Z.copy() * 0.0
# event_data has the same shape as Z, but is filled with zeros
event_data = np.zeros_like(Z)

for channel_id, energy in events:
    event_data[Z == channel_id] += energy

event_data[event_data == 0] = np.nan

# set a color map
cmap = plt.cm.jet
cmap.set_bad(color="white")
# 0 is white

cmap.set_under(color="white")

plt.figure(figsize=(14, 12))
# under 0.0 is white
plt.pcolormesh(X, Y, event_data, cmap="viridis", vmin=0)

plt.xlabel("X [mm]", fontsize=14)
plt.ylabel("Y [mm]", fontsize=14)

plt.xlim(np.min(X), np.max(X))
plt.ylim(np.min(Y), np.max(Y))

cbar = plt.colorbar(label="Energy [keV]", format="%2.2f", fraction=0.046,
                    pad=0.04)

# draw a rectangle 60x60 centered at 0,0
readout_side = 60.0
plt.gca().add_patch(
    plt.Rectangle((-readout_side / 2, -readout_side / 2), readout_side, readout_side, fill=False, linewidth=0.5,
                  edgecolor="black"))

plt.gca().set_aspect("equal", adjustable="box")
plt.tight_layout()

# save as a very high quality png
plt.savefig("event.png", dpi=600)
