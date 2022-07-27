// ----------------------------------------------------------------------------
//! \file RadiationTables.cpp
//
//! \brief This class contains the tables and the functions to generate them
//! for the Nonlinear Inverse Compton Scattering
//
//! \details The implementation is adapted from the thesis results of M. Lobet
//! See http://www.theses.fr/2015BORD0361
// ----------------------------------------------------------------------------

#include "RadiationTables.h"
#include "RadiationTablesDefault.h"

// -----------------------------------------------------------------------------
// INITILIZATION AND DESTRUCTION
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Constructor for RadiationTables
// -----------------------------------------------------------------------------
RadiationTables::RadiationTables()
{

    // Default parameters
    minimum_chi_continuous_ = 1e-3;
    minimum_chi_discontinuous_ = 1e-2;
}

// -----------------------------------------------------------------------------
// Destructor for RadiationTables
// -----------------------------------------------------------------------------
RadiationTables::~RadiationTables()
{
}

// -----------------------------------------------------------------------------
//
//! Initialization of the parameters for the nonlinear inverse Compton
//! scattering
//
//! \param params Object Params for the parameters from the input script
// -----------------------------------------------------------------------------
void RadiationTables::initialization( Params &params , SmileiMPI *smpi )
{

    if( params.has_MC_radiation_ ||
        params.has_LL_radiation_ ||
        params.has_Niel_radiation_||
        params.hasDiagRadiationSpectrum) {
        TITLE( "Initializing radiation reaction (or RadiationSpectrum parameters)" )

        // Preliminary checks
        if( params.reference_angular_frequency_SI <= 0. )
            ERROR_NAMELIST( "The parameter `reference_angular_frequency_SI` needs "
                   << "to be defined and positive to compute radiation losses",
                   LINK_NAMELIST + std::string("#radiation-reaction") );

    }

    // If the namelist for Nonlinear Inverse Compton Scattering exists
    // We read the properties
    if( PyTools::nComponents( "RadiationReaction" ) != 0 ) {

        if( params.has_LL_radiation_ || 
            params.hasDiagRadiationSpectrum || 
            params.has_Niel_radiation_ || 
            params.has_MC_radiation_  ) {
            // Minimum threshold on chi to allow continuous radiation
            PyTools::extract( "minimum_chi_continuous", minimum_chi_continuous_, "RadiationReaction"  );
        }
        
        // If Monte-Carlo radiation loss is requested
        if( params.has_MC_radiation_ ) {
            // Discontinuous minimum threshold
            PyTools::extract( "minimum_chi_discontinuous", minimum_chi_discontinuous_, "RadiationReaction"  );
        }

        if( params.has_Niel_radiation_ ) {
            // How to handle the h function (table or fit)
            PyTools::extract( "Niel_computation_method", niel_computation_method_, "RadiationReaction" );
        }

        if( params.has_Niel_radiation_ || params.has_MC_radiation_ ) {
            // Path to the databases
            PyTools::extract( "table_path", table_path_, "RadiationReaction"  );
        }
    }

    // Computation of some parameters
    if( params.has_MC_radiation_ ||
            params.has_LL_radiation_ ||
            params.has_Niel_radiation_ ) {

        // Computation of the normalized Compton wavelength
        normalized_Compton_wavelength_ = params.red_planck_cst*params.reference_angular_frequency_SI
                                         / ( params.electron_mass*params.c_vacuum_*params.c_vacuum_ );

        // Computation of the factor factor_dNph_dt_
        factor_dNph_dt_ = std::sqrt( 3.0 )*params.fine_struct_cst/( 2.0*M_PI*normalized_Compton_wavelength_ );

        // Computation of the factor for the classical radiated power
        factor_classical_radiated_power_ = 2.*params.fine_struct_cst/( 3.*normalized_Compton_wavelength_ );
    }

    if( params.has_Niel_radiation_ ) {
        if( niel_computation_method_ == "table" ||
                niel_computation_method_ == "fit5"  ||
                niel_computation_method_ == "fit10" ||
                niel_computation_method_ == "ridgers" ) {
        } else {
            ERROR_NAMELIST( 
                " The parameter `Niel_computation_method` must be `table`, `fit5`, `fit10` or `ridgers`.",
                LINK_NAMELIST + std::string("#radiation-reaction")
            );
        }

        // Convert computational methods in index for GPUs
        if( niel_computation_method_ == "table") {
            niel_computation_method_index_ = 0;
        } else if (niel_computation_method_ == "fit5") {
            niel_computation_method_index_ = 1;
        } else if (niel_computation_method_ == "fit10") {
            niel_computation_method_index_ = 2;
        } else if (niel_computation_method_ == "ridgers") {
            niel_computation_method_index_ = 3;
        }

    }

    MESSAGE( "" );

    if( params.has_LL_radiation_ || params.hasDiagRadiationSpectrum ) {
        MESSAGE( 1, "A continuous radiation reaction module"
                 << " is requested by some species:" );
        MESSAGE( 2, "- applied minimum chi for continuous radiation module is "
                <<std::setprecision(6)<<minimum_chi_continuous_<<".");
        MESSAGE( 2, "- factor classical radiated power: " << factor_classical_radiated_power_ );
        MESSAGE( "" );
    }

    if( params.has_Niel_radiation_ ) {
        MESSAGE( 1,"The Fokker-Planck radiation reaction module 'Niel'"
                 << " is requested by some species:" );
        MESSAGE( 2,"- applied minimum chi for Niel's radiation module is "
                <<std::setprecision(6)<<minimum_chi_continuous_<<".");
        MESSAGE( 2,"- Niel h function computation method: " << niel_computation_method_<< ".");
        MESSAGE( 2,"- table path: " << table_path_ );
        MESSAGE( "" );
    }

    if( params.has_MC_radiation_ ) {
        MESSAGE( 1,"The Monte-Carlo Compton radiation module"
                 << " is requested by some species:" );
        MESSAGE( 2,"- applied minimum chi for Monte-Carlo radiation module is "
                <<std::setprecision(6)<<minimum_chi_continuous_<<".");
        MESSAGE( 2,"- applied minimum chi for MC radiation module is "
                 <<std::setprecision(6)<<minimum_chi_discontinuous_<<".");
        MESSAGE( 2,"- table path: " << table_path_ );

    }

    MESSAGE( "" );

    // We read the table only if specified
    if( params.has_MC_radiation_ || params.has_Niel_radiation_ ) {
        if (table_path_.size() > 0) {
            MESSAGE( 1,"Reading of the external database" );
            readTables( params, smpi );
        } else {
            MESSAGE(1,"Default tables (stored in the code) are used:");
            RadiationTablesDefault::setDefault( niel_, integfochi_, xi_ );
        }
    }

    if( params.has_MC_radiation_ ) {
        MESSAGE( "" );
        MESSAGE( 1,"--- Integration F/particle_chi table:" );
        MESSAGE( 2,"Dimension quantum parameter: " << integfochi_.size_ );
        MESSAGE( 2,"Minimum particle quantum parameter chi: " << integfochi_.min_ );
        MESSAGE( 2,"Maximum particle quantum parameter chi: " << integfochi_.max_ );
        MESSAGE( "" );
        MESSAGE( 1,"--- Table `min_photon_chi_for_xi` and `xi`:" );
        MESSAGE( 2,"Dimension particle chi: " << xi_.size_particle_chi_ );
        MESSAGE( 2,"Dimension photon chi: " << xi_.size_photon_chi_ );
        MESSAGE( 2,"Minimum particle chi: " << xi_.min_particle_chi_ );
        MESSAGE( 2,"Maximum particle chi: " << xi_.max_particle_chi_ );
    }

    // if( params.has_Niel_radiation_ ) {
    //     MESSAGE( "" );
    //     MESSAGE( 1, "--- `h` table for the model of Niel et al.:" );
    //     MESSAGE( 2,"Dimension quantum parameter: "
    //              << niel_.size_particle_chi_ );
    //     MESSAGE( 2,"Minimum particle quantum parameter chi: "
    //              << niel_.min_particle_chi_ );
    //     MESSAGE( 2,"Maximum particle quantum parameter chi: "
    //              << niel_.max_particle_chi_ );
    // }
    
    if( params.has_Niel_radiation_ ) {
        MESSAGE( "" );
        MESSAGE( 1, "--- `h` table for the model of Niel et al.:" );
        MESSAGE( 2,"Dimension quantum parameter: "
                 << niel_.size_ );
        MESSAGE( 2,"Minimum particle quantum parameter chi: "
                 << niel_.min_ );
        MESSAGE( 2,"Maximum particle quantum parameter chi: "
                 << niel_.max_ );
    }
}


