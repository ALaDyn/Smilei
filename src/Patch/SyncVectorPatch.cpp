
#include "SyncVectorPatch.h"

#include <vector>

#include "VectorPatch.h"
#include "Params.h"
#include "SmileiMPI.h"

using namespace std;

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
// ----------------------------------------------       PARTICLES         ----------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------

void SyncVectorPatch::exchangeParticles(VectorPatch& vecPatches, int ispec, Params &params, SmileiMPI* smpi, Timers &timers, int itime)
{
    #pragma omp for schedule(runtime)
    for (unsigned int ipatch=0 ; ipatch<vecPatches.size() ; ipatch++) {
        vecPatches(ipatch)->initExchParticles(smpi, ispec, params);
    }

    // Per direction
    for (unsigned int iDim=0 ; iDim<1 ; iDim++) {
        #pragma omp for schedule(runtime)
        for (unsigned int ipatch=0 ; ipatch<vecPatches.size() ; ipatch++) {
            vecPatches(ipatch)->initCommParticles(smpi, ispec, params, iDim, &vecPatches);
        }

//        #pragma omp for schedule(runtime)
//        for (unsigned int ipatch=0 ; ipatch<vecPatches.size() ; ipatch++) {
//            vecPatches(ipatch)->CommParticles(smpi, ispec, params, iDim, &vecPatches);
//        }
//        #pragma omp for schedule(runtime)
//        for (unsigned int ipatch=0 ; ipatch<vecPatches.size() ; ipatch++) {
//            vecPatches(ipatch)->finalizeCommParticles(smpi, ispec, params, iDim, &vecPatches);
//        }
    }

//    #pragma omp for schedule(runtime)
//    for (unsigned int ipatch=0 ; ipatch<vecPatches.size() ; ipatch++)
//        vecPatches(ipatch)->vecSpecies[ispec]->sort_part();
}


void SyncVectorPatch::finalize_and_sort_parts(VectorPatch& vecPatches, int ispec, Params &params, SmileiMPI* smpi, Timers &timers, int itime)
{
    #pragma omp for schedule(runtime)
    for (unsigned int ipatch=0 ; ipatch<vecPatches.size() ; ipatch++) {
        vecPatches(ipatch)->CommParticles(smpi, ispec, params, 0, &vecPatches);
    }
    #pragma omp for schedule(runtime)
    for (unsigned int ipatch=0 ; ipatch<vecPatches.size() ; ipatch++) {
        vecPatches(ipatch)->finalizeCommParticles(smpi, ispec, params, 0, &vecPatches);
    }

    // Per direction
    for (unsigned int iDim=1 ; iDim<params.nDim_particle ; iDim++) {
        #pragma omp for schedule(runtime)
        for (unsigned int ipatch=0 ; ipatch<vecPatches.size() ; ipatch++) {
            vecPatches(ipatch)->initCommParticles(smpi, ispec, params, iDim, &vecPatches);
        }

        #pragma omp for schedule(runtime)
        for (unsigned int ipatch=0 ; ipatch<vecPatches.size() ; ipatch++) {
            vecPatches(ipatch)->CommParticles(smpi, ispec, params, iDim, &vecPatches);
        }
        #pragma omp for schedule(runtime)
        for (unsigned int ipatch=0 ; ipatch<vecPatches.size() ; ipatch++) {
            vecPatches(ipatch)->finalizeCommParticles(smpi, ispec, params, iDim, &vecPatches);
        }
    }

//    #pragma omp for schedule(runtime)
//    for (unsigned int ipatch=0 ; ipatch<vecPatches.size() ; ipatch++)
//        vecPatches(ipatch)->injectParticles(smpi, ispec, params, params.nDim_particle-1, &vecPatches); // wait


    /*
    // Debugging
    for (unsigned int ipatch=0 ; ipatch<vecPatches.size() ; ipatch++)
    {
        unsigned int npart = vecPatches(ipatch)->vecSpecies[ispec]->particles->size();
        for( unsigned int i=0; i<npart; i++ ) {
            if (vecPatches(ipatch)->vecSpecies[ispec]->particles->position(0,i)< vecPatches(ipatch)->getDomainLocalMin(0)
             || vecPatches(ipatch)->vecSpecies[ispec]->particles->position(0,i) > vecPatches(ipatch)->getDomainLocalMax(0)
             || vecPatches(ipatch)->vecSpecies[ispec]->particles->position(1,i) < vecPatches(ipatch)->getDomainLocalMin(1)
             || vecPatches(ipatch)->vecSpecies[ispec]->particles->position(1,i) > vecPatches(ipatch)->getDomainLocalMax(1))
             {
            cerr << setprecision(12)
                 << " Patch: " << ipatch << "/" << vecPatches.size()-1
                 << " Species: " << ispec
                 << " ipart: " << i
                 << " " << vecPatches(ipatch)->vecSpecies[ispec]->particles->weight(i)
                 << " " << vecPatches(ipatch)->vecSpecies[ispec]->particles->charge(0)
                 << " " << vecPatches(ipatch)->getDomainLocalMin(0)
                 << "<" << vecPatches(ipatch)->vecSpecies[ispec]->particles->position(0,i)
                 << "<" << vecPatches(ipatch)->getDomainLocalMax(0)
                 << " " << vecPatches(ipatch)->getDomainLocalMin(1)
                 << "<" << vecPatches(ipatch)->vecSpecies[ispec]->particles->position(1,i)
                 << "<" << vecPatches(ipatch)->getDomainLocalMax(1)
                 << " " << vecPatches(ipatch)->vecSpecies[ispec]->particles->momentum(0,i)
                 << " " << vecPatches(ipatch)->vecSpecies[ispec]->particles->momentum(1,i)
                 << std::endl;
            }
        }
    }*/

}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
// ----------------------------------------------       DENSITIES         ----------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------

void SyncVectorPatch::sumRhoJ(Params& params, VectorPatch& vecPatches, Timers &timers, int itime)
{
    SyncVectorPatch::sum_all_components( vecPatches.densities , vecPatches, timers, itime );
    if( (vecPatches.diag_flag) || (params.is_spectral) )SyncVectorPatch::sum( vecPatches.listrho_, vecPatches, timers, itime );
}

void SyncVectorPatch::sumRhoJs(Params& params, VectorPatch& vecPatches, int ispec , Timers &timers, int itime)
{

    if(vecPatches.listJxs_ .size()>0) SyncVectorPatch::sum( vecPatches.listJxs_ , vecPatches, timers, itime  );
    if(vecPatches.listJys_ .size()>0) SyncVectorPatch::sum( vecPatches.listJys_ , vecPatches, timers, itime  );
    if(vecPatches.listJzs_ .size()>0) SyncVectorPatch::sum( vecPatches.listJzs_ , vecPatches, timers, itime  );
    if(vecPatches.listrhos_.size()>0) SyncVectorPatch::sum( vecPatches.listrhos_, vecPatches, timers, itime  );
}


