import os, re, numpy as np, math, h5py
import happi

S = happi.Open(["./restart*"], verbose=False)

# COMPARE THE Envelope FIELD, absolute value
Env_A_abs = S.Field.Field0.Env_A_abs(subset={"z":S.namelist.Main.grid_length[2]/2, "x":[340,540,2] }, timesteps=8400.).getData()[0]
Validate("Env_A_abs field at iteration 8400", Env_A_abs, 0.01)

# 1-D PROBE IN 3D
Env_A_abs = S.Probe.Probe0.Env_A_abs(timesteps=8400).getData()[0]
Validate("1-D probe Ey at iteration 8400", Env_A_abs, 0.01)

# TEST THE GRID PARAMETERS
with h5py.File("./restart000/Fields0.h5") as f:
	dt = f["data/0000000000"].attrs["dt"]
	dx = f["data/0000000000/Ex"].attrs["gridSpacing"]
	patchSize = f["data/0000000000"].attrs["patchSize"]
Validate("Value of the timestep" , dt, 1e-6)
Validate("Value of the grid step", dx, 1e-6)
Validate("Patch size", patchSize)