// -----------------------------------------------------------------------------
// PHYSICAL COMPUTATION
// -----------------------------------------------------------------------------

// ---------------------------------------------------------------------------------------------------------------------
//! Computation of the Cross Section dNph/dt which is also
//! the number of photons generated per time unit.
//
//! param[in] particle_chi particle quantum parameter
//! param[in] particle_gamma particle Lorentz factor
//! param[in] integfochi table of the discretized integrated f/chi function for Photon production yield computation
// ---------------------------------------------------------------------------------------------------------------------
double RadiationTables::computePhotonProductionYield( 
                                            const double particle_chi, 
                                            const double particle_gamma)
{
    // final value
    double dNphdt;

    // Log of the particle quantum parameter particle_chi
    const double logchipa = std::log10( particle_chi );

    // Lower index for interpolation in the table integfochi_
    int ichipa = int( std::floor( ( logchipa-integfochi_.log10_min_ )
                         *integfochi_.inv_delta_ ) );

    // If we are not in the table...
    if( ichipa < 0 ) {
        ichipa = 0;
        dNphdt = integfochi_.data_[ichipa];
    } else if( ichipa >= integfochi_.size_-1 ) {
        ichipa = integfochi_.size_-2;
        dNphdt = integfochi_.data_[ichipa];
    } else {
        // Upper and lower values for linear interpolation
        const double logchipam = ichipa*integfochi_.delta_ + integfochi_.log10_min_;
        const double logchipap = logchipam + integfochi_.delta_;

        // Interpolation
        dNphdt = ( integfochi_.data_[ichipa+1]*std::fabs( logchipa-logchipam ) +
                   integfochi_.data_[ichipa]*std::fabs( logchipap - logchipa ) )*integfochi_.inv_delta_;
    }
    return factor_dNph_dt_*dNphdt*particle_chi/particle_gamma;
}