void SyncVectorPatch::sum( std::vector<Field*> fields, VectorPatch& vecPatches, Timers &timers, int itime )
{
    unsigned int nx_, ny_, nz_, h0, oversize[3], n_space[3], gsp[3];
    double *pt1,*pt2;
    h0 = vecPatches(0)->hindex;

    int nPatches( vecPatches.size() );

    oversize[0] = vecPatches(0)->EMfields->oversize[0];
    oversize[1] = vecPatches(0)->EMfields->oversize[1];
    oversize[2] = vecPatches(0)->EMfields->oversize[2];

    n_space[0] = vecPatches(0)->EMfields->n_space[0];
    n_space[1] = vecPatches(0)->EMfields->n_space[1];
    n_space[2] = vecPatches(0)->EMfields->n_space[2];

    unsigned int nComp = fields.size()/nPatches;

    // -----------------
    // Sum per direction :

    // iDim = 0, initialize comms : Isend/Irecv
    #pragma omp for schedule(static)
    for (unsigned int ifield=0 ; ifield<fields.size() ; ifield++) {
        unsigned int ipatch = ifield%nPatches;
        vecPatches(ipatch)->initSumField( fields[ifield], 0 );
    }

//    #pragma omp for schedule(static)
//    for (unsigned int ifield=0 ; ifield<fields.size() ; ifield++) {
//        unsigned int ipatch = ifield%nPatches;
//        vecPatches(ipatch)->testSumField( fields[ifield], 0 );
//    }

    // iDim = 0, local
    for (unsigned int icomp=0 ; icomp<nComp ; icomp++) {
        nx_ = fields[icomp*nPatches]->dims_[0];
        ny_ = 1;
        nz_ = 1;
        if (fields[icomp*nPatches]->dims_.size()>1) {
            ny_ = fields[icomp*nPatches]->dims_[1];
            if (fields[icomp*nPatches]->dims_.size()>2)
                nz_ = fields[icomp*nPatches]->dims_[2];
        }
        gsp[0] = 1+2*oversize[0]+fields[icomp*nPatches]->isDual_[0]; //Ghost size primal
        #pragma omp for schedule(static) private(pt1,pt2)
        for (unsigned int ifield=icomp*nPatches ; ifield<(icomp+1)*nPatches ; ifield++) {
            unsigned int ipatch = ifield%nPatches;
            if (vecPatches(ipatch)->MPI_me_ == vecPatches(ipatch)->MPI_neighbor_[0][0]){
                //The patch to the west belongs to the same MPI process than I.
                pt1 = &(*fields[vecPatches(ipatch)->neighbor_[0][0]-h0+icomp*nPatches])(n_space[0]*ny_*nz_);
                pt2 = &(*fields[ifield])(0);
                //Sum 2 ==> 1
                for (unsigned int i = 0; i < gsp[0]* ny_*nz_ ; i++) pt1[i] += pt2[i];
                //Copy back the results to 2
                memcpy( pt2, pt1, gsp[0]*ny_*nz_*sizeof(double));
            }
        }
    }

    // iDim = 0, finalize (waitall)
    #pragma omp for schedule(static)
    for (unsigned int ifield=0 ; ifield<fields.size() ; ifield++){
        unsigned int ipatch = ifield%nPatches;
        vecPatches(ipatch)->finalizeSumField( fields[ifield], 0 );
    }
    // END iDim = 0 sync
    // -----------------

    if (fields[0]->dims_.size()>1) {
        // -----------------
        // Sum per direction :

        // iDim = 1, initialize comms : Isend/Irecv
        #pragma omp for schedule(static)
        for (unsigned int ifield=0 ; ifield<fields.size() ; ifield++) {
            unsigned int ipatch = ifield%nPatches;
            vecPatches(ipatch)->initSumField( fields[ifield], 1 );
        }

//        #pragma omp for schedule(static)
//        for (unsigned int ifield=0 ; ifield<fields.size() ; ifield++) {
//            unsigned int ipatch = ifield%nPatches;
//            vecPatches(ipatch)->testSumField( fields[ifield], 1 );
//        }

        // iDim = 1, local
        for (unsigned int icomp=0 ; icomp<nComp ; icomp++) {
            nx_ = fields[icomp*nPatches]->dims_[0];
            ny_ = 1;
            nz_ = 1;
            if (fields[icomp*nPatches]->dims_.size()>1) {
                ny_ = fields[icomp*nPatches]->dims_[1];
                if (fields[icomp*nPatches]->dims_.size()>2)
                    nz_ = fields[icomp*nPatches]->dims_[2];
            }
            gsp[0] = 1+2*oversize[0]+fields[icomp*nPatches]->isDual_[0]; //Ghost size primal
            gsp[1] = 1+2*oversize[1]+fields[icomp*nPatches]->isDual_[1]; //Ghost size primal

            #pragma omp for schedule(static) private(pt1,pt2)
            for (unsigned int ifield=icomp*nPatches ; ifield<(icomp+1)*nPatches ; ifield++) {
                unsigned int ipatch = ifield%nPatches;
                if (vecPatches(ipatch)->MPI_me_ == vecPatches(ipatch)->MPI_neighbor_[1][0]){
                    //The patch to the south belongs to the same MPI process than I.
                    pt1 = &(*fields[vecPatches(ipatch)->neighbor_[1][0]-h0+icomp*nPatches])(n_space[1]*nz_);
                    pt2 = &(*fields[ifield])(0);
                    for (unsigned int j = 0; j < nx_ ; j++){
                        for (unsigned int i = 0; i < gsp[1]*nz_ ; i++) pt1[i] += pt2[i];
                        memcpy( pt2, pt1, gsp[1]*nz_*sizeof(double));
                        pt1 += ny_*nz_;
                        pt2 += ny_*nz_;
                    }
                }
            }
        }

        // iDim = 1, finalize (waitall)
        #pragma omp for schedule(static)
        for (unsigned int ifield=0 ; ifield<fields.size() ; ifield++){
            unsigned int ipatch = ifield%nPatches;
            vecPatches(ipatch)->finalizeSumField( fields[ifield], 1 );
        }
        // END iDim = 1 sync
        // -----------------

        if (fields[0]->dims_.size()>2) {
            // -----------------
            // Sum per direction :

            // iDim = 2, initialize comms : Isend/Irecv
            #pragma omp for schedule(static)
            for (unsigned int ifield=0 ; ifield<fields.size() ; ifield++) {
                unsigned int ipatch = ifield%nPatches;
                vecPatches(ipatch)->initSumField( fields[ifield], 2 );
            }

            // iDim = 2 local
            for (unsigned int icomp=0 ; icomp<nComp ; icomp++) {
                nx_ = fields[icomp*nPatches]->dims_[0];
                ny_ = 1;
                nz_ = 1;
                if (fields[icomp*nPatches]->dims_.size()>1) {
                    ny_ = fields[icomp*nPatches]->dims_[1];
                    if (fields[icomp*nPatches]->dims_.size()>2)
                        nz_ = fields[icomp*nPatches]->dims_[2];
                }
                gsp[0] = 1+2*oversize[0]+fields[icomp*nPatches]->isDual_[0]; //Ghost size primal
                gsp[1] = 1+2*oversize[1]+fields[icomp*nPatches]->isDual_[1]; //Ghost size primal
                gsp[2] = 1+2*oversize[2]+fields[icomp*nPatches]->isDual_[2]; //Ghost size primal
                #pragma omp for schedule(static) private(pt1,pt2)
                for (unsigned int ifield=icomp*nPatches ; ifield<(icomp+1)*nPatches ; ifield++) {
                    unsigned int ipatch = ifield%nPatches;
                    if (vecPatches(ipatch)->MPI_me_ == vecPatches(ipatch)->MPI_neighbor_[2][0]){
                        //The patch below me belongs to the same MPI process than I.
                        pt1 = &(*fields[vecPatches(ipatch)->neighbor_[2][0]-h0+icomp*nPatches])(n_space[2]);
                        pt2 = &(*fields[ifield])(0);
                        for (unsigned int j = 0; j < nx_*ny_ ; j++){
                            for (unsigned int i = 0; i < gsp[2] ; i++){
                                pt1[i] += pt2[i];
                                pt2[i] =  pt1[i];
                            }
                            pt1 += nz_;
                            pt2 += nz_;
                        }
                    }
                }
            }

            // iDim = 2, complete non local sync through MPIfinalize (waitall)
            #pragma omp for schedule(static)
            for (unsigned int ifield=0 ; ifield<fields.size() ; ifield++){
                unsigned int ipatch = ifield%nPatches;
                vecPatches(ipatch)->finalizeSumField( fields[ifield], 2 );
            }
            // END iDim = 2 sync
            // -----------------

        } // End if dims_.size()>2

    } // End if dims_.size()>1

}


