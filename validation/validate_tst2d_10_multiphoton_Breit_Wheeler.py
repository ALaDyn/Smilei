# ______________________________________________________________________________
#
# Validation script for the benchmark tst1d_10_multiphoton_Breit_Wheeler
#
# Descrition:
# Interaction between a bunch of photons and a constant magnetic field.
#
# Purpose:
# During the interaction,the photons will progressively decay into pairs
# of electron-positron via the multiphoton Breit-Wheeler process.
# The radiation reaction of particles is not activated.
#
# Validation:
# - Propagation of macro-photons
# - multiphoton Breit-Wheeler pair creation process
#
# ______________________________________________________________________________

import os, re, numpy as np, h5py
from Smilei import *

S = Smilei(".", verbose=False)

dt = S.namelist.Main.timestep
dx = S.namelist.Main.cell_length[0]
dy = S.namelist.Main.cell_length[1]

# ______________________________________________________________________________
# Read scalar diagnostics

times = np.array(S.Scalar("Ukin_electron").get()["times"])
ukin_electron = np.array(S.Scalar("Ukin_electron").get()["data"])
ukin_positron = np.array(S.Scalar("Ukin_positron").get()["data"])
ukin_photon = np.array(S.Scalar("Ukin_photon").get()["data"])

utot = ukin_electron + ukin_positron + ukin_photon

ntot_electron = np.array(S.Scalar("Ntot_electron").get()["data"])
ntot_positron = np.array(S.Scalar("Ntot_positron").get()["data"])
ntot_photon = np.array(S.Scalar("Ntot_photon").get()["data"])

print ' Final electron energy / total energy: ',ukin_electron[-1] / utot[0]
print ' Final positron energy / total energy: ',ukin_positron[-1] / utot[0]
print ' Final photon energy / total energy: ',ukin_photon[-1] / utot[0]
print ' Maximal relative error total energy: ', max(abs(utot[:] - utot[0]))/utot[0]

print ' Final number of electrons:',ntot_electron[-1]
print ' Final number of positron: ',ntot_positron[-1]
print ' Final number of photon: ',ntot_photon[-1]

Validate("Electron kinetic energy evolution: ", ukin_electron/utot[0], 5e-3 )
Validate("Positron kinetic energy evolution: ", ukin_positron/utot[0], 5e-3 )
Validate("Photon kinetic energy evolution: ", ukin_photon/utot[0], 5e-3 )
Validate("Maximal relative error total energy: ", max(abs(utot[:] - utot[0]))/utot[0], 5e-3 )

# ______________________________________________________________________________
# Read energy spectrum

PartDiag = S.ParticleDiagnostic(diagNumber=0,timesteps = 2000)
gamma = np.array(PartDiag.get()["gamma"])
density = np.array(PartDiag.get()["data"][0])
integral = sum(density)*(gamma[1] - gamma[0])

print ' Electron energy from spectrum: ',integral
print ' Max from spectrum: ',max(density/integral)

Validate("Electron energy spectrum: ", density/integral, 1e-5 )

PartDiag = S.ParticleDiagnostic(diagNumber=1,timesteps = 2000)
gamma = np.array(PartDiag.get()["gamma"])
density = np.array(PartDiag.get()["data"][0])
integral = sum(density)*(gamma[1] - gamma[0])

print ' Positron energy from spectrum: ',integral
print ' Max from spectrum: ',max(density/integral)

Validate("Positron energy spectrum: ", density/integral, 1e-5 )
