// ----------------------------------------------------------------------------
//! \file MultiphotonBreitWheeler.h
//
//! \brief This file contains the class functions for the generic class
//!  MultiphotonBreitWheeler for the photon decay into pairs via the mutliphoton
//!  Breit-Wheeler process.
//
// ----------------------------------------------------------------------------

#ifndef MULTIPHOTONBREITWHEELER_H
#define MULTIPHOTONBREITWHEELER_H

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#include "MultiphotonBreitWheelerTables.h"
#include "Params.h"

//  ----------------------------------------------------------------------------
//! Class Radiation
//  ----------------------------------------------------------------------------
class MultiphotonBreitWheeler
{
    public:

        //! Creator for Radiation
        MultiphotonBreitWheeler(Params& params, Species *species);
        ~MultiphotonBreitWheeler();

        //! Overloading of () operator
        //! \param particles   particle object containing the particle
        //!                    properties of the current species
        //! \param smpi        MPI properties
        //! \param MultiphotonBreitWheelerTables Cross-section data tables and useful functions
        //                     for multiphoton Breit-Wheeler
        //! \param istart      Index of the first particle
        //! \param iend        Index of the last particle
        //! \param ithread     Thread index
        void operator() (
                Particles &particles,
                SmileiMPI* smpi,
                MultiphotonBreitWheelerTables &MultiphotonBreitWheelerTables,
                int istart,
                int iend,
                int ithread);

        //! Computation of the photon Lorentz invariant quantum parameter
        //! for the given photon properties
        //! \param kx photon x momentum
        //! \param ky photon y momentum
        //! \param kz photon z momentum
        //! \param gamma photon Lorentz factor
        //! \param Ex x component of the particle electric field
        //! \param Ey y component of the particle electric field
        //! \param Ez z component of the particle electric field
        //! \param Bx x component of the particle magnetic field
        //! \param By y component of the particle magnetic field
        //! \param Bz z component of the particle magnetic field
        //#pragma omp declare simd
        double inline compute_chiph(double & kx, double & ky, double & kz,
                                    double & gamma,
                                    double & Ex, double & Ey, double & Ez,
                                    double & Bx, double & By, double & Bz)
        {

            return inv_norm_E_Schwinger
                  * sqrt( fabs( pow(Ex*kx + Ey*ky + Ez*kz,2)
                  - pow(gamma*Ex - By*kz + Bz*ky,2)
                  - pow(gamma*Ey - Bz*kx + Bx*kz,2)
                  - pow(gamma*Ez - Bx*ky + By*kx,2)));
        };

        //! Computation of the quantum parameter for the given
        //! thread of photons
        //! \param Particles class containg the particle property arrays
        //! \param smpi class for mpi parameters
        //! \param istart      Index of the first particle
        //! \param iend        Index of the last particle
        //! \param ithread     Thread index
        void compute_thread_chiph(Particles &particles,
                SmileiMPI* smpi,
                int istart,
                int iend,
                int ithread);

        //! Perform the creation of pairs from a photon
        //! \param ipart              photon index
        //! \param chipa              photon quantum parameter
        //! \param gammapa            photon normalized energy
        //! \param position           photon position
        //! \param momentum           photon momentum
        //! \param MultiphotonBreitWheelerTables    Cross-section data tables
        //!                       and useful functions
        //!                       for the multiphoton Breit-Wheeler process
        void pair_emission(int ipart,
                           double &chiph,
                           double & gammaph,
                            double * position[3],
                            double * momentum[3],
                            double * weight,
                            MultiphotonBreitWheelerTables &MultiphotonBreitWheelerTables);

        //! Clean photons that decayed into pairs (weight <= 0)
        //! \param particles   particle object containing the particle
        //!                    properties of the current species
        //! \param istart      Index of the first particle
        //! \param iend        Index of the last particle
        //! \param ithread     Thread index
        void decayed_photon_cleaning(
                        Particles &particles,
                        int istart,
                        int iend);

        //! Return the pair converted energy
        double inline getPairEnergy(void)
        {
            return pair_converted_energy;
        }

        // Local array of new pairs of electron-positron
        Particles new_pair[2];

    private:

        // ________________________________________
        // General parameters

        //! Dimension of position
        int nDim_;

        //! Time step
        double dt;

        // Number of pairs created per even
        int mBW_pair_creation_sampling[2];

        // Inverse of the number of pairs created per even
        double mBW_pair_creation_inv_sampling[2];

        //! Energy lost after conversion into pairs
        double pair_converted_energy;

        //! Threshold under which pair creation is not considered
        double chiph_threashold;

        // _________________________________________
        // Factors

        //! Normalized Schwinger Electric field
        double norm_E_Schwinger;

        //! Inverse Normalized Schwinger Electric field
        double inv_norm_E_Schwinger;

        //! Espilon to check when tau is near 0
        const double epsilon_tau = 1e-100;

};

#endif