void SyncVectorPatch::sum_all_components( std::vector<Field*>& fields, VectorPatch& vecPatches, Timers &timers, int itime )
{
    unsigned int h0, oversize[3], n_space[3];
    double *pt1,*pt2;
    h0 = vecPatches(0)->hindex;

    int nPatches( vecPatches.size() );

    oversize[0] = vecPatches(0)->EMfields->oversize[0];
    oversize[1] = vecPatches(0)->EMfields->oversize[1];
    oversize[2] = vecPatches(0)->EMfields->oversize[2];

    n_space[0] = vecPatches(0)->EMfields->n_space[0];
    n_space[1] = vecPatches(0)->EMfields->n_space[1];
    n_space[2] = vecPatches(0)->EMfields->n_space[2];

    int nDim = vecPatches(0)->EMfields->Jx_->dims_.size();

    // -----------------
    // Sum per direction :

    // iDim = 0, initialize comms : Isend/Irecv
    unsigned int nPatchMPIx = vecPatches.MPIxIdx.size();
    #pragma omp for schedule(static)
    for (unsigned int ifield=0 ; ifield<nPatchMPIx ; ifield++) {
        unsigned int ipatch = vecPatches.MPIxIdx[ifield];
        vecPatches(ipatch)->initSumField( vecPatches.densitiesMPIx[ifield             ], 0 ); // Jx
        vecPatches(ipatch)->initSumField( vecPatches.densitiesMPIx[ifield+  nPatchMPIx], 0 ); // Jy
        vecPatches(ipatch)->initSumField( vecPatches.densitiesMPIx[ifield+2*nPatchMPIx], 0 ); // Jz
    }
    // iDim = 0, local
    int nFieldLocalx = vecPatches.densitiesLocalx.size()/3;
    for ( int icomp=0 ; icomp<3 ; icomp++ ) {
        if (nFieldLocalx==0) continue;

        unsigned int gsp[3];
        //unsigned int nx_ =  vecPatches.densitiesLocalx[icomp*nFieldLocalx]->dims_[0];
        unsigned int ny_ = 1;
        unsigned int nz_ = 1;
        if (nDim>1) {
            ny_ = vecPatches.densitiesLocalx[icomp*nFieldLocalx]->dims_[1];
            if (nDim>2)
                nz_ = vecPatches.densitiesLocalx[icomp*nFieldLocalx]->dims_[2];
        }
        gsp[0] = 1+2*oversize[0]+vecPatches.densitiesLocalx[icomp*nFieldLocalx]->isDual_[0]; //Ghost size primal

        unsigned int istart =  icomp   *nFieldLocalx;
        unsigned int iend    = (icomp+1)*nFieldLocalx;
        #pragma omp for schedule(static) private(pt1,pt2)
        for (unsigned int ifield=istart ; ifield<iend ; ifield++) {
            int ipatch = vecPatches.LocalxIdx[ ifield-icomp*nFieldLocalx ];
            if (vecPatches(ipatch)->MPI_me_ == vecPatches(ipatch)->MPI_neighbor_[0][0]){
                pt1 = &(fields[ vecPatches(ipatch)->neighbor_[0][0]-h0+icomp*nPatches ]->data_[n_space[0]*ny_*nz_]);
                pt2 = &(vecPatches.densitiesLocalx[ifield]->data_[0]);
                //Sum 2 ==> 1
                for (unsigned int i = 0; i < gsp[0]* ny_*nz_ ; i++) pt1[i] += pt2[i];
                //Copy back the results to 2
                memcpy( pt2, pt1, gsp[0]*ny_*nz_*sizeof(double));
            }
        }
    }

    // iDim = 0, finalize (waitall)
    #pragma omp for schedule(static)
    for (unsigned int ifield=0 ; ifield<nPatchMPIx ; ifield++) {
        unsigned int ipatch = vecPatches.MPIxIdx[ifield];
        vecPatches(ipatch)->finalizeSumField( vecPatches.densitiesMPIx[ifield             ], 0 ); // Jx
        vecPatches(ipatch)->finalizeSumField( vecPatches.densitiesMPIx[ifield+nPatchMPIx  ], 0 ); // Jy
        vecPatches(ipatch)->finalizeSumField( vecPatches.densitiesMPIx[ifield+2*nPatchMPIx], 0 ); // Jz
    }
    // END iDim = 0 sync
    // -----------------

    if (nDim>1) {
        // -----------------
        // Sum per direction :

        // iDim = 1, initialize comms : Isend/Irecv
        unsigned int nPatchMPIy = vecPatches.MPIyIdx.size();
        #pragma omp for schedule(static)
        for (unsigned int ifield=0 ; ifield<nPatchMPIy ; ifield++) {
            unsigned int ipatch = vecPatches.MPIyIdx[ifield];
            vecPatches(ipatch)->initSumField( vecPatches.densitiesMPIy[ifield             ], 1 ); // Jx
            vecPatches(ipatch)->initSumField( vecPatches.densitiesMPIy[ifield+nPatchMPIy  ], 1 ); // Jy
            vecPatches(ipatch)->initSumField( vecPatches.densitiesMPIy[ifield+2*nPatchMPIy], 1 ); // Jz
        }

        // iDim = 1,
        int nFieldLocaly = vecPatches.densitiesLocaly.size()/3;
        for ( int icomp=0 ; icomp<3 ; icomp++ ) {
            if (nFieldLocaly==0) continue;

            unsigned int gsp[3];
            unsigned int nx_ =  vecPatches.densitiesLocaly[icomp*nFieldLocaly]->dims_[0];
            unsigned int ny_ = 1;
            unsigned int nz_ = 1;
            if (nDim>1) {
                ny_ = vecPatches.densitiesLocaly[icomp*nFieldLocaly]->dims_[1];
                if (nDim>2)
                    nz_ = vecPatches.densitiesLocaly[icomp*nFieldLocaly]->dims_[2];
            }
            gsp[0] = 1+2*oversize[0]+vecPatches.densitiesLocaly[icomp*nFieldLocaly]->isDual_[0]; //Ghost size primal
            gsp[1] = 1+2*oversize[1]+vecPatches.densitiesLocaly[icomp*nFieldLocaly]->isDual_[1]; //Ghost size primal

            unsigned int istart =  icomp   *nFieldLocaly;
            unsigned int iend    = (icomp+1)*nFieldLocaly;
            #pragma omp for schedule(static) private(pt1,pt2)
            for (unsigned int ifield=istart ; ifield<iend ; ifield++) {
                int ipatch = vecPatches.LocalyIdx[ ifield-icomp*nFieldLocaly ];
                if (vecPatches(ipatch)->MPI_me_ == vecPatches(ipatch)->MPI_neighbor_[1][0]){
                    //The patch to the south belongs to the same MPI process than I.
                    pt1 = &(fields[vecPatches(ipatch)->neighbor_[1][0]-h0+icomp*nPatches]->data_[n_space[1]*nz_]);
                    pt2 = &(vecPatches.densitiesLocaly[ifield]->data_[0]);
                    for (unsigned int j = 0; j < nx_ ; j++){
                        for (unsigned int i = 0; i < gsp[1]*nz_ ; i++) pt1[i] += pt2[i];
                        memcpy( pt2, pt1, gsp[1]*nz_*sizeof(double));
                        pt1 += ny_*nz_;
                        pt2 += ny_*nz_;
                    }
                }
            }
        }

        // iDim = 1, finalize (waitall)
        #pragma omp for schedule(static)
        for (unsigned int ifield=0 ; ifield<nPatchMPIy ; ifield=ifield+1) {
            unsigned int ipatch = vecPatches.MPIyIdx[ifield];
            vecPatches(ipatch)->finalizeSumField( vecPatches.densitiesMPIy[ifield             ], 1 ); // Jx
            vecPatches(ipatch)->finalizeSumField( vecPatches.densitiesMPIy[ifield+nPatchMPIy  ], 1 ); // Jy
            vecPatches(ipatch)->finalizeSumField( vecPatches.densitiesMPIy[ifield+2*nPatchMPIy], 1 ); // Jz
        }
        // END iDim = 1 sync
        // -----------------

        if (nDim>2) {
            // -----------------
            // Sum per direction :

            // iDim = 2, initialize comms : Isend/Irecv
            unsigned int nPatchMPIz = vecPatches.MPIzIdx.size();
            #pragma omp for schedule(static)
            for (unsigned int ifield=0 ; ifield<nPatchMPIz ; ifield++) {
                unsigned int ipatch = vecPatches.MPIzIdx[ifield];
                vecPatches(ipatch)->initSumField( vecPatches.densitiesMPIz[ifield             ], 2 ); // Jx
                vecPatches(ipatch)->initSumField( vecPatches.densitiesMPIz[ifield+nPatchMPIz  ], 2 ); // Jy
                vecPatches(ipatch)->initSumField( vecPatches.densitiesMPIz[ifield+2*nPatchMPIz], 2 ); // Jz
            }

            // iDim = 2 local
            int nFieldLocalz = vecPatches.densitiesLocalz.size()/3;
            for ( int icomp=0 ; icomp<3 ; icomp++ ) {
            if (nFieldLocalz==0) continue;

                unsigned int gsp[3];
                unsigned int nx_ =  vecPatches.densitiesLocalz[icomp*nFieldLocalz]->dims_[0];
                unsigned int ny_ = 1;
                unsigned int nz_ = 1;
                if (nDim>1) {
                    ny_ = vecPatches.densitiesLocalz[icomp*nFieldLocalz]->dims_[1];
                    if (nDim>2)
                        nz_ = vecPatches.densitiesLocalz[icomp*nFieldLocalz]->dims_[2];
                }
                gsp[0] = 1+2*oversize[0]+vecPatches.densitiesLocalz[icomp*nFieldLocalz]->isDual_[0]; //Ghost size primal
                gsp[1] = 1+2*oversize[1]+vecPatches.densitiesLocalz[icomp*nFieldLocalz]->isDual_[1]; //Ghost size primal
                gsp[2] = 1+2*oversize[2]+vecPatches.densitiesLocalz[icomp*nFieldLocalz]->isDual_[2]; //Ghost size primal

                unsigned int istart  =  icomp   *nFieldLocalz;
                unsigned int iend    = (icomp+1)*nFieldLocalz;
                #pragma omp for schedule(static) private(pt1,pt2)
                for (unsigned int ifield=istart ; ifield<iend ; ifield++) {
                    int ipatch = vecPatches.LocalzIdx[ ifield-icomp*nFieldLocalz ];
                    if (vecPatches(ipatch)->MPI_me_ == vecPatches(ipatch)->MPI_neighbor_[2][0]){
                        //The patch below me belongs to the same MPI process than I.
                        pt1 = &(fields[vecPatches(ipatch)->neighbor_[2][0]-h0+icomp*nPatches]->data_[n_space[2]]);
                        pt2 = &(vecPatches.densitiesLocalz[ifield]->data_[0]);
                        for (unsigned int j = 0; j < nx_*ny_ ; j++){
                            for (unsigned int i = 0; i < gsp[2] ; i++){
                                pt1[i] += pt2[i];
                                pt2[i] =  pt1[i];
                            }
                            pt1 += nz_;
                            pt2 += nz_;
                        }
                    }
                }
            }

            // iDim = 2, complete non local sync through MPIfinalize (waitall)
            #pragma omp for schedule(static)
            for (unsigned int ifield=0 ; ifield<nPatchMPIz ; ifield=ifield+1) {
                unsigned int ipatch = vecPatches.MPIzIdx[ifield];
                vecPatches(ipatch)->finalizeSumField( vecPatches.densitiesMPIz[ifield             ], 2 ); // Jx
                vecPatches(ipatch)->finalizeSumField( vecPatches.densitiesMPIz[ifield+nPatchMPIz  ], 2 ); // Jy
                vecPatches(ipatch)->finalizeSumField( vecPatches.densitiesMPIz[ifield+2*nPatchMPIz], 2 ); // Jz
            }
            // END iDim = 2 sync
            // -----------------

        } // End if dims_.size()>2

    } // End if dims_.size()>1

}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
// ----------------------------------------------         FIELDS          ----------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------