// -----------------------------------------------------------------------------
//! Computation of the photon quantum parameter photon_chi for emission
//! ramdomly and using the tables xi and chiphmin
//
//! \param particle_chi particle quantum parameter
// -----------------------------------------------------------------------------
// double RadiationTables::computeRandomPhotonChi( double particle_chi )
// {
//     // Log10 of particle_chi
//     double logchipa;
//     double photon_chi;
//     double chiph_xip_delta;
//     // Random xi
//     double xi;
//     int ichipa;
//     int ichiph;
//     // For the interpolation
//     double log10_chiphm;
//     double log10_chiphp;
//     double d;
//     int ixip;
//
//     logchipa = std::log10( particle_chi );
//
//     // ---------------------------------------
//     // index of particle_chi in xi_.table
//     // ---------------------------------------
//     // Use floor so that particle_chi corresponding to ichipa is <= given particle_chi
//     ichipa = int( floor( ( logchipa-xi_.log10_min_particle_chi_ )*( xi_.inv_particle_chi_delta_ ) ) );
//
//     // Checking that ichipa is in the range of the tables
//     // Else we use the values at the boundaries
//     if( ichipa < 0 ) {
//         ichipa = 0;
//     } else if( ichipa > xi_.size_particle_chi_-1 ) {
//         ichipa = xi_.size_particle_chi_-1;
//     }
//
//     // ---------------------------------------
//     // Search of the index ichiph for photon_chi
//     // ---------------------------------------
//
//     // First, we compute a random xi in [0,1[
//     xi = Rand::uniform();
//
//     // If the randomly computed xi if below the first one of the row,
//     // we take the first one which corresponds to the minimal photon photon_chi
//     if( xi <= xi_.table_[ichipa*xi_.size_photon_chi_] ) {
//         ichiph = 0;
//         xi = xi_.table_[ichipa*xi_.size_photon_chi_];
//     }
//     // Above the last xi of the row, the last one corresponds
//     // to the maximal photon photon_chi
//     else if( xi > xi_.table_[( ichipa+1 )*xi_.size_photon_chi_-2] ) {
//         ichiph = xi_.size_photon_chi_-2;
//         xi = xi_.table_[( ichipa+1 )*xi_.size_photon_chi_-1];
//         // If nearest point: ichiph = xi_.size_photon_chi_-1
//     } else {
//         // Search for the corresponding index ichiph for xi
//         ichiph = userFunctions::searchValuesInMonotonicArray(
//                      &xi_.table_[ichipa*xi_.size_photon_chi_], xi, xi_.size_photon_chi_ );
//     }
//
//     // Corresponding particle_chi for ichipa
//     logchipa = ichipa*xi_.particle_chi_delta_+xi_.log10_min_particle_chi_;
//
//     // Delta for the corresponding particle_chi
//     chiph_xip_delta = ( logchipa - xi_.min_photon_chi_table_[ichipa] )
//                       *xi_.inv_size_photon_chi_minus_one_;
//
//     // --------------------------------------------------------------------
//     // Compute photon_chi
//     // This method is slow but more accurate than taking the nearest point
//     // --------------------------------------------------------------------
//
//     ixip = ichipa*xi_.size_photon_chi_ + ichiph;
//
//     // Computation of the final photon_chi by interpolation
//     if( xi_.table_[ixip+1] - xi_.table_[ixip] > 1e-15 ) {
//         log10_chiphm = ichiph*chiph_xip_delta
//                        + xi_.min_photon_chi_table_[ichipa];
//         log10_chiphp = log10_chiphm + chiph_xip_delta;
//
//         d = ( xi - xi_.table_[ixip] ) / ( xi_.table_[ixip+1] - xi_.table_[ixip] );
//
//         // Chiph after linear interpolation in the logarithmic scale
//         photon_chi = std::pow( 10.0, log10_chiphm*( 1.0-d ) + log10_chiphp*( d ) );
//     } else
//         // For integration reasons, we can have xi_.table_[ixip+1] = xi_.table_[ixip]
//         // In this case, no interpolation
//     {
//         photon_chi = pow( 10., ichiph*chiph_xip_delta
//                           + xi_.min_photon_chi_table_[ichipa] );
//     }
//
//     // ------------------------------------------------------------
//     // Compute photon_chi
//     // Fastest method using the nearest point but less accurate
//     // ------------------------------------------------------------
//
//     /*photon_chi = pow(10.,ichiph*chiph_xip_delta
//            + xi_.min_photon_chi_table_[ichipa]);*/
//
//
//     return photon_chi;
// }


