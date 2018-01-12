////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////                                                                                                                ////
////                                                                                                                ////
////                                   PARTICLE-IN-CELL CODE SMILEI                                                 ////
////                    Simulation of Matter Irradiated by Laser at Extreme Intensity                               ////
////                                                                                                                ////
////                          Cooperative OpenSource Object-Oriented Project                                        ////
////                                      from the Plateau de Saclay                                                ////
////                                          started January 2013                                                  ////
////                                                                                                                ////
////                                                                                                                ////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <ctime>
#include <cstdlib>
#include <unistd.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <omp.h>

#include "Smilei.h"
#include "SmileiMPI_test.h"
#include "Params.h"
#include "PatchesFactory.h"
#include "Checkpoint.h"
#include "Solver.h"
#include "SimWindow.h"
#include "Diagnostic.h"
#include "Timers.h"
#include "RadiationTables.h"
#include "MultiphotonBreitWheelerTables.h"

using namespace std;

// ---------------------------------------------------------------------------------------------------------------------
//                                                   MAIN CODE
// ---------------------------------------------------------------------------------------------------------------------
int main (int argc, char* argv[])
{
    cout.setf( ios::fixed,  ios::floatfield ); // floatfield set to fixed

    // -------------------------
    // Simulation Initialization
    // -------------------------

    // Create MPI environment :

#ifdef SMILEI_TESTMODE
    SmileiMPI_test smpi( &argc, &argv );
#else
    SmileiMPI smpi(&argc, &argv );
#endif

    MESSAGE("                   _            _");
    MESSAGE(" ___           _  | |        _  \\ \\   Version : " << __VERSION);
    MESSAGE("/ __|  _ __   (_) | |  ___  (_)  | |   ");
    MESSAGE("\\__ \\ | '  \\   _  | | / -_)  _   | |");
    MESSAGE("|___/ |_|_|_| |_| |_| \\___| |_|  | |  ");
    MESSAGE("                                /_/    ");
    MESSAGE("");

    // Read and print simulation parameters
    TITLE("Reading the simulation parameters");
    Params params(&smpi,vector<string>(argv + 1, argv + argc));
    OpenPMDparams openPMD(params);

    // Initialize MPI environment with simulation parameters
    TITLE("Initializing MPI");
    smpi.init(params);

    // Create timers
    Timers timers(&smpi);

    // Print in stdout MPI, OpenMP, patchs parameters
    params.print_parallelism_params(&smpi);

    TITLE("Initializing the restart environment");
    Checkpoint checkpoint(params, &smpi);

    // ------------------------------------------------------------------------
    // Initialize the simulation times time_prim at n=0 and time_dual at n=+1/2
    // Update in "if restart" if necessary
    // ------------------------------------------------------------------------

    // time at integer time-steps (primal grid)
    double time_prim = 0;
    // time at half-integer time-steps (dual grid)
    double time_dual = 0.5 * params.timestep;

    // -------------------------------------------
    // Declaration of the main objects & operators
    // -------------------------------------------
    // --------------------
    // Define Moving Window
    // --------------------
    TITLE("Initializing moving window");
    SimWindow* simWindow = new SimWindow(params);

    // ------------------------------------------------------------------------
    // Init nonlinear inverse Compton scattering
    // ------------------------------------------------------------------------
    RadiationTables RadiationTables;

    // ------------------------------------------------------------------------
    // Create MultiphotonBreitWheelerTables object for multiphoton
    // Breit-Wheeler pair creation
    // ------------------------------------------------------------------------
    MultiphotonBreitWheelerTables MultiphotonBreitWheelerTables;

    // ---------------------------------------------------
    // Initialize patches (including particles and fields)
    // ---------------------------------------------------
    TITLE("Initializing particles & fields");
    VectorPatch vecPatches;

    if( smpi.test_mode ) {
        execute_test_mode( vecPatches, &smpi, simWindow, params, checkpoint, openPMD );
        return 0;
    }

    // reading from dumped file the restart values
    if (params.restart) {

        // smpi.patch_count recomputed in readPatchDistribution
        checkpoint.readPatchDistribution( &smpi, simWindow );
        // allocate patches according to smpi.patch_count
        vecPatches = PatchesFactory::createVector(params, &smpi, openPMD, checkpoint.this_run_start_step+1, simWindow->getNmoved());
        // vecPatches data read in restartAll according to smpi.patch_count
        checkpoint.restartAll( vecPatches, &smpi, simWindow, params, openPMD);

        // time at integer time-steps (primal grid)
        time_prim = checkpoint.this_run_start_step * params.timestep;
        // time at half-integer time-steps (dual grid)
        time_dual = (checkpoint.this_run_start_step +0.5) * params.timestep;

        // ---------------------------------------------------------------------
        // Init and compute tables for radiation effects
        // (nonlinear inverse Compton scattering)
        // ---------------------------------------------------------------------
        RadiationTables.initParams(params);
        RadiationTables.compute_tables(params,&smpi);
        RadiationTables.output_tables(&smpi);

        // ---------------------------------------------------------------------
        // Init and compute tables for multiphoton Breit-Wheeler pair creation
        // ---------------------------------------------------------------------
        MultiphotonBreitWheelerTables.initialization(params);
        MultiphotonBreitWheelerTables.compute_tables(params,&smpi);
        MultiphotonBreitWheelerTables.output_tables(&smpi);

        TITLE("Initializing diagnostics");
        vecPatches.initAllDiags( params, &smpi );

    } else {

        vecPatches = PatchesFactory::createVector(params, &smpi, openPMD, 0);

        // Initialize the electromagnetic fields
        // -------------------------------------
        vecPatches.computeCharge();
        vecPatches.sumDensities(params, time_dual, timers, 0, simWindow);

        TITLE("Applying external fields at time t = 0");
        vecPatches.applyExternalFields();

        // ---------------------------------------------------------------------
        // Init and compute tables for radiation effects
        // (nonlinear inverse Compton scattering)
        // ---------------------------------------------------------------------
        RadiationTables.initParams(params);
        RadiationTables.compute_tables(params,&smpi);
        RadiationTables.output_tables(&smpi);

        // ---------------------------------------------------------------------
        // Init and compute tables for multiphoton Breit-Wheeler pair decay
        // ---------------------------------------------------------------------
        MultiphotonBreitWheelerTables.initialization(params);
        MultiphotonBreitWheelerTables.compute_tables(params,&smpi);
        MultiphotonBreitWheelerTables.output_tables(&smpi);

        // Apply antennas
        // --------------
        vecPatches.applyAntennas(0.5 * params.timestep);
        // Init electric field (Ex/1D, + Ey/2D)
        if (!vecPatches.isRhoNull(&smpi) && params.solve_poisson == true) {
            TITLE("Solving Poisson at time t = 0");
            vecPatches.solvePoisson( params, &smpi );
        }

        vecPatches.dynamics(params, &smpi, simWindow, RadiationTables,
                            MultiphotonBreitWheelerTables, time_dual, timers, 0);

        vecPatches.sumDensities(params, time_dual, timers, 0, simWindow );

        vecPatches.finalize_and_sort_parts(params, &smpi, simWindow,
            RadiationTables,MultiphotonBreitWheelerTables, 
            time_dual, timers, 0);

        TITLE("Initializing diagnostics");
        vecPatches.initAllDiags( params, &smpi );
        TITLE("Running diags at time t = 0");
        vecPatches.runAllDiags(params, &smpi, 0, timers, simWindow);
    }

    TITLE("Species creation summary");
    vecPatches.printNumberOfParticles( &smpi );

    timers.reboot();

    // ------------------------------------------------------------------------
    // Check memory consumption & expected disk usage
    // ------------------------------------------------------------------------
    TITLE("Memory consumption");
    vecPatches.check_memory_consumption( &smpi );
    
    TITLE("Expected disk usage (approximate)");
    vecPatches.check_expected_disk_usage( &smpi, params, checkpoint );
    
    // ------------------------------------------------------------------------
    // check here if we can close the python interpreter
    // ------------------------------------------------------------------------
    TITLE("Cleaning up python runtime environement");
    params.cleanup(&smpi);

/*tommaso
    // save latestTimeStep (used to test if we are at the latest timestep when running diagnostics at run's end)
    unsigned int latestTimeStep=checkpoint.this_run_start_step;
*/
    // ------------------------------------------------------------------
    //                     HERE STARTS THE PIC LOOP
    // ------------------------------------------------------------------

    TITLE("Time-Loop started: number of time-steps n_time = " << params.n_time);
    if ( smpi.isMaster() ) params.print_timestep_headers();

    #pragma omp parallel shared (time_dual,smpi,params, vecPatches, simWindow, checkpoint)
    {

        unsigned int itime=checkpoint.this_run_start_step+1;
        while ( (itime <= params.n_time) && (!checkpoint.exit_asap) ) {

            // calculate new times
            // -------------------
            #pragma omp single
            {
                time_prim += params.timestep;
                time_dual += params.timestep;
            }
            // apply collisions if requested
            vecPatches.applyCollisions(params, itime, timers);

            // (1) interpolate the fields at the particle position
            // (2) move the particle
            // (3) calculate the currents (charge conserving method)

            vecPatches.dynamics(params, &smpi, simWindow, RadiationTables,
                                MultiphotonBreitWheelerTables,
                                time_dual, timers, itime);


            // Sum densities
            vecPatches.sumDensities(params, time_dual, timers, itime, simWindow );

            // apply currents from antennas
            vecPatches.applyAntennas(time_dual);

            // solve Maxwell's equations
            if( time_dual > params.time_fields_frozen )
                vecPatches.solveMaxwell( params, simWindow, itime, time_dual, timers );

            vecPatches.finalize_and_sort_parts(params, &smpi, simWindow, RadiationTables,
                                               MultiphotonBreitWheelerTables,
                                               time_dual, timers, itime);

            // call the various diagnostics
            vecPatches.runAllDiags(params, &smpi, itime, timers, simWindow);

            timers.movWindow.restart();
            simWindow->operate(vecPatches, &smpi, params, itime, time_dual);
            timers.movWindow.update();

            // ----------------------------------------------------------------------
            // Validate restart  : to do
            // Restart patched moving window : to do
            #pragma omp master
            checkpoint.dump(vecPatches, itime, &smpi, simWindow, params);
            #pragma omp barrier
            // ----------------------------------------------------------------------


            if ((params.balancing_every > 0) && (smpi.getSize()!=1) ) {
                if (( itime%params.balancing_every == 0 )) {
                    timers.loadBal.restart();
                    #pragma omp single
                    vecPatches.load_balance( params, time_dual, &smpi, simWindow, itime );
                    timers.loadBal.update( params.printNow( itime ) );
                }
            }

            // print message at given time-steps
            // --------------------------------
            if ( smpi.isMaster() &&  params.printNow( itime ) )
                params.print_timestep(itime, time_dual, timers.global); //contain a timer.update !!!

            itime++;

        }//END of the time loop

    } //End omp parallel region

    smpi.barrier();

    // ------------------------------------------------------------------
    //                      HERE ENDS THE PIC LOOP
    // ------------------------------------------------------------------
    TITLE("End time loop, time dual = " << time_dual);
    timers.global.update();

    TITLE("Time profiling : (print time > 0.001%)");
    timers.profile(&smpi);

/*tommaso
    // ------------------------------------------------------------------
    //                      Temporary validation diagnostics
    // ------------------------------------------------------------------

    if (latestTimeStep==params.n_time)
        vecPatches.runAllDiags(params, smpi, &diag_flag, params.n_time, timer, simWindow);
*/

    // ------------------------------
    //  Cleanup & End the simulation
    // ------------------------------
    vecPatches.close( &smpi );
    smpi.barrier(); // Don't know why but sync needed by HDF5 Phasespace managment
    delete simWindow;
    PyTools::closePython();
    TITLE("END");

    return 0;

}//END MAIN

// ---------------------------------------------------------------------------------------------------------------------
//                                               END MAIN CODE
// ---------------------------------------------------------------------------------------------------------------------


int execute_test_mode( VectorPatch &vecPatches, SmileiMPI* smpi, SimWindow* simWindow, Params &params, Checkpoint &checkpoint, OpenPMDparams& openPMD )
{
    int itime = 0;
    int moving_window_movement = 0;

    if (params.restart) {
        checkpoint.readPatchDistribution( smpi, simWindow );
        itime = checkpoint.this_run_start_step+1;
        moving_window_movement = simWindow->getNmoved();
    }

    vecPatches = PatchesFactory::createVector(params, smpi, openPMD, itime, moving_window_movement );

    if (params.restart)
        checkpoint.restartAll( vecPatches, smpi, simWindow, params, openPMD);

    
    TITLE("Expected disk usage (approximate)");
    vecPatches.check_expected_disk_usage( smpi, params, checkpoint );
    
    // If test mode enable, code stops here
    TITLE("Cleaning up python runtime environement");
    params.cleanup(smpi);
    delete simWindow;
    PyTools::closePython();
    TITLE("END TEST MODE");

    return 0;
}