void SyncVectorPatch::exchangeE( Params& params, VectorPatch& vecPatches )
{
    if (!params.full_B_exchange) {
        SyncVectorPatch::exchange_along_all_directions( vecPatches.listEx_, vecPatches );
        SyncVectorPatch::exchange_along_all_directions( vecPatches.listEy_, vecPatches );
        SyncVectorPatch::exchange_along_all_directions( vecPatches.listEz_, vecPatches );
    }
    else {
        SyncVectorPatch::exchange_synchronized_per_direction( vecPatches.listEx_, vecPatches );
        SyncVectorPatch::exchange_synchronized_per_direction( vecPatches.listEy_, vecPatches );
        SyncVectorPatch::exchange_synchronized_per_direction( vecPatches.listEz_, vecPatches );
    }
}

void SyncVectorPatch::finalizeexchangeE( Params& params, VectorPatch& vecPatches )
{

    if (!params.full_B_exchange) {
        SyncVectorPatch::finalize_exchange_along_all_directions( vecPatches.listEx_, vecPatches );
        SyncVectorPatch::finalize_exchange_along_all_directions( vecPatches.listEy_, vecPatches );
        SyncVectorPatch::finalize_exchange_along_all_directions( vecPatches.listEz_, vecPatches );
    }
}

void SyncVectorPatch::exchangeB( Params& params, VectorPatch& vecPatches )
{

    if (vecPatches.listBx_[0]->dims_.size()==1) {
        SyncVectorPatch::exchange_all_components_along_X( vecPatches.Bs0, vecPatches );
    }
    else if ( vecPatches.listBx_[0]->dims_.size()==2 ) {
        if (!params.full_B_exchange) {
            SyncVectorPatch::exchange_all_components_along_X( vecPatches.Bs0, vecPatches );
            SyncVectorPatch::exchange_all_components_along_Y( vecPatches.Bs1, vecPatches );
        }
        else {
            SyncVectorPatch::exchange_synchronized_per_direction( vecPatches.listBx_, vecPatches );
            SyncVectorPatch::exchange_synchronized_per_direction( vecPatches.listBy_, vecPatches );
            SyncVectorPatch::exchange_synchronized_per_direction( vecPatches.listBz_, vecPatches );
        }
    }
    else if ( vecPatches.listBx_[0]->dims_.size()==3 ) {
        if (!params.full_B_exchange) {
            SyncVectorPatch::exchange_all_components_along_X( vecPatches.Bs0, vecPatches );
            SyncVectorPatch::exchange_all_components_along_Y( vecPatches.Bs1, vecPatches );
            SyncVectorPatch::exchange_all_components_along_Z( vecPatches.Bs2, vecPatches );
        }
        else {
            SyncVectorPatch::exchange_synchronized_per_direction( vecPatches.listBx_, vecPatches );
            SyncVectorPatch::exchange_synchronized_per_direction( vecPatches.listBy_, vecPatches );
            SyncVectorPatch::exchange_synchronized_per_direction( vecPatches.listBz_, vecPatches );
        }
    }

}