// -----------------------------------------------------------------------------
//! Computation of the photon quantum parameter photon_chi for emission
//! ramdomly and using the tables xi and chiphmin
//
//! \param[in] particle_chi particle quantum parameter
//! \param[in] xi
//! \param[in] table_min_photon_chi
//! \param[in] table_xi
// -----------------------------------------------------------------------------
double RadiationTables::computeRandomPhotonChiWithInterpolation( double particle_chi, double xi,
                                                const double *const table_min_photon_chi, 
                                                double * table_xi)
{

    // Photon chi (1 and 2 for interpolation)
    double photon_chi;
    double photon_chi_1;
    double photon_chi_2;

    // Random xi
    //double xi;

    //double chiph_xip_delta;
    double chiph_xip_delta_2;

    int ichiph_1;
    int ichiph_2;
    // For the interpolation
    double log10_chiphm;
    double log10_chiphp;
    double d_photon_chi;
    int ixip_1;
    int ixip_2;

    // Log10 of particle_chi
    const double log10_particle_chi = std::log10( particle_chi );

    // ---------------------------------------
    // index of particle_chi in xi_.table
    // ---------------------------------------

    // Use floor so that particle_chi corresponding to ichipa is <= given particle_chi
    int ichipa = int( std::floor( ( log10_particle_chi-xi_.log10_min_particle_chi_ )*( xi_.inv_particle_chi_delta_ ) ) );

    // Checking that ichipa is in the range of the tables
    // Else we use the values at the boundaries
    if( ichipa < 0 ) {
        ichipa = 0;
    } else if( ichipa > xi_.size_particle_chi_-2 ) {
        // xi_.size_particle_chi_-2 for interpolation
        ichipa = xi_.size_particle_chi_-2;
    }

    // ---------------------------------------
    // Search of the index ichiph for photon_chi
    // ---------------------------------------

    //xi = rand->uniform();

    // If the randomly computed xi if below the first one of the row,
    // we take the first one which corresponds to the minimal photon photon_chi
    if( xi <= table_xi[ichipa*xi_.size_photon_chi_] ) {
        ichiph_1 = 0;
        ichiph_2 = 0;
        xi = table_xi[ichipa*xi_.size_photon_chi_];
    }
    // Above the last xi of the row, the last one corresponds
    // to the maximal photon photon_chi
    // else if( xi > table_xi[( ichipa+1 )*xi_.size_photon_chi_-2] ) {
    //     ichiph = xi_.size_photon_chi_-2;
    //     xi = table_xi[( ichipa+1 )*xi_.size_photon_chi_-1];
    // If nearest point: ichiph = xi_.size_photon_chi_-1
    // }
    else {
        // Search for the corresponding index ichiph for xi
        ichiph_1 = userFunctions::searchValuesInMonotonicArray(
                     &table_xi[ichipa*xi_.size_photon_chi_], xi, xi_.size_photon_chi_ );
        ichiph_2 = userFunctions::searchValuesInMonotonicArray(
                  &table_xi[(ichipa+1)*xi_.size_photon_chi_], xi, xi_.size_photon_chi_ );
    }

    // Corresponding particle_chi for ichipa
    // log10_particle_chi = ichipa*xi_.particle_chi_delta_+xi_.log10_min_particle_chi_;
    const double d_particle_chi = (log10_particle_chi - (ichipa*xi_.particle_chi_delta_+xi_.log10_min_particle_chi_))
                       * xi_.inv_particle_chi_delta_;

     /*std::cerr << " " << log10_particle_chi
               << " " <<  ichipa*xi_.particle_chi_delta_+xi_.log10_min_particle_chi_
               << " " << xi_.inv_particle_chi_delta_
               << " " << xi_.particle_chi_delta_
               << " " << xi_.log10_min_particle_chi_
               << " " << ichipa
               << " " << d_particle_chi
               << " " << (log10_particle_chi - ichipa*xi_.particle_chi_delta_+xi_.log10_min_particle_chi_)
               << std::endl;*/

    // double log10_min_photon_chi =  table_min_photon_chi[ichipa] * (d_particle_chi - 1.0)
    //                             + d_particle_chi*table_min_photon_chi[ichipa+1];

    // Chi gap for the corresponding particle_chi

    const double chiph_xip_delta_1 = ( ichipa*xi_.particle_chi_delta_+xi_.log10_min_particle_chi_ - table_min_photon_chi[ichipa])
                      *xi_.inv_size_photon_chi_minus_one_;

    chiph_xip_delta_2 = ( (ichipa+1)*xi_.particle_chi_delta_+xi_.log10_min_particle_chi_ - table_min_photon_chi[ichipa+1])
                      *xi_.inv_size_photon_chi_minus_one_;

    // --------------------------------------------------------------------
    // Compute photon_chi
    // This method is slow but more accurate than taking the nearest point
    // --------------------------------------------------------------------

    ixip_1 = ichipa*xi_.size_photon_chi_ + ichiph_1;
    ixip_2 = (ichipa+1)*xi_.size_photon_chi_ + ichiph_2;

    // Computation of the final photon_chi by interpolation
    if( (table_xi[ixip_1] < 1.0) && (table_xi[ixip_1+1] - table_xi[ixip_1] > 1e-15) ) {

        log10_chiphm = ichiph_1*chiph_xip_delta_1
                       + table_min_photon_chi[ichipa];

        log10_chiphp = log10_chiphm + chiph_xip_delta_1;

        d_photon_chi = ( xi - table_xi[ixip_1] ) / ( table_xi[ixip_1+1] - table_xi[ixip_1] );

        // Chiph after linear interpolation in the logarithmic scale
        photon_chi_1 = log10_chiphm*( 1.0-d_photon_chi ) + log10_chiphp*( d_photon_chi ) ;
    } else
    // For integration reasons, we can have table_xi[ixip+1] = table_xi[ixip]
    // In this case, no interpolation
    {
        photon_chi_1 = ichiph_1*chiph_xip_delta_1
                          + table_min_photon_chi[ichipa] ;
    }

    // Computation of the final photon_chi by interpolation
    if( (table_xi[ixip_2] < 1.0) && (table_xi[ixip_2+1] - table_xi[ixip_2] > 1e-15) ) {
        log10_chiphm = ichiph_2*chiph_xip_delta_2
                       + table_min_photon_chi[ichipa+1];
        log10_chiphp = log10_chiphm + chiph_xip_delta_2;

        d_photon_chi = ( xi - table_xi[ixip_2] ) / ( table_xi[ixip_2+1] - table_xi[ixip_2] );

        // Chiph after linear interpolation in the logarithmic scale
        photon_chi_2 = log10_chiphm*( 1.0-d_photon_chi ) + log10_chiphp*( d_photon_chi ) ;
    } else
    // For integration reasons, we can have table_xi[ixip+1] = table_xi[ixip]
    // In this case, no interpolation
    {
        photon_chi_2 = ichiph_2*chiph_xip_delta_2
                          + table_min_photon_chi[ichipa+1] ;
    }

    // Chiph after linear interpolation in the logarithmic scale
    photon_chi = std::pow( 10.0, photon_chi_1*(1 - d_particle_chi) + photon_chi_2*d_particle_chi);
    //photon_chi = std::pow( 10.0, photon_chi_1*(1));
    //photon_chi = 0;
    return photon_chi;
}

