#include "VectorPatch.h"

#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cstring>
//#include <string>

#include "Collisions.h"
#include "Hilbert_functions.h"
#include "PatchesFactory.h"
#include "Species.h"
#include "Particles.h"
#include "SimWindow.h"
#include "SolverFactory.h"
#include "DiagnosticFactory.h"

#include "SyncVectorPatch.h"

#include "Timers.h"

using namespace std;


VectorPatch::VectorPatch()
{
}


VectorPatch::~VectorPatch()
{
}

void VectorPatch::close(SmileiMPI * smpiData)
{
    closeAllDiags( smpiData );

    for (unsigned int idiag=0 ; idiag<localDiags.size(); idiag++)
        delete localDiags[idiag];
    localDiags.clear();

    for (unsigned int idiag=0 ; idiag<globalDiags.size(); idiag++)
        delete globalDiags[idiag];
    globalDiags.clear();

    for (unsigned int ipatch=0 ; ipatch<size(); ipatch++)
        delete patches_[ipatch];

    patches_.clear();
}

void VectorPatch::createDiags(Params& params, SmileiMPI* smpi, OpenPMDparams& openPMD)
{
    globalDiags = DiagnosticFactory::createGlobalDiagnostics(params, smpi, *this );
    localDiags  = DiagnosticFactory::createLocalDiagnostics (params, smpi, *this, openPMD );

    // Delete all unused fields
    for (unsigned int ipatch=0 ; ipatch<size() ; ipatch++) {
        for (unsigned int ifield=0 ; ifield<(*this)(ipatch)->EMfields->Jx_s.size(); ifield++) {
            if( (*this)(ipatch)->EMfields->Jx_s[ifield]->data_ == NULL ){
                delete (*this)(ipatch)->EMfields->Jx_s[ifield];
                (*this)(ipatch)->EMfields->Jx_s[ifield]=NULL;
            }
        }
        for (unsigned int ifield=0 ; ifield<(*this)(ipatch)->EMfields->Jy_s.size(); ifield++) {
            if( (*this)(ipatch)->EMfields->Jy_s[ifield]->data_ == NULL ){
                delete (*this)(ipatch)->EMfields->Jy_s[ifield];
                (*this)(ipatch)->EMfields->Jy_s[ifield]=NULL;
            }
        }
        for (unsigned int ifield=0 ; ifield<(*this)(ipatch)->EMfields->Jz_s.size(); ifield++) {
            if( (*this)(ipatch)->EMfields->Jz_s[ifield]->data_ == NULL ){
                delete (*this)(ipatch)->EMfields->Jz_s[ifield];
                (*this)(ipatch)->EMfields->Jz_s[ifield]=NULL;
            }
        }
        for (unsigned int ifield=0 ; ifield<(*this)(ipatch)->EMfields->rho_s.size(); ifield++) {
            if( (*this)(ipatch)->EMfields->rho_s[ifield]->data_ == NULL ){
                delete (*this)(ipatch)->EMfields->rho_s[ifield];
                (*this)(ipatch)->EMfields->rho_s[ifield]=NULL;
            }
        }
    }
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
// ----------------------------------------------       INTERFACES        ----------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------


// ---------------------------------------------------------------------------------------------------------------------
// For all patch, move particles (restartRhoJ(s), dynamics and exchangeParticles)
// ---------------------------------------------------------------------------------------------------------------------
void VectorPatch::dynamics(Params& params,
                           SmileiMPI* smpi,
                           SimWindow* simWindow,
                           RadiationTables & RadiationTables,
                           MultiphotonBreitWheelerTables & MultiphotonBreitWheelerTables,
                           double time_dual, Timers &timers, int itime)
{

    #pragma omp single
    diag_flag = needsRhoJsNow(itime);

    timers.particles.restart();
    ostringstream t;
    #pragma omp for schedule(runtime)
    for (unsigned int ipatch=0 ; ipatch<(*this).size() ; ipatch++) {
        (*this)(ipatch)->EMfields->restartRhoJ();
        for (unsigned int ispec=0 ; ispec<(*this)(ipatch)->vecSpecies.size() ; ispec++) {
            if ( (*this)(ipatch)->vecSpecies[ispec]->isProj(time_dual, simWindow) || diag_flag  ) {
                species(ipatch, ispec)->dynamics(time_dual, ispec,
                                                 emfields(ipatch), interp(ipatch), proj(ipatch),
                                                 params, diag_flag, partwalls(ipatch),
                                                 (*this)(ipatch), smpi,
                                                 RadiationTables,
                                                 MultiphotonBreitWheelerTables,
                                                 localDiags);
            }
        }

    }
    timers.particles.update( params.printNow( itime ) );

    timers.syncPart.restart();
    for (unsigned int ispec=0 ; ispec<(*this)(0)->vecSpecies.size(); ispec++) {
        if ( (*this)(0)->vecSpecies[ispec]->isProj(time_dual, simWindow) ){
            SyncVectorPatch::exchangeParticles((*this), ispec, params, smpi, timers, itime ); // Included sort_part
        }
    }
    timers.syncPart.update( params.printNow( itime ) );

} // END dynamics


void VectorPatch::finalize_and_sort_parts(Params& params, SmileiMPI* smpi, SimWindow* simWindow,
                           RadiationTables & RadiationTables,
                           MultiphotonBreitWheelerTables & MultiphotonBreitWheelerTables,
                           double time_dual, Timers &timers, int itime)
{
    timers.syncPart.restart();
    for (unsigned int ispec=0 ; ispec<(*this)(0)->vecSpecies.size(); ispec++) {
        if ( (*this)(0)->vecSpecies[ispec]->isProj(time_dual, simWindow) ){
            SyncVectorPatch::finalize_and_sort_parts((*this), ispec, params, smpi, timers, itime ); // Included sort_part
        }
    }

    #pragma omp for schedule(runtime)
    for (unsigned int ipatch=0 ; ipatch<(*this).size() ; ipatch++) {
        // Particle importation for all species
        for (unsigned int ispec=0 ; ispec<(*this)(ipatch)->vecSpecies.size() ; ispec++) {
            if ( (*this)(ipatch)->vecSpecies[ispec]->isProj(time_dual, simWindow) || diag_flag  ) {

                species(ipatch, ispec)->dynamics_import_particles(time_dual, ispec,
                                                                  params,
                                                                  (*this)(ipatch), smpi,
                                                                  RadiationTables,
                                                                  MultiphotonBreitWheelerTables,
                                                                  localDiags);
            }
        }
    }
    if (itime%params.every_clean_particles_overhead==0) {
        #pragma omp master
        for (unsigned int ipatch=0 ; ipatch<(*this).size() ; ipatch++)
            (*this)(ipatch)->cleanParticlesOverhead(params);
        #pragma omp barrier
    }
    timers.syncPart.update( params.printNow( itime ) );

    if ( (itime!=0) && ( time_dual > params.time_fields_frozen ) ) {
        if ( params.geometry != "3drz" ) {
            timers.syncField.restart();
            SyncVectorPatch::finalizeexchangeB( (*this) );
            timers.syncField.update(  params.printNow( itime ) );
        }
	
        // else finalize is done just after exchange
        //     for now ; communication must be completed par mode (tag management)
        #pragma omp for schedule(static)
        for (unsigned int ipatch=0 ; ipatch<(*this).size() ; ipatch++){
            // Applies boundary conditions on B
            (*this)(ipatch)->EMfields->boundaryConditions(itime, time_dual, (*this)(ipatch), params, simWindow); 
            // Computes B at time n using B and B_m.
            (*this)(ipatch)->EMfields->centerMagneticFields();
        }
    }

} // END dynamics


void VectorPatch::computeCharge()
{
    #pragma omp for schedule(runtime)
    for (unsigned int ipatch=0 ; ipatch<(*this).size() ; ipatch++) {
        (*this)(ipatch)->EMfields->restartRhoJ();
        for (unsigned int ispec=0 ; ispec<(*this)(ipatch)->vecSpecies.size() ; ispec++) {
            species(ipatch, ispec)->computeCharge(ispec, emfields(ipatch), proj(ipatch) );
        }
    }

} // END computeRho


// ---------------------------------------------------------------------------------------------------------------------
// For all patch, sum densities on ghost cells (sum per species if needed, sync per patch and MPI sync)
// ---------------------------------------------------------------------------------------------------------------------
void VectorPatch::sumDensities(Params &params, double time_dual, Timers &timers, int itime, SimWindow* simWindow )
{
    bool some_particles_are_moving = false;
    unsigned int n_species( (*this)(0)->vecSpecies.size() );
    for ( unsigned int ispec=0 ; ispec < n_species ; ispec++ ) {
        if ( (*this)(0)->vecSpecies[ispec]->isProj(time_dual, simWindow) )
            some_particles_are_moving = true;
    }
    if ( !some_particles_are_moving  && !diag_flag )
        return;

    timers.densities.restart();
    if  (diag_flag){
        #pragma omp for schedule(static)
        for (unsigned int ipatch=0 ; ipatch<(*this).size() ; ipatch++) {
             // Per species in global, Attention if output -> Sync / per species fields
            (*this)(ipatch)->EMfields->computeTotalRhoJ();
        }
    }
    timers.densities.update();

    timers.syncDens.restart();

    if ( params.geometry != "3drz" ) {
        SyncVectorPatch::sumRhoJ( (*this), timers, itime ); // MPI
    }
    else {
        for (unsigned int imode = 0 ; imode < static_cast<ElectroMagn3DRZ*>(patches_[0]->EMfields)->Jl_.size() ; imode++  ) {
            SyncVectorPatch::sumRhoJ( (*this), imode, timers, itime );
        }
    }

    if(diag_flag){
        for (unsigned int ispec=0 ; ispec<(*this)(0)->vecSpecies.size(); ispec++) {
            if( ! (*this)(0)->vecSpecies[ispec]->particles->is_test ) {
                update_field_list(ispec);
                SyncVectorPatch::sumRhoJs( (*this), ispec, timers, itime ); // MPI
            }
        }
    }
    timers.syncDens.update( params.printNow( itime ) );

} // End sumDensities


// ---------------------------------------------------------------------------------------------------------------------
// For all patch, update E and B (Ampere, Faraday, boundary conditions, exchange B and center B)
// ---------------------------------------------------------------------------------------------------------------------
void VectorPatch::solveMaxwell(Params& params, SimWindow* simWindow, int itime, double time_dual, Timers & timers)
{
    timers.maxwell.restart();

    for (unsigned int ipassfilter=0 ; ipassfilter<params.currentFilter_passes ; ipassfilter++){
        #pragma omp for schedule(static)
        for (unsigned int ipatch=0 ; ipatch<(*this).size() ; ipatch++){
            // Current spatial filtering
            (*this)(ipatch)->EMfields->binomialCurrentFilter();
        }
        SyncVectorPatch::exchangeJ( (*this) );
        SyncVectorPatch::finalizeexchangeJ( (*this) );
    }
    #pragma omp for schedule(static)
    for (unsigned int ipatch=0 ; ipatch<(*this).size() ; ipatch++){
        // Saving magnetic fields (to compute centered fields used in the particle pusher)
        // Stores B at time n in B_m.
        (*this)(ipatch)->EMfields->saveMagneticFields();
        // Computes Ex_, Ey_, Ez_ on all points.
        // E is already synchronized because J has been synchronized before.
        (*(*this)(ipatch)->EMfields->MaxwellAmpereSolver_)((*this)(ipatch)->EMfields);
        // Computes Bx_, By_, Bz_ at time n+1 on interior points.
        //for (unsigned int ipatch=0 ; ipatch<(*this).size() ; ipatch++) {
        (*(*this)(ipatch)->EMfields->MaxwellFaradaySolver_)((*this)(ipatch)->EMfields);
    }
    //Synchronize B fields between patches.
    timers.maxwell.update( params.printNow( itime ) );
    timers.syncField.restart();
    if ( params.geometry != "3drz" ) {
        SyncVectorPatch::exchangeB( (*this) );
    }
    else {
        for (unsigned int imode = 0 ; imode < static_cast<ElectroMagn3DRZ*>(patches_[0]->EMfields)->El_.size() ; imode++  ) {
            SyncVectorPatch::exchangeB( (*this), imode );
            SyncVectorPatch::finalizeexchangeB( (*this), imode ); // disable async, because of tags which is the same for all modes
        }
    }

    timers.syncField.update(  params.printNow( itime ) );
} // END solveMaxwell


void VectorPatch::initExternals(Params& params)
{
    // Init all lasers
    for( unsigned int ipatch=0; ipatch<size(); ipatch++ ) {
        // check if patch is on the border
        int iBC;
        if     ( (*this)(ipatch)->isXmin() ) {
            iBC = 0;
        }
        else if( (*this)(ipatch)->isXmax() ) {
            iBC = 1;
        }
        else continue;
        // If patch is on border, then fill the fields arrays
        unsigned int nlaser = (*this)(ipatch)->EMfields->emBoundCond[iBC]->vecLaser.size();
        for (unsigned int ilaser = 0; ilaser < nlaser; ilaser++)
             (*this)(ipatch)->EMfields->emBoundCond[iBC]->vecLaser[ilaser]->initFields(params, (*this)(ipatch));
    }

    // Init all antennas
    for( unsigned int ipatch=0; ipatch<size(); ipatch++ ) {
        (*this)(ipatch)->EMfields->initAntennas((*this)(ipatch));
    }
}


void VectorPatch::initAllDiags(Params& params, SmileiMPI* smpi)
{
    // Global diags: scalars + particles
    for (unsigned int idiag = 0 ; idiag < globalDiags.size() ; idiag++) {
        globalDiags[idiag]->init(params, smpi, *this);
        // MPI master creates the file
        if( smpi->isMaster() )
            globalDiags[idiag]->openFile( params, smpi, true );
    }

    // Local diags : fields, probes, tracks
    for (unsigned int idiag = 0 ; idiag < localDiags.size() ; idiag++)
        localDiags[idiag]->init(params, smpi, *this);

} // END initAllDiags


void VectorPatch::closeAllDiags(SmileiMPI* smpi)
{
    // MPI master closes all global diags
    if ( smpi->isMaster() )
        for (unsigned int idiag = 0 ; idiag < globalDiags.size() ; idiag++)
            globalDiags[idiag]->closeFile();

    // All MPI close local diags
    for (unsigned int idiag = 0 ; idiag < localDiags.size() ; idiag++)
        localDiags[idiag]->closeFile();
}


void VectorPatch::openAllDiags(Params& params,SmileiMPI* smpi)
{
    // MPI master opens all global diags
    if ( smpi->isMaster() )
        for (unsigned int idiag = 0 ; idiag < globalDiags.size() ; idiag++)
            globalDiags[idiag]->openFile( params, smpi, false );

    // All MPI open local diags
    for (unsigned int idiag = 0 ; idiag < localDiags.size() ; idiag++)
        localDiags[idiag]->openFile( params, smpi, false );
}


// ---------------------------------------------------------------------------------------------------------------------
// For all patch, Compute and Write all diags
//   - Scalars, Probes, Phases, TrackParticles, Fields, Average fields
//   - set diag_flag to 0 after write
// ---------------------------------------------------------------------------------------------------------------------
void VectorPatch::runAllDiags(Params& params, SmileiMPI* smpi, unsigned int itime, Timers & timers, SimWindow* simWindow)
{

    // Global diags: scalars + particles
    timers.diags.restart();
    for (unsigned int idiag = 0 ; idiag < globalDiags.size() ; idiag++) {
        #pragma omp single
        globalDiags[idiag]->theTimeIsNow = globalDiags[idiag]->prepare( itime );
        #pragma omp barrier
        if( globalDiags[idiag]->theTimeIsNow ) {
            // All patches run
            #pragma omp for schedule(runtime)
            for (unsigned int ipatch=0 ; ipatch<size() ; ipatch++)
                globalDiags[idiag]->run( (*this)(ipatch), itime, simWindow );
            // MPI procs gather the data and compute
            #pragma omp single
            smpi->computeGlobalDiags( globalDiags[idiag], itime);
            // MPI master writes
            #pragma omp single
            globalDiags[idiag]->write( itime , smpi );
        }
    }

    // Local diags : fields, probes, tracks
    for (unsigned int idiag = 0 ; idiag < localDiags.size() ; idiag++) {
        #pragma omp single
        localDiags[idiag]->theTimeIsNow = localDiags[idiag]->prepare( itime );
        #pragma omp barrier
        // All MPI run their stuff and write out
        if( localDiags[idiag]->theTimeIsNow )
            localDiags[idiag]->run( smpi, *this, itime, simWindow );
    }

    // Manage the "diag_flag" parameter, which indicates whether Rho and Js were used
    if( diag_flag ) {
        #pragma omp barrier
        #pragma omp single
        diag_flag = false;
        #pragma omp for
        for (unsigned int ipatch=0 ; ipatch<size() ; ipatch++)
            (*this)(ipatch)->EMfields->restartRhoJs();
    }
    timers.diags.update();

} // END runAllDiags


// ---------------------------------------------------------------------------------------------------------------------
// Check if rho is null (MPI & patch sync)
// ---------------------------------------------------------------------------------------------------------------------
bool VectorPatch::isRhoNull( SmileiMPI* smpi )
{
    double norm2(0.);
    double locnorm2(0.);
    for (unsigned int ipatch=0 ; ipatch<this->size() ; ipatch++)
        locnorm2 += (*this)(ipatch)->EMfields->computeRhoNorm2();

    MPI_Allreduce(&locnorm2, &norm2, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

    return (norm2<=0.);
} // END isRhoNull


// ---------------------------------------------------------------------------------------------------------------------
// Solve Poisson to initialize E
//   - all steps are done locally, sync per patch, sync per MPI process
// ---------------------------------------------------------------------------------------------------------------------
void VectorPatch::solvePoisson( Params &params, SmileiMPI* smpi )
{
    Timer ptimer("global");
    ptimer.init(smpi);
    ptimer.restart();


    unsigned int iteration_max = params.poisson_max_iteration;
    double           error_max = params.poisson_max_error;
    unsigned int iteration=0;

    // Init & Store internal data (phi, r, p, Ap) per patch
    double rnew_dot_rnew_local(0.);
    double rnew_dot_rnew(0.);
    for (unsigned int ipatch=0 ; ipatch<this->size() ; ipatch++) {
        (*this)(ipatch)->EMfields->initPoisson( (*this)(ipatch) );
        rnew_dot_rnew_local += (*this)(ipatch)->EMfields->compute_r();
    }
    MPI_Allreduce(&rnew_dot_rnew_local, &rnew_dot_rnew, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

    std::vector<Field*> Ex_;
    std::vector<Field*> Ap_;

    for (unsigned int ipatch=0 ; ipatch<this->size() ; ipatch++) {
        Ex_.push_back( (*this)(ipatch)->EMfields->Ex_ );
        Ap_.push_back( (*this)(ipatch)->EMfields->Ap_ );
    }

    unsigned int nx_p2_global = (params.n_space_global[0]+1);
    if ( Ex_[0]->dims_.size()>1 ) {
        nx_p2_global *= (params.n_space_global[1]+1);
        if ( Ex_[0]->dims_.size()>2 ) {
            nx_p2_global *= (params.n_space_global[2]+1);
        }
    }

    // compute control parameter
    double ctrl = rnew_dot_rnew / (double)(nx_p2_global);

    // ---------------------------------------------------------
    // Starting iterative loop for the conjugate gradient method
    // ---------------------------------------------------------
    if (smpi->isMaster()) DEBUG("Starting iterative loop for CG method");
    while ( (ctrl > error_max) && (iteration<iteration_max) ) {
        iteration++;
        if (smpi->isMaster()) DEBUG("iteration " << iteration << " started with control parameter ctrl = " << ctrl*1.e14 << " x 1e-14");

        // scalar product of the residual
        double r_dot_r = rnew_dot_rnew;

        for (unsigned int ipatch=0 ; ipatch<this->size() ; ipatch++)
            (*this)(ipatch)->EMfields->compute_Ap( (*this)(ipatch) );

        // Exchange Ap_ (intra & extra MPI)
        SyncVectorPatch::exchange( Ap_, *this );
        SyncVectorPatch::finalizeexchange( Ap_, *this );

       // scalar product p.Ap
        double p_dot_Ap       = 0.0;
        double p_dot_Ap_local = 0.0;
        for (unsigned int ipatch=0 ; ipatch<this->size() ; ipatch++) {
            p_dot_Ap_local += (*this)(ipatch)->EMfields->compute_pAp();
        }
        MPI_Allreduce(&p_dot_Ap_local, &p_dot_Ap, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);


        // compute new potential and residual
        for (unsigned int ipatch=0 ; ipatch<this->size() ; ipatch++) {
            (*this)(ipatch)->EMfields->update_pand_r( r_dot_r, p_dot_Ap );
        }

        // compute new residual norm
        rnew_dot_rnew       = 0.0;
        rnew_dot_rnew_local = 0.0;
        for (unsigned int ipatch=0 ; ipatch<this->size() ; ipatch++) {
            rnew_dot_rnew_local += (*this)(ipatch)->EMfields->compute_r();
        }
        MPI_Allreduce(&rnew_dot_rnew_local, &rnew_dot_rnew, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
        if (smpi->isMaster()) DEBUG("new residual norm: rnew_dot_rnew = " << rnew_dot_rnew);

        // compute new directio
        for (unsigned int ipatch=0 ; ipatch<this->size() ; ipatch++) {
            (*this)(ipatch)->EMfields->update_p( rnew_dot_rnew, r_dot_r );
        }

        // compute control parameter
        ctrl = rnew_dot_rnew / (double)(nx_p2_global);
        if (smpi->isMaster()) DEBUG("iteration " << iteration << " done, exiting with control parameter ctrl = " << ctrl);

    }//End of the iterative loop


    // --------------------------------
    // Status of the solver convergence
    // --------------------------------
    if (iteration_max>0 && iteration == iteration_max) {
        if (smpi->isMaster())
            WARNING("Poisson solver did not converge: reached maximum iteration number: " << iteration
                    << ", relative err is ctrl = " << 1.0e14*ctrl << " x 1e-14");
    }
    else {
        if (smpi->isMaster())
            MESSAGE(1,"Poisson solver converged at iteration: " << iteration
                    << ", relative err is ctrl = " << 1.0e14*ctrl << " x 1e-14");
    }

    // ------------------------------------------
    // Compute the electrostatic fields Ex and Ey
    // ------------------------------------------
    for (unsigned int ipatch=0 ; ipatch<this->size() ; ipatch++)
        (*this)(ipatch)->EMfields->initE( (*this)(ipatch) );

    SyncVectorPatch::exchangeE( *this );
    SyncVectorPatch::finalizeexchangeE( *this );

    // Centering of the electrostatic fields
    // -------------------------------------
    vector<double> E_Add(Ex_[0]->dims_.size(),0.);
    if ( Ex_[0]->dims_.size()==3 ) {
        double Ex_avg_local(0.), Ex_avg(0.), Ey_avg_local(0.), Ey_avg(0.), Ez_avg_local(0.), Ez_avg(0.);
        for (unsigned int ipatch=0 ; ipatch<this->size() ; ipatch++) {
            Ex_avg_local += (*this)(ipatch)->EMfields->computeExSum();
            Ey_avg_local += (*this)(ipatch)->EMfields->computeEySum();
            Ez_avg_local += (*this)(ipatch)->EMfields->computeEzSum();
        }

        MPI_Allreduce(&Ex_avg_local, &Ex_avg, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
        MPI_Allreduce(&Ey_avg_local, &Ey_avg, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
        MPI_Allreduce(&Ez_avg_local, &Ez_avg, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

        E_Add[0] = -Ex_avg/((params.n_space[0]+2)*(params.n_space[1]+1)*(params.n_space[2]+1));
        E_Add[1] = -Ey_avg/((params.n_space[0]+1)*(params.n_space[1]+2)*(params.n_space[2]+1));;
        E_Add[2] = -Ez_avg/((params.n_space[0]+1)*(params.n_space[1]+1)*(params.n_space[2]+2));;
    }
    else if ( Ex_[0]->dims_.size()==2 ) {
        double Ex_XminYmax = 0.0;
        double Ey_XminYmax = 0.0;
        double Ex_XmaxYmin = 0.0;
        double Ey_XmaxYmin = 0.0;

        //The YmaxXmin patch has Patch coordinates X=0, Y=2^m1-1= number_of_patches[1]-1.
        //Its hindex is
        int patch_YmaxXmin = generalhilbertindex(params.mi[0], params.mi[1], 0,  params.number_of_patches[1]-1);
        //The MPI rank owning it is
        int rank_XminYmax = smpi->hrank(patch_YmaxXmin);
        //The YminXmax patch has Patch coordinates X=2^m0-1= number_of_patches[0]-1, Y=0.
        //Its hindex is
        int patch_YminXmax = generalhilbertindex(params.mi[0], params.mi[1], params.number_of_patches[0]-1, 0);
        //The MPI rank owning it is
        int rank_XmaxYmin = smpi->hrank(patch_YminXmax);


        //cout << params.mi[0] << " " << params.mi[1] << " " << params.number_of_patches[0] << " " << params.number_of_patches[1] << endl;
        //cout << patch_YmaxXmin << " " << rank_XminYmax << " " << patch_YminXmax << " " << rank_XmaxYmin << endl;

        if ( smpi->getRank() == rank_XminYmax ) {
            Ex_XminYmax = (*this)(patch_YmaxXmin-((*this).refHindex_))->EMfields->getEx_XminYmax();
            Ey_XminYmax = (*this)(patch_YmaxXmin-((*this).refHindex_))->EMfields->getEy_XminYmax();
        }

        // Xmax-Ymin corner
        if ( smpi->getRank() == rank_XmaxYmin ) {
            Ex_XmaxYmin = (*this)(patch_YminXmax-((*this).refHindex_))->EMfields->getEx_XmaxYmin();
            Ey_XmaxYmin = (*this)(patch_YminXmax-((*this).refHindex_))->EMfields->getEy_XmaxYmin();
        }

        MPI_Bcast(&Ex_XminYmax, 1, MPI_DOUBLE, rank_XminYmax, MPI_COMM_WORLD);
        MPI_Bcast(&Ey_XminYmax, 1, MPI_DOUBLE, rank_XminYmax, MPI_COMM_WORLD);

        MPI_Bcast(&Ex_XmaxYmin, 1, MPI_DOUBLE, rank_XmaxYmin, MPI_COMM_WORLD);
        MPI_Bcast(&Ey_XmaxYmin, 1, MPI_DOUBLE, rank_XmaxYmin, MPI_COMM_WORLD);

        //This correction is always done, independantly of the periodicity. Is this correct ?
        E_Add[0] = -0.5*(Ex_XminYmax+Ex_XmaxYmin);
        E_Add[1] = -0.5*(Ey_XminYmax+Ey_XmaxYmin);

#ifdef _3D_LIKE_CENTERING
        double Ex_avg_local(0.), Ex_avg(0.), Ey_avg_local(0.), Ey_avg(0.);
        for (unsigned int ipatch=0 ; ipatch<this->size() ; ipatch++) {
            Ex_avg_local += (*this)(ipatch)->EMfields->computeExSum();
            Ey_avg_local += (*this)(ipatch)->EMfields->computeEySum();
        }

        MPI_Allreduce(&Ex_avg_local, &Ex_avg, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
        MPI_Allreduce(&Ey_avg_local, &Ey_avg, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

        E_Add[0] = -Ex_avg/((params.n_space[0]+2)*(params.n_space[1]+1));
        E_Add[1] = -Ey_avg/((params.n_space[0]+1)*(params.n_space[1]+2));;
#endif

    }
    else if( Ex_[0]->dims_.size()==1 ) {
        double Ex_Xmin = 0.0;
        double Ex_Xmax = 0.0;

        unsigned int rankXmin = 0;
        if ( smpi->getRank() == 0 ) {
            //Ex_Xmin = (*Ex1D)(index_bc_min[0]);
            Ex_Xmin = (*this)( (0)-((*this).refHindex_))->EMfields->getEx_Xmin();
        }
        MPI_Bcast(&Ex_Xmin, 1, MPI_DOUBLE, rankXmin, MPI_COMM_WORLD);

        unsigned int rankXmax = smpi->getSize()-1;
        if ( smpi->getRank() == smpi->getSize()-1 ) {
            //Ex_Xmax = (*Ex1D)(index_bc_max[0]);
            Ex_Xmax = (*this)( (params.number_of_patches[0]-1)-((*this).refHindex_))->EMfields->getEx_Xmax();
        }
        MPI_Bcast(&Ex_Xmax, 1, MPI_DOUBLE, rankXmax, MPI_COMM_WORLD);
        E_Add[0] = -0.5*(Ex_Xmin+Ex_Xmax);

#ifdef _3D_LIKE_CENTERING
        double Ex_avg_local(0.), Ex_avg(0.);
        for (unsigned int ipatch=0 ; ipatch<this->size() ; ipatch++) {
            Ex_avg_local += (*this)(ipatch)->EMfields->computeExSum();
        }

        MPI_Allreduce(&Ex_avg_local, &Ex_avg, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

        E_Add[0] = -Ex_avg/((params.n_space[0]+2));
#endif

    }

    // Centering electrostatic fields
    for (unsigned int ipatch=0 ; ipatch<this->size() ; ipatch++)
        (*this)(ipatch)->EMfields->centeringE( E_Add );


    // Compute error on the Poisson equation
    double deltaPoisson_max = 0.0;
    int i_deltaPoisson_max  = -1;

#ifdef _A_FINALISER
    for (unsigned int i=0; i<nx_p; i++) {
        double deltaPoisson = abs( ((*Ex1D)(i+1)-(*Ex1D)(i))/dx - (*rho1D)(i) );
        if (deltaPoisson > deltaPoisson_max) {
            deltaPoisson_max   = deltaPoisson;
            i_deltaPoisson_max = i;
        }
    }
#endif

    //!\todo Reduce to find global max
    if (smpi->isMaster())
        MESSAGE(1,"Poisson equation solved. Maximum err = " << deltaPoisson_max << " at i= " << i_deltaPoisson_max);

    ptimer.update();
    MESSAGE("Time in Poisson : " << ptimer.getTime() );

} // END solvePoisson


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
// ----------------------------------------------    BALANCING METHODS    ----------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------


void VectorPatch::load_balance(Params& params, double time_dual, SmileiMPI* smpi, SimWindow* simWindow, unsigned int itime)
{

    // Compute new patch distribution
    smpi->recompute_patch_count( params, *this, time_dual );

    // Create empty patches according to this new distribution
    this->createPatches(params, smpi, simWindow);

    // Proceed to patch exchange, and delete patch which moved
    this->exchangePatches(smpi, params);

    // Tell that the patches moved this iteration (needed for probes)
    lastIterationPatchesMoved = itime;

}


// ---------------------------------------------------------------------------------------------------------------------
// Explicits patch movement regarding new patch distribution stored in smpi->patch_count
//   - compute send_patch_id_
//   - compute recv_patch_id_
//   - create empty (not really, created like at t0) new patch in recv_patches_
// ---------------------------------------------------------------------------------------------------------------------
void VectorPatch::createPatches(Params& params, SmileiMPI* smpi, SimWindow* simWindow)
{
    unsigned int n_moved(0);
    recv_patches_.resize(0);

    // Set Index of the 1st patch of the vector yet on current MPI rank
    // Is this really necessary ? It should be done already ...
    refHindex_ = (*this)(0)->Hindex();

    // Current number of patch
    int nPatches_now = this->size() ;

    // When going to openMP, these two vectors must be stored by patch and not by vectorPatch.
    recv_patch_id_.clear();
    send_patch_id_.clear();

    // istart = Index of the futur 1st patch
    int istart( 0 );
    for (int irk=0 ; irk<smpi->getRank() ; irk++) istart += smpi->patch_count[irk];

    // recv_patch_id_ = vector of the hindex this process must own at the end of the exchange.
    for (int ipatch=0 ; ipatch<smpi->patch_count[smpi->getRank()] ; ipatch++)
        recv_patch_id_.push_back( istart+ipatch );


    // Loop on current patches to define patch to send
    for (int ipatch=0 ; ipatch < nPatches_now ; ipatch++) {
        //if  current hindex     <  future refHindex   OR      current hindex > future last hindex...
        if ( ( refHindex_+ipatch < recv_patch_id_[0] ) || ( refHindex_+ipatch > recv_patch_id_.back() ) ) {
            // Put this patch in the send list.
            send_patch_id_.push_back( ipatch );
        }
    }


    // Backward loop on future patches to define suppress patch in receive list
    // before this loop, recv_patch_id_ stores all patches index define in SmileiMPI::patch_count
    int existing_patch_id = -1;
    for ( int ipatch=recv_patch_id_.size()-1 ; ipatch>=0 ; ipatch--) {
        //if    future patch hindex  >= current refHindex AND  future patch hindex <= current last hindex
        if ( ( recv_patch_id_[ipatch]>=refHindex_ ) && ( recv_patch_id_[ipatch] <= refHindex_ + nPatches_now - 1 ) ) {
            //Store an existing patch id for cloning.
            existing_patch_id = recv_patch_id_[ipatch];
            //Remove this patch from the receive list because I already own it.
            recv_patch_id_.erase( recv_patch_id_.begin()+ipatch );
        }
    }


    // Get an existing patch that will be used for cloning
    if( existing_patch_id<0 )
        ERROR("No patch to clone. This should never happen!");
    Patch * existing_patch = (*this)(existing_patch_id-refHindex_);


    // Create new Patches
    n_moved = simWindow->getNmoved();
    // Store in local vector future patches
    // Loop on the patches I have to receive and do not already own.
    for (unsigned int ipatch=0 ; ipatch < recv_patch_id_.size() ; ipatch++) {
        // density profile is initializes as if t = 0 !
        // Species will be cleared when, nbr of particles will be known
        // Creation of a new patch, ready to receive its content from MPI neighbours.
        Patch* newPatch = PatchesFactory::clone(existing_patch, params, smpi, recv_patch_id_[ipatch], n_moved, false );
        newPatch->finalizeMPIenvironment(params);
        //Store pointers to newly created patch in recv_patches_.
        recv_patches_.push_back( newPatch );
    }


} // END createPatches


// ---------------------------------------------------------------------------------------------------------------------
// Exchange patches, based on createPatches initialization
//   take care of reinitialize patch master and diag file managment
// ---------------------------------------------------------------------------------------------------------------------
void VectorPatch::exchangePatches(SmileiMPI* smpi, Params& params)
{

    //int newMPIrankbis, oldMPIrankbis, tmp;
    int newMPIrank = smpi->getRank() -1;
    int oldMPIrank = smpi->getRank() -1;
    int istart = 0;
    int nmessage = nrequests;

    for (int irk=0 ; irk<smpi->getRank() ; irk++) istart += smpi->patch_count[irk];


    // Send particles
    for (unsigned int ipatch=0 ; ipatch < send_patch_id_.size() ; ipatch++) {
        // locate rank which will own send_patch_id_[ipatch]
        // We assume patches are only exchanged with neighbours.
        // Once all patches supposed to be sent to the left are done, we send the rest to the right.
        // if hindex of patch to be sent      >  future hindex of the first patch owned by this process
        if (send_patch_id_[ipatch]+refHindex_ > istart ) newMPIrank = smpi->getRank() + 1;

        smpi->isend( (*this)(send_patch_id_[ipatch]), newMPIrank, (refHindex_+send_patch_id_[ipatch])*nmessage, params );
    }

    for (unsigned int ipatch=0 ; ipatch < recv_patch_id_.size() ; ipatch++) {
        //if  hindex of patch to be received > first hindex actually owned, that means it comes from the next MPI process and not from the previous anymore.
        if(recv_patch_id_[ipatch] > refHindex_ ) oldMPIrank = smpi->getRank() + 1;

        smpi->recv( recv_patches_[ipatch], oldMPIrank, recv_patch_id_[ipatch]*nmessage, params );
    }


    for (unsigned int ipatch=0 ; ipatch < send_patch_id_.size() ; ipatch++)
        smpi->waitall( (*this)(send_patch_id_[ipatch]) );

    smpi->barrier();
    //Delete sent patches
    int nPatchSend(send_patch_id_.size());
    for (int ipatch=nPatchSend-1 ; ipatch>=0 ; ipatch--) {
        //Ok while at least 1 old patch stay inon current CPU
        delete (*this)(send_patch_id_[ipatch]);
        patches_[ send_patch_id_[ipatch] ] = NULL;
        patches_.erase( patches_.begin() + send_patch_id_[ipatch] );

    }


    //Put received patches in the global vecPatches
    for (unsigned int ipatch=0 ; ipatch<recv_patch_id_.size() ; ipatch++) {
        if ( recv_patch_id_[ipatch] > refHindex_ )
            patches_.push_back( recv_patches_[ipatch] );
        else
            patches_.insert( patches_.begin()+ipatch, recv_patches_[ipatch] );
    }
    recv_patches_.clear();


    for (unsigned int ipatch=0 ; ipatch<patches_.size() ; ipatch++ ) {
        (*this)(ipatch)->updateMPIenv(smpi);
        if ((*this)(ipatch)->has_an_MPI_neighbor())
            (*this)(ipatch)->createType(params);
         else
            (*this)(ipatch)->cleanType();
    }
    (*this).set_refHindex() ;
    update_field_list() ;

} // END exchangePatches

// ---------------------------------------------------------------------------------------------------------------------
// Write in a file patches communications
//   - Send/Recv MPI rank
//   - Send/Recv patch Id
// ---------------------------------------------------------------------------------------------------------------------
void VectorPatch::output_exchanges(SmileiMPI* smpi)
{
    ofstream output_file;
    ostringstream name("");
    name << "debug_output"<<smpi->getRank()<<".txt" ;
    output_file.open(name.str().c_str(), std::ofstream::out | std::ofstream::app);
    int newMPIrank, oldMPIrank;
    newMPIrank = smpi->getRank() -1;
    oldMPIrank = smpi->getRank() -1;
    int istart( 0 );
    for (int irk=0 ; irk<smpi->getRank() ; irk++) istart += smpi->patch_count[irk];
    for (unsigned int ipatch=0 ; ipatch < send_patch_id_.size() ; ipatch++) {
        if(send_patch_id_[ipatch]+refHindex_ > istart ) newMPIrank = smpi->getRank() + 1;
        output_file << "Rank " << smpi->getRank() << " sending patch " << send_patch_id_[ipatch]+refHindex_ << " to " << newMPIrank << endl;
    }
    for (unsigned int ipatch=0 ; ipatch < recv_patch_id_.size() ; ipatch++) {
        if(recv_patch_id_[ipatch] > refHindex_ ) oldMPIrank = smpi->getRank() + 1;
        output_file << "Rank " << smpi->getRank() << " receiving patch " << recv_patch_id_[ipatch] << " from " << oldMPIrank << endl;
    }
    output_file << "NEXT" << endl;
    output_file.close();
} // END output_exchanges

//! Resize vector of field*
void VectorPatch::update_field_list()
{
    int nDim(0);
    if ( !dynamic_cast<ElectroMagn3DRZ*>(patches_[0]->EMfields) )
        nDim = patches_[0]->EMfields->Ex_->dims_.size();
    else
        nDim = static_cast<ElectroMagn3DRZ*>(patches_[0]->EMfields)->El_[0]->dims_.size();
    densities.resize( 3*size() ) ; // Jx + Jy + Jz

    //                          1D  2D  3D
    Bs0.resize( 2*size() ) ; //  2   2   2
    Bs1.resize( 2*size() ) ; //  0   2   2
    Bs2.resize( 2*size() ) ; //  0   0   2

    densitiesLocalx.clear();
    densitiesLocaly.clear();
    densitiesLocalz.clear();
    densitiesMPIx.clear();
    densitiesMPIy.clear();
    densitiesMPIz.clear();
    LocalxIdx.clear();
    LocalyIdx.clear();
    LocalzIdx.clear();
    MPIxIdx.clear();
    MPIyIdx.clear();
    MPIzIdx.clear();

    listJx_.resize( size() ) ;
    listJy_.resize( size() ) ;
    listJz_.resize( size() ) ;
    listrho_.resize( size() ) ;
    listEx_.resize( size() ) ;
    listEy_.resize( size() ) ;
    listEz_.resize( size() ) ;
    listBx_.resize( size() ) ;
    listBy_.resize( size() ) ;
    listBz_.resize( size() ) ;

    for (unsigned int ipatch=0 ; ipatch < size() ; ipatch++) {
        listJx_[ipatch] = patches_[ipatch]->EMfields->Jx_ ;
        listJy_[ipatch] = patches_[ipatch]->EMfields->Jy_ ;
        listJz_[ipatch] = patches_[ipatch]->EMfields->Jz_ ;
        listrho_[ipatch] =patches_[ipatch]->EMfields->rho_;
        listEx_[ipatch] = patches_[ipatch]->EMfields->Ex_ ;
        listEy_[ipatch] = patches_[ipatch]->EMfields->Ey_ ;
        listEz_[ipatch] = patches_[ipatch]->EMfields->Ez_ ;
        listBx_[ipatch] = patches_[ipatch]->EMfields->Bx_ ;
        listBy_[ipatch] = patches_[ipatch]->EMfields->By_ ;
        listBz_[ipatch] = patches_[ipatch]->EMfields->Bz_ ;
    }


    #ifdef _TODO_RZ
    // Manage RZ & cartesian
    #endif

    if ( dynamic_cast<ElectroMagn3DRZ*>(patches_[0]->EMfields) ) {
        unsigned int nmodes = static_cast<ElectroMagn3DRZ*>(patches_[0]->EMfields)->El_.size();
        listJl_.resize( nmodes ) ;
        listJr_.resize( nmodes ) ;
        listJt_.resize( nmodes ) ;
        listrho_RZ_.resize( nmodes ) ;
        listEl_.resize( nmodes ) ;
        listEr_.resize( nmodes ) ;
        listEt_.resize( nmodes ) ;
        listBl_.resize( nmodes ) ;
        listBr_.resize( nmodes ) ;
        listBt_.resize( nmodes ) ;
    
        for (unsigned int imode=0 ; imode < nmodes ; imode++) {
            listJl_[imode].resize( size() );
            listJr_[imode].resize( size() );
            listJt_[imode].resize( size() );
            listrho_RZ_[imode].resize( size() );
            listEl_[imode].resize( size() );
            listEr_[imode].resize( size() );
            listEt_[imode].resize( size() );
            listBl_[imode].resize( size() );
            listBr_[imode].resize( size() );
            listBt_[imode].resize( size() );
            for (unsigned int ipatch=0 ; ipatch < size() ; ipatch++) {
                listJl_[imode][ipatch] = static_cast<ElectroMagn3DRZ*>(patches_[ipatch]->EMfields)->Jl_[imode] ;
                listJr_[imode][ipatch] = static_cast<ElectroMagn3DRZ*>(patches_[ipatch]->EMfields)->Jr_[imode] ;
                listJt_[imode][ipatch] = static_cast<ElectroMagn3DRZ*>(patches_[ipatch]->EMfields)->Jt_[imode] ;
                listrho_RZ_[imode][ipatch] =static_cast<ElectroMagn3DRZ*>(patches_[ipatch]->EMfields)->rho_RZ_[imode];
                listEl_[imode][ipatch] = static_cast<ElectroMagn3DRZ*>(patches_[ipatch]->EMfields)->El_[imode] ;
                listEr_[imode][ipatch] = static_cast<ElectroMagn3DRZ*>(patches_[ipatch]->EMfields)->Er_[imode] ;
                listEt_[imode][ipatch] = static_cast<ElectroMagn3DRZ*>(patches_[ipatch]->EMfields)->Et_[imode] ;
                listBl_[imode][ipatch] = static_cast<ElectroMagn3DRZ*>(patches_[ipatch]->EMfields)->Bl_[imode] ;
                listBr_[imode][ipatch] = static_cast<ElectroMagn3DRZ*>(patches_[ipatch]->EMfields)->Br_[imode] ;
                listBt_[imode][ipatch] = static_cast<ElectroMagn3DRZ*>(patches_[ipatch]->EMfields)->Bt_[imode] ;
            }
        }
    }
    #ifdef _TODO_RZ
    // Manage RZ & cartesian
    #endif


    B_localx.clear();
    B_MPIx.clear();

    B1_localy.clear();
    B1_MPIy.clear();

    B2_localz.clear();
    B2_MPIz.clear();

    for (unsigned int ipatch=0 ; ipatch < size() ; ipatch++) {
        densities[ipatch         ] = patches_[ipatch]->EMfields->Jx_ ;
        densities[ipatch+  size()] = patches_[ipatch]->EMfields->Jy_ ;
        densities[ipatch+2*size()] = patches_[ipatch]->EMfields->Jz_ ;

        Bs0[ipatch       ] = patches_[ipatch]->EMfields->By_ ;
        Bs0[ipatch+size()] = patches_[ipatch]->EMfields->Bz_ ;

        // TO DO , B size depend of nDim
        // Pas grave, au pire inutil
        Bs1[ipatch       ] = patches_[ipatch]->EMfields->Bx_ ;
        Bs1[ipatch+size()] = patches_[ipatch]->EMfields->Bz_ ;

        // TO DO , B size depend of nDim
        // Pas grave, au pire inutil
        Bs2[ipatch       ] = patches_[ipatch]->EMfields->Bx_ ;
        Bs2[ipatch+size()] = patches_[ipatch]->EMfields->By_ ;
    }

    for (unsigned int ipatch=0 ; ipatch < size() ; ipatch++) {
        if ( (*this)(ipatch)->has_an_MPI_neighbor( 0 ) ) {
            MPIxIdx.push_back(ipatch);
        }
        if ( (*this)(ipatch)->has_an_local_neighbor( 0 ) ) {
            LocalxIdx.push_back(ipatch);
        }
    }
    if (nDim>1) {
        for (unsigned int ipatch=0 ; ipatch < size() ; ipatch++) {
            if ( (*this)(ipatch)->has_an_MPI_neighbor( 1 ) ) {
                MPIyIdx.push_back(ipatch);
            }
            if ( (*this)(ipatch)->has_an_local_neighbor( 1 ) ) {
                LocalyIdx.push_back(ipatch);
            }
        }
        if (nDim>2) {
            for (unsigned int ipatch=0 ; ipatch < size() ; ipatch++) {

                if ( (*this)(ipatch)->has_an_MPI_neighbor( 2 ) ) {
                    MPIzIdx.push_back(ipatch);
                }
                if ( (*this)(ipatch)->has_an_local_neighbor( 2 ) ) {
                    LocalzIdx.push_back(ipatch);
                }
            }
        }
    }

    B_MPIx.resize( 2*MPIxIdx.size() );
    B_localx.resize( 2*LocalxIdx.size() );
    B1_MPIy.resize( 2*MPIyIdx.size() );
    B1_localy.resize( 2*LocalyIdx.size() );
    B2_MPIz.resize( 2*MPIzIdx.size() );
    B2_localz.resize( 2*LocalzIdx.size() );

    densitiesMPIx.resize( 3*MPIxIdx.size() );
    densitiesLocalx.resize( 3*LocalxIdx.size() );
    densitiesMPIy.resize( 3*MPIyIdx.size() );
    densitiesLocaly.resize( 3*LocalyIdx.size() );
    densitiesMPIz.resize( 3*MPIzIdx.size() );
    densitiesLocalz.resize( 3*LocalzIdx.size() );

    int mpix(0), locx(0), mpiy(0), locy(0), mpiz(0), locz(0);

    for (unsigned int ipatch=0 ; ipatch < size() ; ipatch++) {

        if ( (*this)(ipatch)->has_an_MPI_neighbor( 0 ) ) {
            B_MPIx[mpix               ] = patches_[ipatch]->EMfields->By_;
            B_MPIx[mpix+MPIxIdx.size()] = patches_[ipatch]->EMfields->Bz_;

            densitiesMPIx[mpix                 ] = patches_[ipatch]->EMfields->Jx_;
            densitiesMPIx[mpix+  MPIxIdx.size()] = patches_[ipatch]->EMfields->Jy_;
            densitiesMPIx[mpix+2*MPIxIdx.size()] = patches_[ipatch]->EMfields->Jz_;
            mpix++;
        }
        if ( (*this)(ipatch)->has_an_local_neighbor( 0 ) ) {
            B_localx[locx                 ] = patches_[ipatch]->EMfields->By_;
            B_localx[locx+LocalxIdx.size()] = patches_[ipatch]->EMfields->Bz_;

            densitiesLocalx[locx                   ] = patches_[ipatch]->EMfields->Jx_;
            densitiesLocalx[locx+  LocalxIdx.size()] = patches_[ipatch]->EMfields->Jy_;
            densitiesLocalx[locx+2*LocalxIdx.size()] = patches_[ipatch]->EMfields->Jz_;
            locx++;
        }
    }
    if (nDim>1) {
        for (unsigned int ipatch=0 ; ipatch < size() ; ipatch++) {
            if ( (*this)(ipatch)->has_an_MPI_neighbor( 1 ) ) {
                B1_MPIy[mpiy               ] = patches_[ipatch]->EMfields->Bx_;
                B1_MPIy[mpiy+MPIyIdx.size()] = patches_[ipatch]->EMfields->Bz_;

                densitiesMPIy[mpiy                 ] = patches_[ipatch]->EMfields->Jx_;
                densitiesMPIy[mpiy+  MPIyIdx.size()] = patches_[ipatch]->EMfields->Jy_;
                densitiesMPIy[mpiy+2*MPIyIdx.size()] = patches_[ipatch]->EMfields->Jz_;
                mpiy++;
            }
            if ( (*this)(ipatch)->has_an_local_neighbor( 1 ) ) {
                B1_localy[locy                 ] = patches_[ipatch]->EMfields->Bx_;
                B1_localy[locy+LocalyIdx.size()] = patches_[ipatch]->EMfields->Bz_;

                densitiesLocaly[locy                   ] = patches_[ipatch]->EMfields->Jx_;
                densitiesLocaly[locy+  LocalyIdx.size()] = patches_[ipatch]->EMfields->Jy_;
                densitiesLocaly[locy+2*LocalyIdx.size()] = patches_[ipatch]->EMfields->Jz_;
                locy++;
            }
        }
        if (nDim>2) {
            for (unsigned int ipatch=0 ; ipatch < size() ; ipatch++) {
                if ( (*this)(ipatch)->has_an_MPI_neighbor( 2 ) ) {
                    B2_MPIz[mpiz               ] = patches_[ipatch]->EMfields->Bx_;
                    B2_MPIz[mpiz+MPIzIdx.size()] = patches_[ipatch]->EMfields->By_;

                    densitiesMPIz[mpiz                 ] = patches_[ipatch]->EMfields->Jx_;
                    densitiesMPIz[mpiz+  MPIzIdx.size()] = patches_[ipatch]->EMfields->Jy_;
                    densitiesMPIz[mpiz+2*MPIzIdx.size()] = patches_[ipatch]->EMfields->Jz_;
                    mpiz++;
                }
                if ( (*this)(ipatch)->has_an_local_neighbor( 2 ) ) {
                    B2_localz[locz                 ] = patches_[ipatch]->EMfields->Bx_;
                    B2_localz[locz+LocalzIdx.size()] = patches_[ipatch]->EMfields->By_;

                    densitiesLocalz[locz                   ] = patches_[ipatch]->EMfields->Jx_;
                    densitiesLocalz[locz+  LocalzIdx.size()] = patches_[ipatch]->EMfields->Jy_;
                    densitiesLocalz[locz+2*LocalzIdx.size()] = patches_[ipatch]->EMfields->Jz_;
                    locz++;
                }
            }
        }

    }

    if ( !dynamic_cast<ElectroMagn3DRZ*>(patches_[0]->EMfields) ) {
        for ( unsigned int ifields = 0 ; ifields < listBx_.size() ; ifields++ ) {
            listJx_[ifields]->MPIbuff.defineTags( patches_[ifields], 1 );
            listJy_[ifields]->MPIbuff.defineTags( patches_[ifields], 2 );
            listJz_[ifields]->MPIbuff.defineTags( patches_[ifields], 3 );
            listBx_[ifields]->MPIbuff.defineTags( patches_[ifields], 6 );
            listBy_[ifields]->MPIbuff.defineTags( patches_[ifields], 7 );
            listBz_[ifields]->MPIbuff.defineTags( patches_[ifields], 8 );

            listrho_[ifields]->MPIbuff.defineTags( patches_[ifields], 4 );
        }
    }
    else {
        unsigned int nmodes = static_cast<ElectroMagn3DRZ*>(patches_[0]->EMfields)->El_.size();
        for (unsigned int imode=0 ; imode < nmodes ; imode++) {
            for ( unsigned int ifields = 0 ; ifields < listBl_[imode].size() ; ifields++ ) {
                listJl_[imode][ifields]->MPIbuff.defineTags( patches_[ifields], 1 );
                listJr_[imode][ifields]->MPIbuff.defineTags( patches_[ifields], 2 );
                listJr_[imode][ifields]->MPIbuff.defineTags( patches_[ifields], 3 );
                listBl_[imode][ifields]->MPIbuff.defineTags( patches_[ifields], 6 );
                listBr_[imode][ifields]->MPIbuff.defineTags( patches_[ifields], 7 );
                listBt_[imode][ifields]->MPIbuff.defineTags( patches_[ifields], 8 );

                listrho_RZ_[imode][ifields]->MPIbuff.defineTags( patches_[ifields], 4 );
            }
        }
    }
}



void VectorPatch::update_field_list(int ispec)
{
    #pragma omp barrier
    #pragma omp single
    {
        if(patches_[0]->EMfields->Jx_s [ispec]) listJxs_.resize( size() ) ;
        else
            listJxs_.clear();
        if(patches_[0]->EMfields->Jy_s [ispec]) listJys_.resize( size() ) ;
        else
            listJys_.clear();
        if(patches_[0]->EMfields->Jz_s [ispec]) listJzs_.resize( size() ) ;
        else
            listJzs_.clear();
        if(patches_[0]->EMfields->rho_s[ispec]) listrhos_.resize( size() ) ;
        else
            listrhos_.clear();
    }

    #pragma omp for schedule(static)
    for (unsigned int ipatch=0 ; ipatch < size() ; ipatch++) {
        if(patches_[ipatch]->EMfields->Jx_s [ispec]) {
            listJxs_ [ipatch] = patches_[ipatch]->EMfields->Jx_s [ispec];
            listJxs_ [ipatch]->MPIbuff.defineTags( patches_[ipatch], 0 );
        }
        if(patches_[ipatch]->EMfields->Jy_s [ispec]) {
            listJys_ [ipatch] = patches_[ipatch]->EMfields->Jy_s [ispec];
            listJys_ [ipatch]->MPIbuff.defineTags( patches_[ipatch], 0 );
        }
        if(patches_[ipatch]->EMfields->Jz_s [ispec]) {
            listJzs_ [ipatch] = patches_[ipatch]->EMfields->Jz_s [ispec];
            listJzs_ [ipatch]->MPIbuff.defineTags( patches_[ipatch], 0 );
        }
        if(patches_[ipatch]->EMfields->rho_s[ispec]) {
            listrhos_[ipatch] = patches_[ipatch]->EMfields->rho_s[ispec];
            listrhos_[ipatch]->MPIbuff.defineTags( patches_[ipatch], 0 );
        }
    }




}


void VectorPatch::applyAntennas(double time)
{
#ifdef  __DEBUG
    if( nAntennas>0 ) {
        #pragma omp single
        TITLE("Applying antennas at time t = " << time);
    }
#endif

    // Loop antennas
    for(unsigned int iAntenna=0; iAntenna<nAntennas; iAntenna++) {

        // Get intensity from antenna of the first patch
        #pragma omp single
        antenna_intensity = patches_[0]->EMfields->antennas[iAntenna].time_profile->valueAt(time);

        // Loop patches to apply
        #pragma omp for schedule(static)
        for (unsigned int ipatch=0 ; ipatch<size() ; ipatch++) {
            patches_[ipatch]->EMfields->applyAntenna(iAntenna, antenna_intensity);
        }

    }
}

// For each patch, apply the collisions
void VectorPatch::applyCollisions(Params& params, int itime, Timers & timers)
{
    timers.collisions.restart();

    if (Collisions::debye_length_required)
        #pragma omp for schedule(static)
        for (unsigned int ipatch=0 ; ipatch<size() ; ipatch++)
            Collisions::calculate_debye_length(params,patches_[ipatch]);

    unsigned int ncoll = patches_[0]->vecCollisions.size();

    #pragma omp for schedule(static)
    for (unsigned int ipatch=0 ; ipatch<size() ; ipatch++)
        for (unsigned int icoll=0 ; icoll<ncoll; icoll++)
            patches_[ipatch]->vecCollisions[icoll]->collide(params,patches_[ipatch],itime, localDiags);

    #pragma omp single
    for (unsigned int icoll=0 ; icoll<ncoll; icoll++)
        Collisions::debug(params, itime, icoll, *this);
    #pragma omp barrier

    timers.collisions.update();
}


// For each patch, apply external fields
void VectorPatch::applyExternalFields()
{
    for (unsigned int ipatch=0 ; ipatch<size() ; ipatch++)
        patches_[ipatch]->EMfields->applyExternalFields( (*this)(ipatch) ); // Must be patch
}


// Print information on the memory consumption
void VectorPatch::check_memory_consumption(SmileiMPI* smpi)
{
    long int particlesMem(0);
    for (unsigned int ipatch=0 ; ipatch<size() ; ipatch++)
        for (unsigned int ispec=0 ; ispec<patches_[ipatch]->vecSpecies.size(); ispec++)
            particlesMem += patches_[ipatch]->vecSpecies[ispec]->getMemFootPrint();
    MESSAGE( 1, "(Master) Species part = " << (int)( (double)particlesMem / 1024./1024.) << " MB" );

    long double dParticlesMem = (double)particlesMem / 1024./1024./1024.;
    MPI_Reduce( smpi->isMaster()?MPI_IN_PLACE:&dParticlesMem, &dParticlesMem, 1, MPI_LONG_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD );
    MESSAGE( 1, setprecision(3) << "Global Species part = " << dParticlesMem << " GB" );

    MPI_Reduce( smpi->isMaster()?MPI_IN_PLACE:&particlesMem, &particlesMem, 1, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD );
    MESSAGE( 1, "Max Species part = " << (int)( (double)particlesMem / 1024./1024.) << " MB" );

    // fieldsMem contains field per species and average fields
    long int fieldsMem(0);
    for (unsigned int ipatch=0 ; ipatch<size() ; ipatch++)
        fieldsMem += patches_[ipatch]->EMfields->getMemFootPrint();
    MESSAGE( 1, "(Master) Fields part = " << (int)( (double)fieldsMem / 1024./1024.) << " MB" );

    long double dFieldsMem = (double)fieldsMem / 1024./1024./1024.;
    MPI_Reduce( smpi->isMaster()?MPI_IN_PLACE:&dFieldsMem, &dFieldsMem, 1, MPI_LONG_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD );
    MESSAGE( 1, setprecision(3) << "Global Fields part = " << dFieldsMem << " GB" );

    MPI_Reduce( smpi->isMaster()?MPI_IN_PLACE:&fieldsMem, &fieldsMem, 1, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD );
    MESSAGE( 1, "Max Fields part = " << (int)( (double)fieldsMem / 1024./1024.) << " MB" );


    for (unsigned int idiags=0 ; idiags<globalDiags.size() ; idiags++) {
        // fieldsMem contains field per species
        long int diagsMem(0);
        diagsMem += globalDiags[idiags]->getMemFootPrint();

        long double dDiagsMem = (double)diagsMem / 1024./1024./1024.;
        MPI_Reduce( smpi->isMaster()?MPI_IN_PLACE:&dDiagsMem, &dDiagsMem, 1, MPI_LONG_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD );
        if (dDiagsMem>0.) {
            MESSAGE( 1, "(Master) " <<  globalDiags[idiags]->filename << "  = " << (int)( (double)diagsMem / 1024./1024.) << " MB" );
            MESSAGE( 1, setprecision(3) << "Global " <<  globalDiags[idiags]->filename << " = " << dDiagsMem << " GB" );
        }

        MPI_Reduce( smpi->isMaster()?MPI_IN_PLACE:&diagsMem, &diagsMem, 1, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD );
        if (dDiagsMem>0.)
            MESSAGE( 1, "Max " <<  globalDiags[idiags]->filename << " = " << (int)( (double)diagsMem / 1024./1024.) << " MB" );
    }

    for (unsigned int idiags=0 ; idiags<localDiags.size() ; idiags++) {
        // fieldsMem contains field per species
        long int diagsMem(0);
        diagsMem += localDiags[idiags]->getMemFootPrint();

        long double dDiagsMem = (double)diagsMem / 1024./1024./1024.;
        MPI_Reduce( smpi->isMaster()?MPI_IN_PLACE:&dDiagsMem, &dDiagsMem, 1, MPI_LONG_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD );
        if (dDiagsMem>0.) {
            MESSAGE( 1, "(Master) " <<  localDiags[idiags]->filename << "  = " << (int)( (double)diagsMem / 1024./1024.) << " MB" );
            MESSAGE( 1, setprecision(3) << "Global " <<  localDiags[idiags]->filename << " = " << dDiagsMem << " GB" );
        }

        MPI_Reduce( smpi->isMaster()?MPI_IN_PLACE:&diagsMem, &diagsMem, 1, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD );
        if (dDiagsMem>0.)
            MESSAGE( 1, "Max " <<  localDiags[idiags]->filename << " = " << (int)( (double)diagsMem / 1024./1024.) << " MB" );
    }

    // Read value in /proc/pid/status
    //Tools::printMemFootPrint( "End Initialization" );
}


// Print information on the memory consumption
void VectorPatch::check_expected_disk_usage( SmileiMPI* smpi, Params& params, Checkpoint& checkpoint)
{
    if( smpi->isMaster() ){
        
        // Find the initial and final timesteps for this simulation
        int istart = 0, istop = params.n_time;
        // If restarting simulation define the starting point
        if( params.restart ) {
            istart = checkpoint.this_run_start_step+1;
        }
        // If leaving the simulation after dump, define the stopping point
        if( checkpoint.dump_step > 0 && checkpoint.exit_after_dump ) {
            int ncheckpoint = (istart/(int)checkpoint.dump_step) + 1;
            int nextdumptime = ncheckpoint * (int)checkpoint.dump_step;
            if( nextdumptime < istop ) istop = nextdumptime;
        }
        
        MESSAGE(1, "Diagnostics expected disk usage:" );
        // Calculate the footprint from local then global diagnostics
        uint64_t total_footprint = 0;
        for (unsigned int idiags=0 ; idiags<localDiags.size() ; idiags++) {
            uint64_t footprint = localDiags[idiags]->getDiskFootPrint(istart, istop, patches_[0]);
            total_footprint += footprint;
            MESSAGE(2, "File " << localDiags[idiags]->filename << ": " << Tools::printBytes(footprint));
        }
        for (unsigned int idiags=0 ; idiags<globalDiags.size() ; idiags++) {
            uint64_t footprint = globalDiags[idiags]->getDiskFootPrint(istart, istop, patches_[0]);
            total_footprint += footprint;
            MESSAGE(2, "File " << globalDiags[idiags]->filename << ": " << Tools::printBytes(footprint));
        }
        
        MESSAGE(1, "Total disk usage: " << Tools::printBytes(total_footprint) );
        
    }
}