void SyncVectorPatch::finalizeexchangeB( Params& params, VectorPatch& vecPatches )
{
    if (vecPatches.listBx_[0]->dims_.size()==1) {
        SyncVectorPatch::finalize_exchange_all_components_along_X( vecPatches.Bs0, vecPatches );
    }
    else if ( vecPatches.listBx_[0]->dims_.size()==2 ) {
        if ( !params.full_B_exchange) {
            SyncVectorPatch::finalize_exchange_all_components_along_X( vecPatches.Bs0, vecPatches );
            SyncVectorPatch::finalize_exchange_all_components_along_Y( vecPatches.Bs1, vecPatches );
        }
    }
    else if ( vecPatches.listBx_[0]->dims_.size()==3 ) {
        if ( !params.full_B_exchange) {
            SyncVectorPatch::finalize_exchange_all_components_along_X( vecPatches.Bs0, vecPatches );
            SyncVectorPatch::finalize_exchange_all_components_along_Y( vecPatches.Bs1, vecPatches );
            SyncVectorPatch::finalize_exchange_all_components_along_Z( vecPatches.Bs2, vecPatches );
        }
    }

}

void SyncVectorPatch::exchangeJ( Params& params, VectorPatch& vecPatches )
{

    SyncVectorPatch::exchange_along_all_directions( vecPatches.listJx_, vecPatches );
    SyncVectorPatch::exchange_along_all_directions( vecPatches.listJy_, vecPatches );
    SyncVectorPatch::exchange_along_all_directions( vecPatches.listJz_, vecPatches );
}

void SyncVectorPatch::finalizeexchangeJ( Params& params, VectorPatch& vecPatches )
{

    SyncVectorPatch::finalize_exchange_along_all_directions( vecPatches.listJx_, vecPatches );
    SyncVectorPatch::finalize_exchange_along_all_directions( vecPatches.listJy_, vecPatches );
    SyncVectorPatch::finalize_exchange_along_all_directions( vecPatches.listJz_, vecPatches );
}


void SyncVectorPatch::exchange_along_all_directions( std::vector<Field*> fields, VectorPatch& vecPatches )
{
    for ( unsigned int iDim=0 ; iDim<fields[0]->dims_.size() ; iDim++ ) {
        #pragma omp for schedule(static)
        for (unsigned int ipatch=0 ; ipatch<fields.size() ; ipatch++)
            vecPatches(ipatch)->initExchange( fields[ipatch], iDim );
    } // End for iDim


    unsigned int nx_, ny_(1), nz_(1), h0, oversize[3], n_space[3], gsp[3];
    double *pt1,*pt2;
    h0 = vecPatches(0)->hindex;

    oversize[0] = vecPatches(0)->EMfields->oversize[0];
    oversize[1] = vecPatches(0)->EMfields->oversize[1];
    oversize[2] = vecPatches(0)->EMfields->oversize[2];

    n_space[0] = vecPatches(0)->EMfields->n_space[0];
    n_space[1] = vecPatches(0)->EMfields->n_space[1];
    n_space[2] = vecPatches(0)->EMfields->n_space[2];

    nx_ = fields[0]->dims_[0];
    if (fields[0]->dims_.size()>1) {
        ny_ = fields[0]->dims_[1];
        if (fields[0]->dims_.size()>2)
            nz_ = fields[0]->dims_[2];
    }


    gsp[0] = ( oversize[0] + 1 + fields[0]->isDual_[0] ); //Ghost size primal

    #pragma omp for schedule(static) private(pt1,pt2)
    for (unsigned int ipatch=0 ; ipatch<fields.size() ; ipatch++) {

        if (vecPatches(ipatch)->MPI_me_ == vecPatches(ipatch)->MPI_neighbor_[0][0]){
            pt1 = &(*fields[vecPatches(ipatch)->neighbor_[0][0]-h0])((n_space[0])*ny_*nz_);
            pt2 = &(*fields[ipatch])(0);
            memcpy( pt2, pt1, oversize[0]*ny_*nz_*sizeof(double));
            memcpy( pt1+gsp[0]*ny_*nz_, pt2+gsp[0]*ny_*nz_, oversize[0]*ny_*nz_*sizeof(double));
        } // End if ( MPI_me_ == MPI_neighbor_[0][0] )

        if (fields[0]->dims_.size()>1) {
            gsp[1] = ( oversize[1] + 1 + fields[0]->isDual_[1] ); //Ghost size primal
            if (vecPatches(ipatch)->MPI_me_ == vecPatches(ipatch)->MPI_neighbor_[1][0]){
                pt1 = &(*fields[vecPatches(ipatch)->neighbor_[1][0]-h0])(n_space[1]*nz_);
                pt2 = &(*fields[ipatch])(0);
                for (unsigned int i = 0 ; i < nx_*ny_*nz_ ; i += ny_*nz_){
                    for (unsigned int j = 0 ; j < oversize[1]*nz_ ; j++ ){
                        // Rewrite with memcpy ?
                        pt2[i+j] = pt1[i+j] ;
                        pt1[i+j+gsp[1]*nz_] = pt2[i+j+gsp[1]*nz_] ;
                    }
                }
            } // End if ( MPI_me_ == MPI_neighbor_[1][0] )

            if (fields[0]->dims_.size()>2) {
                gsp[2] = ( oversize[2] + 1 + fields[0]->isDual_[2] ); //Ghost size primal
                if (vecPatches(ipatch)->MPI_me_ == vecPatches(ipatch)->MPI_neighbor_[2][0]){
                    pt1 = &(*fields[vecPatches(ipatch)->neighbor_[2][0]-h0])(n_space[2]);
                    pt2 = &(*fields[ipatch])(0);
                    for (unsigned int i = 0 ; i < nx_*ny_*nz_ ; i += ny_*nz_){
                        for (unsigned int j = 0 ; j < ny_*nz_ ; j += nz_){
                            for (unsigned int k = 0 ; k < oversize[2] ; k++ ){
                                pt2[i+j+k] = pt1[i+j+k] ;
                                pt1[i+j+k+gsp[2]] = pt2[i+j+k+gsp[2]] ;
                            }
                        }
                    }
                }// End if ( MPI_me_ == MPI_neighbor_[2][0] )
            }// End if dims_.size()>2
        } // End if dims_.size()>1
    } // End for( ipatch )

}