// ---------------------------------------------------------------------------------------------------------------------
//! Computation of the Cross Section dNph/dt which is also
//! the number of photons generated per time unit.
//
//! param[in] particle_chi particle quantum parameter
//! param[in] particle_gamma particle Lorentz factor
//! param[in] integfochi table of the discretized integrated f/chi function for Photon production yield computation
// ---------------------------------------------------------------------------------------------------------------------
// double RadiationTables::computePhotonProductionYield( const double particle_chi, 
//                                                       const double particle_gamma, 
//                                                       const Table & integfochi)
// {
//     // final value
//     double dNphdt;
// 
//     // Log of the particle quantum parameter particle_chi
//     const double logchipa = std::log10( particle_chi );
// 
//     // Lower index for interpolation in the table integfochi_
//     int ichipa = int( std::floor( ( logchipa-integfochi.log10_min_ )
//                          *integfochi.inv_delta_ ) );
// 
//     // If we are not in the table...
//     if( ichipa < 0 ) {
//         ichipa = 0;
//         dNphdt = integfochi.data_[ichipa];
//     } else if( ichipa >= integfochi.size_-1 ) {
//         ichipa = integfochi.size_-2;
//         dNphdt = integfochi.data_[ichipa];
//     } else {
//         // Upper and lower values for linear interpolation
//         const double logchipam = ichipa*integfochi.delta_ + integfochi.log10_min_;
//         const double logchipap = logchipam + integfochi.delta_;
// 
//         // Interpolation
//         dNphdt = ( integfochi.data_[ichipa+1]*std::fabs( logchipa-logchipam ) +
//                    integfochi.data_[ichipa]*std::fabs( logchipap - logchipa ) )*integfochi.inv_delta_;
//     }
// 
//     return factor_dNph_dt_*dNphdt*particle_chi/particle_gamma;
// 
// }


