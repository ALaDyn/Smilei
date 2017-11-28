#include "ElectroMagnBCRZ_Axis.h"

#include <cstdlib>

#include <iostream>
#include <string>

#include "Params.h"
#include "Patch.h"
#include "ElectroMagn.h"
#include "cField2D.h"
#include "Tools.h"
#include <complex>
#include "dcomplex.h"
using namespace std;

ElectroMagnBCRZ_Axis::ElectroMagnBCRZ_Axis( Params &params, Patch* patch, unsigned int _min_max )
: ElectroMagnBC( params, patch, _min_max )
{
    // conversion factor from degree to radian
    conv_deg2rad = M_PI/180.0;
    
    // number of nodes of the primal and dual grid in the x-direction
    nl_p = params.n_space[0]+1+2*params.oversize[0];
    nl_d = nl_p+1;
    // number of nodes of the primal and dual grid in the y-direction
    nr_p = params.n_space[1]+1+2*params.oversize[1];
    nr_d = nr_p+1;
    
    // spatial-step and ratios time-step by spatial-step & spatial-step by time-step (in the x-direction)
    dl       = params.cell_length[0];
    dt_ov_dl = dt/dl;
    dl_ov_dt = 1.0/dt_ov_dl;
    
    // spatial-step and ratios time-step by spatial-step & spatial-step by time-step (in the y-direction)
    dr       = params.cell_length[1];
    dt_ov_dr = dt/dr;
    dr_ov_dt = 1.0/dt_ov_dr;
    
    if (min_max == 2 && patch->isYmin() ) {
    }
    
    #ifdef _TODO_RZ
    #endif
}


void ElectroMagnBCRZ_Axis::save_fields(Field* my_field, Patch* patch)
{
    ERROR( "Impossible" );
}

void ElectroMagnBCRZ_Axis::disableExternalFields()
{
    ERROR( "Impossible" );
}


// ---------------------------------------------------------------------------------------------------------------------
// Apply Boundary Conditions
// ---------------------------------------------------------------------------------------------------------------------
void ElectroMagnBCRZ_Axis::apply(ElectroMagn* EMfields, double time_dual, Patch* patch)
{
    // Loop on imode 
    int imode = 0;

    // Static cast of the fields
    cField2D* ElRZ = (static_cast<ElectroMagn3DRZ*>(EMfields))->El_[imode];
    cField2D* ErRZ = (static_cast<ElectroMagn3DRZ*>(EMfields))->Er_[imode];
    cField2D* EtRZ = (static_cast<ElectroMagn3DRZ*>(EMfields))->Et_[imode];
    cField2D* BlRZ = (static_cast<ElectroMagn3DRZ*>(EMfields))->Bl_[imode];
    cField2D* BrRZ = (static_cast<ElectroMagn3DRZ*>(EMfields))->Br_[imode];
    cField2D* BtRZ = (static_cast<ElectroMagn3DRZ*>(EMfields))->Bt_[imode];
	cField2D* BlRZ_old = (static_cast<ElectroMagn3DRZ*>(EMfields))->Bl_m[imode];
    cField2D* BtRZ_old = (static_cast<ElectroMagn3DRZ*>(EMfields))->Bt_m[imode]; 
	cField2D* JlRZ = (static_cast<ElectroMagn3DRZ*>(EMfields))->Jl_[imode];
    cField2D* JrRZ = (static_cast<ElectroMagn3DRZ*>(EMfields))->Jr_[imode];
    cField2D* JtRZ = (static_cast<ElectroMagn3DRZ*>(EMfields))->Jt_[imode];

	if (min_max == 2 && patch->isYmin()){
		if (imode==0){
			//MF_Solver_Yee
			for (unsigned int i=0 ; i<nl_d ; i++) {
				(*BrRZ)(i,0)=0;
			}
			for (unsigned int i=0 ; i<nl_d ; i++) {
				(*BtRZ)(i,0)= -(*BtRZ)(i,1);
			}
			for (unsigned int i=0 ; i<nl_p ; i++) {
				(*BlRZ)(i,0)+= -(*BlRZ)(i,1)+(*BlRZ_old)(i,1)-4*dt_ov_dr*(*EtRZ)(i,1);
			}
			//MA_SolverRZ_norm
			for (unsigned int i=0 ; i<nl_p ; i++) {
				(*EtRZ)(i,0)=0;
			}
			for (unsigned int i=0 ; i<nl_p ; i++) {
				(*ErRZ)(i,0)= -(*ErRZ)(i,1);
			}
			for (unsigned int i=0 ; i<nl_d ; i++) {
				(*ElRZ)(i,0)+= 4*dt_ov_dr*(*BtRZ)(i,1)-dt*(*JlRZ)(i,0);
			}
		}
		if (imode==1){
			//MF
			for (unsigned int i=0 ; i<nl_d ; i++) {
				(*BlRZ)(i,0)= -(*BlRZ)(i,1);
			}
			for (unsigned int i=0 ; i<nl_d ; i++) {
				(*BrRZ)(i,0)+= (*BrRZ)(i,1)+ Icpx*dt_ov_dr*(*ElRZ)(0,i)
				+			dt_ov_dl*((*EtRZ)(i,0)-(*EtRZ)(i+1,0));
			}
			for (unsigned int i=0 ; i<nl_p ; i++) {
				(*BtRZ)(i,0)+= -dt_ov_dl*((*ErRZ)(i+1,0)-(*ErRZ)(i,0)+(*ErRZ)(i+1,1)-(*ErRZ)(i,1))
				+				2*dt_ov_dr*(*ElRZ)(i+1,1) - (*BtRZ_old)(i,1)+ (*BtRZ)(i,1);
			}	
			//MA
			for (unsigned int i=0 ; i<nl_d ; i++) {
				(*ElRZ)(i,0)= 0;
			}
			for (unsigned int i=0 ; i<nl_p ; i++) {
				(*ErRZ)(i,0)= (*ErRZ)(i,1);
			}
			for (unsigned int i=0 ; i<nl_d ; i++) {
				(*EtRZ)(i,0)= (*EtRZ)(i,1);
			}	

		}

	}    
}