//Proceed to the synchronization of field including corner ghost cells.
//This is done by exchanging one dimension at a time
void SyncVectorPatch::exchange_synchronized_per_direction( std::vector<Field*> fields, VectorPatch& vecPatches )
{

    unsigned int nx_, ny_(1), nz_(1), h0, oversize[3], n_space[3], gsp[3];
    double *pt1,*pt2;
    h0 = vecPatches(0)->hindex;

    oversize[0] = vecPatches(0)->EMfields->oversize[0];
    oversize[1] = vecPatches(0)->EMfields->oversize[1];
    oversize[2] = vecPatches(0)->EMfields->oversize[2];

    n_space[0] = vecPatches(0)->EMfields->n_space[0];
    n_space[1] = vecPatches(0)->EMfields->n_space[1];
    n_space[2] = vecPatches(0)->EMfields->n_space[2];

    nx_ = fields[0]->dims_[0];
    if (fields[0]->dims_.size()>1) {
        ny_ = fields[0]->dims_[1];
        if (fields[0]->dims_.size()>2)
            nz_ = fields[0]->dims_[2];
    }

    if (fields[0]->dims_.size()>2) {

// Dimension 2
    #pragma omp for schedule(static)
    for (unsigned int ipatch=0 ; ipatch<fields.size() ; ipatch++)
        vecPatches(ipatch)->initExchange( fields[ipatch], 2 );

    #pragma omp for schedule(static)
    for (unsigned int ipatch=0 ; ipatch<fields.size() ; ipatch++)
        vecPatches(ipatch)->finalizeExchange( fields[ipatch], 2 );

    #pragma omp for schedule(static) private(pt1,pt2)
    for (unsigned int ipatch=0 ; ipatch<fields.size() ; ipatch++) {

        gsp[2] = ( oversize[2] + 1 + fields[0]->isDual_[2] ); //Ghost size primal
        if (vecPatches(ipatch)->MPI_me_ == vecPatches(ipatch)->MPI_neighbor_[2][0]){
            pt1 = &(*fields[vecPatches(ipatch)->neighbor_[2][0]-h0])(n_space[2]);
            pt2 = &(*fields[ipatch])(0);
            //for (unsigned int in = oversize[0] ; in < nx_-oversize[0]; in ++){
            for (unsigned int in = 0 ; in < nx_ ; in ++){
                unsigned int i = in * ny_*nz_;
                //for (unsigned int jn = oversize[1] ; jn < ny_-oversize[1] ; jn ++){
                for (unsigned int jn = 0 ; jn < ny_ ; jn ++){
                    unsigned int j = jn *nz_;
                    for (unsigned int k = 0 ; k < oversize[2] ; k++ ){
                        pt2[i+j+k] = pt1[i+j+k] ;
                        pt1[i+j+k+gsp[2]] = pt2[i+j+k+gsp[2]] ;
                    }
                }
            }
        }// End if ( MPI_me_ == MPI_neighbor_[2][0] )

    } // End for( ipatch )
    }

// Dimension 1
    #pragma omp for schedule(static)
    for (unsigned int ipatch=0 ; ipatch<fields.size() ; ipatch++)
        vecPatches(ipatch)->initExchange( fields[ipatch], 1 );

    #pragma omp for schedule(static)
    for (unsigned int ipatch=0 ; ipatch<fields.size() ; ipatch++)
        vecPatches(ipatch)->finalizeExchange( fields[ipatch], 1 );

    #pragma omp for schedule(static) private(pt1,pt2)
    for (unsigned int ipatch=0 ; ipatch<fields.size() ; ipatch++) {

        gsp[1] = ( oversize[1] + 1 + fields[0]->isDual_[1] ); //Ghost size primal
        if (vecPatches(ipatch)->MPI_me_ == vecPatches(ipatch)->MPI_neighbor_[1][0]){
            pt1 = &(*fields[vecPatches(ipatch)->neighbor_[1][0]-h0])(n_space[1]*nz_);
            pt2 = &(*fields[ipatch])(0);
            for (unsigned int in = 0 ; in < nx_ ; in ++){
            //for (unsigned int in = oversize[0] ; in < nx_-oversize[0] ; in ++){ // <== This doesn't work. Why ??
                unsigned int i = in * ny_*nz_;
                for (unsigned int j = 0 ; j < oversize[1]*nz_ ; j++ ){
                    // Rewrite with memcpy ?
                    pt2[i+j] = pt1[i+j] ;
                    pt1[i+j+gsp[1]*nz_] = pt2[i+j+gsp[1]*nz_] ;
                }
            }
        } // End if ( MPI_me_ == MPI_neighbor_[1][0] )

    } // End for( ipatch )

// Dimension 0
    #pragma omp for schedule(static)
    for (unsigned int ipatch=0 ; ipatch<fields.size() ; ipatch++)
        vecPatches(ipatch)->initExchange( fields[ipatch], 0 );

    #pragma omp for schedule(static)
    for (unsigned int ipatch=0 ; ipatch<fields.size() ; ipatch++)
        vecPatches(ipatch)->finalizeExchange( fields[ipatch], 0 );



    gsp[0] = ( oversize[0] + 1 + fields[0]->isDual_[0] ); //Ghost size primal

    #pragma omp for schedule(static) private(pt1,pt2)
    for (unsigned int ipatch=0 ; ipatch<fields.size() ; ipatch++) {

        if (vecPatches(ipatch)->MPI_me_ == vecPatches(ipatch)->MPI_neighbor_[0][0]){
            pt1 = &(*fields[vecPatches(ipatch)->neighbor_[0][0]-h0])((n_space[0])*ny_*nz_);
            pt2 = &(*fields[ipatch])(0);
            memcpy( pt2, pt1, oversize[0]*ny_*nz_*sizeof(double));
            memcpy( pt1+gsp[0]*ny_*nz_, pt2+gsp[0]*ny_*nz_, oversize[0]*ny_*nz_*sizeof(double));
        } // End if ( MPI_me_ == MPI_neighbor_[0][0] )

    } // End for( ipatch )

}


void SyncVectorPatch::finalize_exchange_along_all_directions( std::vector<Field*> fields, VectorPatch& vecPatches )
{
    for ( unsigned int iDim=0 ; iDim<fields[0]->dims_.size() ; iDim++ ) {
        #pragma omp for schedule(static)
        for (unsigned int ipatch=0 ; ipatch<fields.size() ; ipatch++)
            vecPatches(ipatch)->finalizeExchange( fields[ipatch], iDim );
    } // End for iDim

}



void SyncVectorPatch::exchange_all_components_along_X( std::vector<Field*>& fields, VectorPatch& vecPatches )
{
    unsigned int nMPIx = vecPatches.MPIxIdx.size();
    #pragma omp for schedule(static)
    for (unsigned int ifield=0 ; ifield<nMPIx ; ifield++) {
        unsigned int ipatch = vecPatches.MPIxIdx[ifield];
        vecPatches(ipatch)->initExchange( vecPatches.B_MPIx[ifield      ], 0 ); // By
        vecPatches(ipatch)->initExchange( vecPatches.B_MPIx[ifield+nMPIx], 0 ); // Bz
    }


    unsigned int h0, oversize, n_space;
    double *pt1,*pt2;
    h0 = vecPatches(0)->hindex;

    oversize = vecPatches(0)->EMfields->oversize[0];

    n_space = vecPatches(0)->EMfields->n_space[0];

    int nPatches( vecPatches.size() );
    int nDim = vecPatches(0)->EMfields->Bx_->dims_.size();

    int nFieldLocalx = vecPatches.B_localx.size()/2;
    for ( int icomp=0 ; icomp<2 ; icomp++ ) {
        if (nFieldLocalx==0) continue;

        unsigned int ny_(1), nz_(1), gsp;
        if (nDim>1) {
            ny_ = vecPatches.B_localx[icomp*nFieldLocalx]->dims_[1];
            if (nDim>2)
                nz_ = vecPatches.B_localx[icomp*nFieldLocalx]->dims_[2];
        }
        gsp = ( oversize + 1 + vecPatches.B_localx[icomp*nFieldLocalx]->isDual_[0] ); //Ghost size primal

        unsigned int istart =  icomp   *nFieldLocalx;
        unsigned int iend    = (icomp+1)*nFieldLocalx;
        #pragma omp for schedule(static) private(pt1,pt2)
        for (unsigned int ifield=istart ; ifield<iend ; ifield++) {
            int ipatch = vecPatches.LocalxIdx[ ifield-icomp*nFieldLocalx ];

            if (vecPatches(ipatch)->MPI_me_ == vecPatches(ipatch)->MPI_neighbor_[0][0]){
                pt1 = &(fields[vecPatches(ipatch)->neighbor_[0][0]-h0+icomp*nPatches]->data_[n_space*ny_*nz_]);
                pt2 = &(vecPatches.B_localx[ifield]->data_[0]);
                //for filter
                memcpy( pt2, pt1, oversize*ny_*nz_*sizeof(double));
                memcpy( pt1+gsp*ny_*nz_, pt2+gsp*ny_*nz_, oversize*ny_*nz_*sizeof(double));
            } // End if ( MPI_me_ == MPI_neighbor_[0][0] )

        } // End for( ipatch )
    }

}