// -----------------------------------------------------------------------------
//! Return the stochastic diffusive component of the pusher
//! of Niel et al.
//! \param gamma particle Lorentz factor
//! \param particle_chi particle quantum parameter
// -----------------------------------------------------------------------------
// double RadiationTables::getNielStochasticTerm( double gamma,
//         double particle_chi,
//         double sqrtdt,
//         Random * rand)
// {
//     // Get the value of h for the corresponding particle_chi
//     double h, r;
//
//     h = RadiationTables::getHNielFromTable( particle_chi );
//
//     // Pick a random number in the normal distribution of standard
//     // deviation sqrt(dt) (variance dt)
//     // r = Rand::normal( sqrtdt );
//     r = rand->normal() * sqrtdt;
//
//     /*std::random_device device;
//     std::mt19937 gen(device());
//     std::normal_distribution<double> normal_distribution(0., sqrt(dt));
//     r = normal_distribution(gen);*/
//
//     return sqrt( factor_classical_radiated_power_*gamma*h )*r;
// }

// -----------------------------------------------------------------------------
// TABLE READING
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//! Read the external table h
//
//! \param smpi Object of class SmileiMPI containing MPI properties
// -----------------------------------------------------------------------------
void RadiationTables::readHTable( SmileiMPI *smpi )
{
    
    std::string file = table_path_ + "/radiation_tables.h5";
    if( Tools::fileExists( file ) ) {
        if( smpi->isMaster() ) {
            H5Read f( file );

            // First, we read attributes
            H5Read h = f.dataset( "h" );
            // h.attr( "size_particle_chi", niel_.size_particle_chi_ );
            // h.attr( "min_particle_chi", niel_.min_particle_chi_ );
            // h.attr( "max_particle_chi", niel_.max_particle_chi_ );
            
            h.attr( "size_particle_chi", niel_.size_ );
            h.attr( "min_particle_chi", niel_.min_ );
            h.attr( "max_particle_chi", niel_.max_ );

            // Resize and read array
            // niel_.table_.resize( niel_.size_particle_chi_ );
            // f.vect( "h", niel_.table_ );
            
            niel_.allocate();
            f.vect( "h", niel_.data_[0], H5T_NATIVE_DOUBLE, 0, niel_.size_ );
            
        }
    } else {
        ERROR_NAMELIST("The table H could not be read from the provided path: `"
              << table_path_<<"`. Please check that the path is correct.",
              LINK_NAMELIST + std::string("#radiation-reaction"))
    }

    // checks
    if( minimum_chi_continuous_ < niel_.min_ ) {
        ERROR_NAMELIST( "Parameter `minimum_chi_continuous_` (="
               << minimum_chi_continuous_ << ") is below `niel_.min_particle_chi_` (="
               << niel_.min_ << "), the lower bound of the h table should be equal or below "
               << "the radiation threshold on chi.",
               LINK_NAMELIST + std::string("#radiation-reaction") )
    }

    // Bcast the table to all MPI ranks
    // RadiationTables::bcastHTable( smpi );
    niel_.bcast( smpi );
}

