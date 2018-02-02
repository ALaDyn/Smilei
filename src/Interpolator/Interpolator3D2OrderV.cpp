#include "Interpolator3D2OrderV.h"

#include <cmath>
#include <iostream>

#include "ElectroMagn.h"
#include "Field3D.h"
#include "Particles.h"

using namespace std;


// ---------------------------------------------------------------------------------------------------------------------
// Creator for Interpolator3D2OrderV
// ---------------------------------------------------------------------------------------------------------------------
Interpolator3D2OrderV::Interpolator3D2OrderV(Params &params, Patch *patch) : Interpolator3D(params, patch)
{

    dx_inv_ = 1.0/params.cell_length[0];
    dy_inv_ = 1.0/params.cell_length[1];
    dz_inv_ = 1.0/params.cell_length[2];
    D_inv[0] = 1.0/params.cell_length[0];
    D_inv[1] = 1.0/params.cell_length[1];
    D_inv[2] = 1.0/params.cell_length[2];

}

// ---------------------------------------------------------------------------------------------------------------------
// 2nd OrderV Interpolation of the fields at a the particle position (3 nodes are used)
// ---------------------------------------------------------------------------------------------------------------------
void Interpolator3D2OrderV::operator() (ElectroMagn* EMfields, Particles &particles, int ipart, double* ELoc, double* BLoc)
{
}