void SyncVectorPatch::finalize_exchange_all_components_along_X( std::vector<Field*>& fields, VectorPatch& vecPatches )
{
    unsigned int nMPIx = vecPatches.MPIxIdx.size();
    #pragma omp for schedule(static)
    for (unsigned int ifield=0 ; ifield<nMPIx ; ifield++) {
        unsigned int ipatch = vecPatches.MPIxIdx[ifield];
        vecPatches(ipatch)->finalizeExchange( vecPatches.B_MPIx[ifield      ], 0 ); // By
        vecPatches(ipatch)->finalizeExchange( vecPatches.B_MPIx[ifield+nMPIx], 0 ); // Bz
    }
}

void SyncVectorPatch::exchange_all_components_along_Y( std::vector<Field*>& fields, VectorPatch& vecPatches )
{
    unsigned int nMPIy = vecPatches.MPIyIdx.size();
    #pragma omp for schedule(static)
    for (unsigned int ifield=0 ; ifield<nMPIy ; ifield++) {
        unsigned int ipatch = vecPatches.MPIyIdx[ifield];
        vecPatches(ipatch)->initExchange( vecPatches.B1_MPIy[ifield], 1 );   // Bx
        vecPatches(ipatch)->initExchange( vecPatches.B1_MPIy[ifield+nMPIy], 1 ); // Bz
    }

    unsigned int h0, oversize, n_space;
    double *pt1,*pt2;
    h0 = vecPatches(0)->hindex;

    oversize = vecPatches(0)->EMfields->oversize[1];
    n_space = vecPatches(0)->EMfields->n_space[1];

    int nPatches( vecPatches.size() );
    int nDim = vecPatches(0)->EMfields->Bx_->dims_.size();

    int nFieldLocaly = vecPatches.B1_localy.size()/2;
    for ( int icomp=0 ; icomp<2 ; icomp++ ) {
        if (nFieldLocaly==0) continue;

        unsigned int nx_, ny_, nz_(1), gsp;
        nx_ = vecPatches.B1_localy[icomp*nFieldLocaly]->dims_[0];
        ny_ = vecPatches.B1_localy[icomp*nFieldLocaly]->dims_[1];
        if (nDim>2)
            nz_ = vecPatches.B1_localy[icomp*nFieldLocaly]->dims_[2];
        //for filter
        gsp = ( oversize + 1 + vecPatches.B1_localy[icomp*nFieldLocaly]->isDual_[1] ); //Ghost size primal

        unsigned int istart =  icomp   *nFieldLocaly;
        unsigned int iend    = (icomp+1)*nFieldLocaly;
        #pragma omp for schedule(static) private(pt1,pt2)
        for (unsigned int ifield=istart ; ifield<iend ; ifield++) {

            int ipatch = vecPatches.LocalyIdx[ ifield-icomp*nFieldLocaly ];
            if (vecPatches(ipatch)->MPI_me_ == vecPatches(ipatch)->MPI_neighbor_[1][0]){
                pt1 = &(fields[vecPatches(ipatch)->neighbor_[1][0]-h0+icomp*nPatches]->data_[n_space*nz_]);
                pt2 = &(vecPatches.B1_localy[ifield]->data_[0]);
                for (unsigned int i = 0 ; i < nx_*ny_*nz_ ; i += ny_*nz_){
                    // for filter
                    for (unsigned int j = 0 ; j < oversize*nz_ ; j++ ){
                        pt2[i+j] = pt1[i+j] ;
                        pt1[i+j+gsp*nz_] = pt2[i+j+gsp*nz_] ;
                    } // mempy to do
                }
            } // End if ( MPI_me_ == MPI_neighbor_[1][0] )

        } // End for( ipatch )
    }
}


void SyncVectorPatch::finalize_exchange_all_components_along_Y( std::vector<Field*>& fields, VectorPatch& vecPatches )
{
    unsigned int nMPIy = vecPatches.MPIyIdx.size();
    #pragma omp for schedule(static)
    for (unsigned int ifield=0 ; ifield<nMPIy ; ifield++) {
        unsigned int ipatch = vecPatches.MPIyIdx[ifield];
        vecPatches(ipatch)->finalizeExchange( vecPatches.B1_MPIy[ifield      ], 1 ); // By
        vecPatches(ipatch)->finalizeExchange( vecPatches.B1_MPIy[ifield+nMPIy], 1 ); // Bz
    }


}

void SyncVectorPatch::exchange_all_components_along_Z( std::vector<Field*> fields, VectorPatch& vecPatches )
{
    unsigned int nMPIz = vecPatches.MPIzIdx.size();
    #pragma omp for schedule(static)
    for (unsigned int ifield=0 ; ifield<nMPIz ; ifield++) {
        unsigned int ipatch = vecPatches.MPIzIdx[ifield];
        vecPatches(ipatch)->initExchange( vecPatches.B2_MPIz[ifield],       2 ); // Bx
        vecPatches(ipatch)->initExchange( vecPatches.B2_MPIz[ifield+nMPIz], 2 ); // By
    }

    unsigned int h0, oversize, n_space;
    double *pt1,*pt2;
    h0 = vecPatches(0)->hindex;

    oversize = vecPatches(0)->EMfields->oversize[2];
    n_space = vecPatches(0)->EMfields->n_space[2];

    int nPatches( vecPatches.size() );

    int nFieldLocalz = vecPatches.B2_localz.size()/2;
    for ( int icomp=0 ; icomp<2 ; icomp++ ) {
        if (nFieldLocalz==0) continue;

        unsigned int nx_, ny_, nz_, gsp;
        nx_ = vecPatches.B2_localz[icomp*nFieldLocalz]->dims_[0];
        ny_ = vecPatches.B2_localz[icomp*nFieldLocalz]->dims_[1];
        nz_ = vecPatches.B2_localz[icomp*nFieldLocalz]->dims_[2];
        //for filter
        gsp = ( oversize + 1 + vecPatches.B2_localz[icomp*nFieldLocalz]->isDual_[2] ); //Ghost size primal

        unsigned int istart  =  icomp   *nFieldLocalz;
        unsigned int iend    = (icomp+1)*nFieldLocalz;
        #pragma omp for schedule(static) private(pt1,pt2)
        for (unsigned int ifield=istart ; ifield<iend ; ifield++) {

            int ipatch = vecPatches.LocalzIdx[ ifield-icomp*nFieldLocalz ];
            if (vecPatches(ipatch)->MPI_me_ == vecPatches(ipatch)->MPI_neighbor_[2][0]){
                pt1 = &(fields[vecPatches(ipatch)->neighbor_[2][0]-h0+icomp*nPatches]->data_[n_space]);
                pt2 = &(vecPatches.B2_localz[ifield]->data_[0]);
                for (unsigned int i = 0 ; i < nx_*ny_*nz_ ; i += ny_*nz_){
                    for (unsigned int j = 0 ; j < ny_*nz_ ; j += nz_){
                        for (unsigned int k = 0 ; k < oversize ; k++ ){
                            pt2[i+j+k] = pt1[i+j+k] ;
                            pt1[i+j+k+gsp] = pt2[i+j+k+gsp] ;
                        }
                    }
                }
            } // End if ( MPI_me_ == MPI_neighbor_[2][0] )

        } // End for( ipatch )
    }
}