// -----------------------------------------------------------------------------
//! Read the external table integfochi
//
//! \param smpi Object of class SmileiMPI containing MPI properties
// -----------------------------------------------------------------------------
void RadiationTables::readIntegfochiTable( SmileiMPI *smpi )
{
    std::string file = table_path_ + "/radiation_tables.h5";
    if( Tools::fileExists( file ) ) {
        if( smpi->isMaster() ) {
            H5Read f( file );

            // First, we read attributes
            H5Read c = f.dataset( "integfochi" );
            c.attr( "size_particle_chi", integfochi_.size_ );
            c.attr( "min_particle_chi", integfochi_.min_ );
            c.attr( "max_particle_chi", integfochi_.max_ );

            // Resize and read array
            // integfochi_.table_.resize( integfochi_.size_ );
            // f.vect( "integfochi", integfochi_.table_ );

            integfochi_.allocate();
            f.vect( "integfochi", integfochi_.data_[0], H5T_NATIVE_DOUBLE, 0, integfochi_.size_ );

            
        }

        // Bcast the table to all MPI ranks
        integfochi_.bcast( smpi );
    }
    // Else, the table can not be found, we throw an error
    else {
        ERROR_NAMELIST("The table `integfochi` could not be read from the provided path: `"
              << table_path_<<"`. Please check that the path is correct.",
              LINK_NAMELIST + std::string("#radiation-reaction"))
    }
}

// -----------------------------------------------------------------------------
//! Read the external table xip_chiphmin and xi
//
//! \param smpi Object of class SmileiMPI containing MPI properties
// -----------------------------------------------------------------------------
void RadiationTables::readXiTable( SmileiMPI *smpi )
{
    std::string file = table_path_ + "/radiation_tables.h5";
    if( Tools::fileExists( file ) ) {
        if( smpi->isMaster() ) {
            H5Read f( file );

            // First, we read attributes
            H5Read xi = f.dataset( "xi" );
            xi.attr( "size_particle_chi", xi_.size_particle_chi_ );
            xi.attr( "size_photon_chi", xi_.size_photon_chi_ );
            xi.attr( "min_particle_chi", xi_.min_particle_chi_ );
            xi.attr( "max_particle_chi", xi_.max_particle_chi_ );

            // Allocate and read arrays
            xi_.min_photon_chi_table_.resize( xi_.size_particle_chi_ );
            xi_.table_.resize( xi_.size_particle_chi_*xi_.size_photon_chi_ );
            f.vect( "min_photon_chi_for_xi", xi_.min_photon_chi_table_ );
            f.vect( "xi", xi_.table_ );
        }

        // Bcast the table to all MPI ranks
        RadiationTables::bcastTableXi( smpi );

    }
    // Else, the table can not be found, we throw an error
    else {
        ERROR_NAMELIST("The table `xi` could not be read from the provided path: `"
              << table_path_<<"`. Please check that the path is correct.",
              LINK_NAMELIST + std::string("#radiation-reaction"))
    }
}

// -----------------------------------------------------------------------------
//! Read all tables
//
//! \param params list of simulation parameters
//! \param smpi MPI parameters
// -----------------------------------------------------------------------------
void RadiationTables::readTables( Params &params, SmileiMPI *smpi )
{
    // These tables are loaded only if if one species has Monte-Carlo Compton radiation
    // And if the h values are not computed from a numerical fit
    if( params.has_Niel_radiation_ && this->niel_computation_method_ == "table" ) {
        RadiationTables::readHTable( smpi );
    }
    if( params.has_MC_radiation_ ) {
        RadiationTables::readIntegfochiTable( smpi );
        RadiationTables::readXiTable( smpi );
    }
}