void Interpolator3D2OrderV::operator() (ElectroMagn* EMfields, Particles &particles, SmileiMPI* smpi, int *istart, int *iend, int ithread)
{
    if ( istart[0] == iend[0] ) return; //Don't treat empty cells.

    int nparts( particles.size() );

    double *Epart[3], *Bpart[3];
    double E,E2;

    double *deltaO[3];
    deltaO[0] = &(smpi->dynamics_deltaold[ithread][0]);
    deltaO[1] = &(smpi->dynamics_deltaold[ithread][nparts]);
    deltaO[2] = &(smpi->dynamics_deltaold[ithread][2*nparts]);

    for (unsigned int k=0; k<3;k++) {   
        Epart[k]= &(smpi->dynamics_Epart[ithread][k*nparts]);
        Bpart[k]= &(smpi->dynamics_Bpart[ithread][k*nparts]);
    }

    int idx[3], idxO[3];
    //Primal indices are constant over the all cell
    idx[0]  = round( particles.position(0,*istart) * D_inv[0] );
    idxO[0] = idx[0] - i_domain_begin -1 ;
    idx[1]  = round( particles.position(1,*istart) * D_inv[1] );
    idxO[1] = idx[1] - j_domain_begin -1 ;
    idx[2]  = round( particles.position(2,*istart) * D_inv[2] );
    idxO[2] = idx[2] - k_domain_begin -1 ;

    double ***Egrid[3], ***Bgrid[3];

    Egrid[0] = (static_cast<Field3D*>(EMfields->Ex_))->data_3D;
    Egrid[1] = (static_cast<Field3D*>(EMfields->Ey_))->data_3D;
    Egrid[2] = (static_cast<Field3D*>(EMfields->Ez_))->data_3D;
    Bgrid[0] = (static_cast<Field3D*>(EMfields->Bx_m))->data_3D;
    Bgrid[1] = (static_cast<Field3D*>(EMfields->By_m))->data_3D;
    Bgrid[2] = (static_cast<Field3D*>(EMfields->Bz_m))->data_3D;

    Field3D* Ex3D = static_cast<Field3D*>(EMfields->Ex_);
    Field3D* Ey3D = static_cast<Field3D*>(EMfields->Ey_);
    Field3D* Ez3D = static_cast<Field3D*>(EMfields->Ez_);
    Field3D* Bx3D = static_cast<Field3D*>(EMfields->Bx_);
    Field3D* By3D = static_cast<Field3D*>(EMfields->By_);
    Field3D* Bz3D = static_cast<Field3D*>(EMfields->Bz_);

    double coeff[3][2][3][32]; 
    int dual[3][32]; // Size ndim. Boolean indicating if the part has a dual indice equal to the primal one (dual=0) or if it is +1 (dual=1).

    int vecSize = 32;

    int cell_nparts( (int)iend[0]-(int)istart[0] );
    int nbVec = ( iend[0]-istart[0]+(cell_nparts-1)-((iend[0]-istart[0]-1)&(cell_nparts-1)) ) / vecSize;

    if (nbVec*vecSize != cell_nparts)
        nbVec++;

    for (int iivect=0 ; iivect<nbVec; iivect++ ){
        int ivect = vecSize*iivect;

        int np_computed(0);
        if (cell_nparts > vecSize ) {
            np_computed = vecSize;
            cell_nparts -= vecSize;
        }       
        else
            np_computed = cell_nparts;

        #pragma omp simd
        for (int ipart=0 ; ipart<np_computed; ipart++ ){

            double delta0, delta;
            double delta2;
            

            for (int i=0;i<3;i++) { // for X/Y
                delta0 = particles.position(i,ipart+ivect+istart[0])*D_inv[i];
                dual [i][ipart] = ( delta0 - (double)idx[i] >=0. );

                for (int j=0;j<2;j++) { // for dual

                    delta   = delta0 - (double)idx[i] + (double)j*(0.5-dual[i][ipart]);
                    delta2  = delta*delta;

                    coeff[i][j][0][ipart]    =  0.5 * (delta2-delta+0.25);
                    coeff[i][j][1][ipart]    =  (0.75 - delta2);
                    coeff[i][j][2][ipart]    =  0.5 * (delta2+delta+0.25);
    
                    if (j==0) deltaO[i][ipart+ivect+istart[0]] = delta;
                }
            }
        }

        #pragma omp simd
        for (int ipart=0 ; ipart<np_computed; ipart++ ){

            double* coeffyp = &(coeff[1][0][1][ipart]);
            double* coeffyd = &(coeff[1][1][1][ipart]);
            double* coeffxd = &(coeff[0][1][1][ipart]);
            double* coeffxp = &(coeff[0][0][1][ipart]);
            double* coeffzp = &(coeff[2][0][1][ipart]);
            double* coeffzd = &(coeff[2][1][1][ipart]);

            //Ex(dual, primal, primal)
            double interp_res = 0.;
            for (int iloc=-1 ; iloc<2 ; iloc++) {
                for (int jloc=-1 ; jloc<2 ; jloc++) {
                    for (int kloc=-1 ; kloc<2 ; kloc++) {
                        interp_res += *(coeffxd+iloc*32) * *(coeffyp+jloc*32) * *(coeffzp+kloc*32) *
                            ( (1-dual[0][ipart])*(*Ex3D)(idxO[0]+1+iloc,idxO[1]+1+jloc,idxO[2]+1+kloc) + dual[0][ipart]*(*Ex3D)(idxO[0]+2+iloc,idxO[1]+1+jloc,idxO[2]+1+kloc ) );
                    }
                }
            }
            Epart[0][ipart+ivect+istart[0]] = interp_res;


            //Ey(primal, dual, primal)
            interp_res = 0.;
            for (int iloc=-1 ; iloc<2 ; iloc++) {
                for (int jloc=-1 ; jloc<2 ; jloc++) {
                    for (int kloc=-1 ; kloc<2 ; kloc++) {
                        interp_res += *(coeffxp+iloc*32) * *(coeffyd+jloc*32) * *(coeffzp+kloc*32) *
                            ( (1-dual[1][ipart])*(*Ey3D)(idxO[0]+1+iloc,idxO[1]+1+jloc,idxO[2]+1+kloc) + dual[1][ipart]*(*Ey3D)(idxO[0]+1+iloc,idxO[1]+2+jloc,idxO[2]+1+kloc ) );
                    }
                }
            }
            Epart[1][ipart+ivect+istart[0]] = interp_res;


            //Ez(primal, primal, dual)
            interp_res = 0.;
            for (int iloc=-1 ; iloc<2 ; iloc++) {
                for (int jloc=-1 ; jloc<2 ; jloc++) {
                    for (int kloc=-1 ; kloc<2 ; kloc++) {
                        interp_res += *(coeffxp+iloc*32) * *(coeffyd+jloc*32) * *(coeffzp+kloc*32) *
                            ( (1-dual[2][ipart])*(*Ez3D)(idxO[0]+1+iloc,idxO[1]+1+jloc,idxO[2]+1+kloc) + dual[2][ipart]*(*Ez3D)(idxO[0]+1+iloc,idxO[1]+1+jloc,idxO[2]+2+kloc ) );
                    }
                }
            }
            Epart[2][ipart+ivect+istart[0]] = interp_res;


            //Bx(primal, dual , dual )
            interp_res = 0.;
            for (int iloc=-1 ; iloc<2 ; iloc++) {
                for (int jloc=-1 ; jloc<2 ; jloc++) {
                    for (int kloc=-1 ; kloc<2 ; kloc++) {
                        interp_res += *(coeffxp+iloc*32) * *(coeffyd+jloc*32) * *(coeffzp+kloc*32) * 
                            ( (1-dual[2][ipart]) * ( (1-dual[1][ipart])*(*Bx3D)(idxO[0]+1+iloc,idxO[1]+1+jloc,idxO[2]+1+kloc) + dual[1][ipart]*(*Bx3D)(idxO[0]+1+iloc,idxO[1]+2+jloc,idxO[2]+1+kloc ) )
                            +    dual[2][ipart]  * ( (1-dual[1][ipart])*(*Bx3D)(idxO[0]+1+iloc,idxO[1]+1+jloc,idxO[2]+2+kloc) + dual[1][ipart]*(*Bx3D)(idxO[0]+1+iloc,idxO[1]+2+jloc,idxO[2]+2+kloc ) ) );
                    }
                }
            }
            Bpart[0][ipart+ivect+istart[0]] = interp_res;

            //By(dual, primal, dual )
            interp_res = 0.;
            for (int iloc=-1 ; iloc<2 ; iloc++) {
                for (int jloc=-1 ; jloc<2 ; jloc++) {
                    for (int kloc=-1 ; kloc<2 ; kloc++) {
                        interp_res += *(coeffxp+iloc*32) * *(coeffyd+jloc*32) * *(coeffzp+kloc*32) * 
                            ( (1-dual[2][ipart]) * ( (1-dual[0][ipart])*(*By3D)(idxO[0]+1+iloc,idxO[1]+1+jloc,idxO[2]+1+kloc) + dual[0][ipart]*(*By3D)(idxO[0]+2+iloc,idxO[1]+1+jloc,idxO[2]+1+kloc ) )
                            +    dual[2][ipart]  * ( (1-dual[0][ipart])*(*By3D)(idxO[0]+1+iloc,idxO[1]+1+jloc,idxO[2]+2+kloc) + dual[0][ipart]*(*By3D)(idxO[0]+2+iloc,idxO[1]+1+jloc,idxO[2]+2+kloc ) ) );
                    }
                }
            }
            Bpart[1][ipart+ivect+istart[0]] = interp_res;

            //Bz(dual, dual, prim )
            interp_res = 0.;
            for (int iloc=-1 ; iloc<2 ; iloc++) {
                for (int jloc=-1 ; jloc<2 ; jloc++) {
                    for (int kloc=-1 ; kloc<2 ; kloc++) {
                        interp_res += *(coeffxp+iloc*32) * *(coeffyd+jloc*32) * *(coeffzp+kloc*32) * 
                            ( (1-dual[1][ipart]) * ( (1-dual[0][ipart])*(*Bz3D)(idxO[0]+1+iloc,idxO[1]+1+jloc,idxO[2]+1+kloc) + dual[0][ipart]*(*Bz3D)(idxO[0]+2+iloc,idxO[1]+1+jloc,idxO[2]+1+kloc ) )
                            +    dual[1][ipart]  * ( (1-dual[0][ipart])*(*Bz3D)(idxO[0]+1+iloc,idxO[1]+2+jloc,idxO[2]+1+kloc) + dual[0][ipart]*(*Bz3D)(idxO[0]+2+iloc,idxO[1]+2+jloc,idxO[2]+1+kloc ) ) );
                    }
                }
            }
            Bpart[2][ipart+ivect+istart[0]] = interp_res;

        }

    }
    //}

} // END Interpolator3D2OrderV

void Interpolator3D2OrderV::operator() (ElectroMagn* EMfields, Particles &particles, SmileiMPI* smpi, int *istart, int *iend, int ithread, LocalFields* JLoc, double* RhoLoc)
{
}