void SyncVectorPatch::finalize_exchange_all_components_along_Z( std::vector<Field*> fields, VectorPatch& vecPatches )
{
    unsigned int nMPIz = vecPatches.MPIzIdx.size();
    #pragma omp for schedule(static)
    for (unsigned int ifield=0 ; ifield<nMPIz ; ifield++) {
        unsigned int ipatch = vecPatches.MPIzIdx[ifield];
        vecPatches(ipatch)->finalizeExchange( vecPatches.B2_MPIz[ifield      ], 2 ); // Bx
        vecPatches(ipatch)->finalizeExchange( vecPatches.B2_MPIz[ifield+nMPIz], 2 ); // By
    }

}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
// -------------------------------------------  DEPRECATED FIELD FUNCTIONS  --------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------

void SyncVectorPatch::exchange_along_X( std::vector<Field*> fields, VectorPatch& vecPatches )
{
    #pragma omp for schedule(static)
    for (unsigned int ipatch=0 ; ipatch<fields.size() ; ipatch++)
        vecPatches(ipatch)->initExchange( fields[ipatch], 0 );

    unsigned int ny_(1), nz_(1), h0, oversize, n_space, gsp;
    double *pt1,*pt2;
    h0 = vecPatches(0)->hindex;

    oversize = vecPatches(0)->EMfields->oversize[0];

    n_space = vecPatches(0)->EMfields->n_space[0];

    if (fields[0]->dims_.size()>1) {
        ny_ = fields[0]->dims_[1];
        if (fields[0]->dims_.size()>2)
            nz_ = fields[0]->dims_[2];
    }

    //gsp[0] = 2*oversize[0]+fields[0]->isDual_[0]; //Ghost size primal
    //for filter
    gsp = ( oversize + 1 + fields[0]->isDual_[0] ); //Ghost size primal

    #pragma omp for schedule(static) private(pt1,pt2)
    for (unsigned int ipatch=0 ; ipatch<fields.size() ; ipatch++) {

        if (vecPatches(ipatch)->MPI_me_ == vecPatches(ipatch)->MPI_neighbor_[0][0]){
            pt1 = &(*fields[vecPatches(ipatch)->neighbor_[0][0]-h0])(n_space*ny_*nz_);
            pt2 = &(*fields[ipatch])(0);
            //memcpy( pt2, pt1, ny_*sizeof(double));
            //memcpy( pt1+gsp[0]*ny_, pt2+gsp[0]*ny_, ny_*sizeof(double));
            //for filter
            memcpy( pt2, pt1, oversize*ny_*nz_*sizeof(double));
            memcpy( pt1+gsp*ny_*nz_, pt2+gsp*ny_*nz_, oversize*ny_*nz_*sizeof(double));
        } // End if ( MPI_me_ == MPI_neighbor_[0][0] )


    } // End for( ipatch )

}

void SyncVectorPatch::finalize_exchange_along_X( std::vector<Field*> fields, VectorPatch& vecPatches )
{
    #pragma omp for schedule(static)
    for (unsigned int ipatch=0 ; ipatch<fields.size() ; ipatch++)
        vecPatches(ipatch)->finalizeExchange( fields[ipatch], 0 );

}

void SyncVectorPatch::exchange_along_Y( std::vector<Field*> fields, VectorPatch& vecPatches )
{
    #pragma omp for schedule(static)
    for (unsigned int ipatch=0 ; ipatch<fields.size() ; ipatch++)
        vecPatches(ipatch)->initExchange( fields[ipatch], 1 );

    unsigned int nx_, ny_, nz_(1), h0, oversize, n_space, gsp;
    double *pt1,*pt2;
    h0 = vecPatches(0)->hindex;

    oversize = vecPatches(0)->EMfields->oversize[1];
    n_space = vecPatches(0)->EMfields->n_space[1];

    nx_ = fields[0]->dims_[0];
    ny_ = fields[0]->dims_[1];
    if (fields[0]->dims_.size()>2)
        nz_ = fields[0]->dims_[2];

    //gsp = 2*oversize[1]+fields[0]->isDual_[1]; //Ghost size primal
    //for filter
    gsp = ( oversize + 1 + fields[0]->isDual_[1] ); //Ghost size primal

    #pragma omp for schedule(static) private(pt1,pt2)
    for (unsigned int ipatch=0 ; ipatch<fields.size() ; ipatch++) {

        if (vecPatches(ipatch)->MPI_me_ == vecPatches(ipatch)->MPI_neighbor_[1][0]){
            pt1 = &(*fields[vecPatches(ipatch)->neighbor_[1][0]-h0])(n_space*nz_);
            pt2 = &(*fields[ipatch])(0);
            for (unsigned int i = 0 ; i < nx_*ny_*nz_ ; i += ny_*nz_){
                // for filter
                for (unsigned int j = 0 ; j < oversize*nz_ ; j++ ){
                    pt2[i+j] = pt1[i+j] ;
                    pt1[i+j+gsp*nz_] = pt2[i+j+gsp*nz_] ;
                } // mempy to do
            }
        } // End if ( MPI_me_ == MPI_neighbor_[1][0] )

    } // End for( ipatch )

}

void SyncVectorPatch::finalize_exchange_along_Y( std::vector<Field*> fields, VectorPatch& vecPatches )
{
    #pragma omp for schedule(static)
    for (unsigned int ipatch=0 ; ipatch<fields.size() ; ipatch++)
        vecPatches(ipatch)->finalizeExchange( fields[ipatch], 1 );

}

void SyncVectorPatch::exchange_along_Z( std::vector<Field*> fields, VectorPatch& vecPatches )
{
    #pragma omp for schedule(static)
    for (unsigned int ipatch=0 ; ipatch<fields.size() ; ipatch++)
        vecPatches(ipatch)->initExchange( fields[ipatch], 2 );

    unsigned int nx_, ny_, nz_, h0, oversize, n_space, gsp;
    double *pt1,*pt2;
    h0 = vecPatches(0)->hindex;

    oversize = vecPatches(0)->EMfields->oversize[2];
    n_space = vecPatches(0)->EMfields->n_space[2];

    nx_ = fields[0]->dims_[0];
    ny_ = fields[0]->dims_[1];
    nz_ = fields[0]->dims_[2];

    //gsp = 2*oversize[1]+fields[0]->isDual_[1]; //Ghost size primal
    //for filter
    gsp = ( oversize + 1 + fields[0]->isDual_[2] ); //Ghost size primal

    #pragma omp for schedule(static) private(pt1,pt2)
    for (unsigned int ipatch=0 ; ipatch<fields.size() ; ipatch++) {

        if (vecPatches(ipatch)->MPI_me_ == vecPatches(ipatch)->MPI_neighbor_[2][0]){
           pt1 = &(*fields[vecPatches(ipatch)->neighbor_[2][0]-h0])(n_space);
           pt2 = &(*fields[ipatch])(0);
           for (unsigned int i = 0 ; i < nx_*ny_*nz_ ; i += ny_*nz_){
               for (unsigned int j = 0 ; j < ny_*nz_ ; j += nz_){
                   for (unsigned int k = 0 ; k < oversize ; k++ ){
                       pt2[i+j+k] = pt1[i+j+k] ;
                       pt1[i+j+k+gsp] = pt2[i+j+k+gsp] ;
                   }
               }
           }
        } // End if ( MPI_me_ == MPI_neighbor_[2][0] )

    } // End for( ipatch )
}

void SyncVectorPatch::finalize_exchange_along_Z( std::vector<Field*> fields, VectorPatch& vecPatches )
{
    #pragma omp for schedule(static)
    for (unsigned int ipatch=0 ; ipatch<fields.size() ; ipatch++)
        vecPatches(ipatch)->finalizeExchange( fields[ipatch], 2 );

}