// -----------------------------------------------------------------------------
// TABLE COMMUNICATIONS
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//! Bcast of the external table xip_chiphmin and xi
//
//! \param smpi Object of class SmileiMPI containing MPI properties
// -----------------------------------------------------------------------------
void RadiationTables::bcastTableXi( SmileiMPI *smpi )
{
    // Position for MPI pack and unack
    int position = 0;
    // buffer size for MPI pack and unpack
    int buf_size = 0;

    // -------------------------------------------
    // Bcast of all the parameters
    // We pack everything in a buffer
    // -------------------------------------------

    // Compute the buffer size
    if( smpi->getRank() == 0 ) {
        MPI_Pack_size( 2, MPI_INT, smpi->world(), &position );
        buf_size = position;
        MPI_Pack_size( 2, MPI_DOUBLE, smpi->world(), &position );
        buf_size += position;
        MPI_Pack_size( xi_.size_particle_chi_, MPI_DOUBLE, smpi->world(),
                       &position );
        buf_size += position;
        MPI_Pack_size( xi_.size_particle_chi_*xi_.size_photon_chi_, MPI_DOUBLE,
                       smpi->world(), &position );
        buf_size += position;
    }

    //MESSAGE( 2,"Buffer size for MPI exchange: " << buf_size );

    // Exchange buf_size with all ranks
    MPI_Bcast( &buf_size, 1, MPI_INT, 0, smpi->world() );

    // Packet that will contain all parameters
    char *buffer = new char[buf_size];

    // Proc 0 packs
    if( smpi->getRank() == 0 ) {
        position = 0;
        MPI_Pack( &xi_.size_particle_chi_,
                  1, MPI_INT, buffer, buf_size, &position, smpi->world() );
        MPI_Pack( &xi_.size_photon_chi_,
                  1, MPI_INT, buffer, buf_size, &position, smpi->world() );
        MPI_Pack( &xi_.min_particle_chi_,
                  1, MPI_DOUBLE, buffer, buf_size, &position, smpi->world() );
        MPI_Pack( &xi_.max_particle_chi_,
                  1, MPI_DOUBLE, buffer, buf_size, &position, smpi->world() );

        MPI_Pack( &xi_.min_photon_chi_table_[0], xi_.size_particle_chi_,
                  MPI_DOUBLE, buffer, buf_size, &position, smpi->world() );

        MPI_Pack( &xi_.table_[0], xi_.size_particle_chi_*xi_.size_photon_chi_,
                  MPI_DOUBLE, buffer, buf_size, &position, smpi->world() );
    }

    // Bcast all parameters
    MPI_Bcast( &buffer[0], buf_size, MPI_PACKED, 0, smpi->world() );

    // Other ranks unpack
    if( smpi->getRank() != 0 ) {
        position = 0;
        MPI_Unpack( buffer, buf_size, &position,
                    &xi_.size_particle_chi_, 1, MPI_INT, smpi->world() );
        MPI_Unpack( buffer, buf_size, &position,
                    &xi_.size_photon_chi_, 1, MPI_INT, smpi->world() );
        MPI_Unpack( buffer, buf_size, &position,
                    &xi_.min_particle_chi_, 1, MPI_DOUBLE, smpi->world() );
        MPI_Unpack( buffer, buf_size, &position,
                    &xi_.max_particle_chi_, 1, MPI_DOUBLE, smpi->world() );

        // Resize tables before unpacking values
        xi_.min_photon_chi_table_.resize( xi_.size_particle_chi_ );
        xi_.table_.resize( xi_.size_particle_chi_*xi_.size_photon_chi_ );

        MPI_Unpack( buffer, buf_size, &position, &xi_.min_photon_chi_table_[0],
                    xi_.size_particle_chi_, MPI_DOUBLE, smpi->world() );

        MPI_Unpack( buffer, buf_size, &position, &xi_.table_[0],
                    xi_.size_particle_chi_*xi_.size_photon_chi_, MPI_DOUBLE, smpi->world() );

    }

    delete[] buffer;

    // Log10 of xi_.min_particle_chi_ for efficiency
    xi_.log10_min_particle_chi_ = std::log10( xi_.min_particle_chi_ );

    // Computation of the delta
    xi_.particle_chi_delta_ = ( std::log10( xi_.max_particle_chi_ )
                        - xi_.log10_min_particle_chi_ )/( xi_.size_particle_chi_-1 );

    // Inverse of delta
    xi_.inv_particle_chi_delta_ = 1.0/xi_.particle_chi_delta_;

    // Inverse photon_chi discetization (regularly used)
    xi_.inv_size_photon_chi_minus_one_ = 1.0/( xi_.size_photon_chi_ - 1. );

    // -----------------------------------------------------------------------
    // DEBUG
    //
    // double sum_xi = 0;
    // double sum_min_photon_chi = 0;
    // for (int i = 0 ; i < xi_.size_particle_chi_ ; i++) {
    //     sum_min_photon_chi += xi_.min_photon_chi_table_[i];
    // }
    // for (int i = 0 ; i < xi_.size_particle_chi_*xi_.size_photon_chi_ ; i++) {
    //     sum_xi += xi_.table_[i];
    // }
    // for (int i = 0 ; i < smpi->getSize() ; i++) {
    //     if( smpi->getRank() == i ) {
    //         std::cerr << " rank: " << smpi->getRank()
    //                   << " xi_.size_particle_chi_: " << xi_.size_particle_chi_
    //                   << " xi_.size_photon_chi_: " << xi_.size_photon_chi_
    //                   << " xi_.min_particle_chi_: " << xi_.min_particle_chi_
    //                   << " xi_.max_particle_chi_: " << xi_.max_particle_chi_
    //                   << " xi_.log10_min_particle_chi_: " << xi_.log10_min_particle_chi_
    //                   << " xi_.particle_chi_delta_: " << xi_.particle_chi_delta_
    //                   << " xi_.inv_particle_chi_delta_: " << xi_.inv_particle_chi_delta_
    //                   << " xi_.inv_size_photon_chi_minus_one_: " << xi_.inv_size_photon_chi_minus_one_
    //                   << " sum min_photon_chi: " << sum_min_photon_chi
    //                   << " sum xi: " << sum_xi
    //                   << std::endl;
    //         usleep(1000);
    //         smpi->barrier();
    //     }
    // }
    // -----------------------------------------------------------------------
}
