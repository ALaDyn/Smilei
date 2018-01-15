#include "ElectroMagnBCRZ_SM.h"

#include <cstdlib>

#include <iostream>
#include <string>

#include "Params.h"
#include "Patch.h"
#include "ElectroMagn.h"
#include "cField2D.h"
#include "Tools.h"
#include "Laser.h"
#include <complex>
#include "dcomplex.h"

using namespace std;

ElectroMagnBCRZ_SM::ElectroMagnBCRZ_SM( Params &params, Patch* patch, unsigned int _min_max )
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
    
    //Number of modes
	Nmode= params.Nmode;

    if (min_max == 0 && patch->isXmin() ) {
        // BCs at the x-border min
        Bl_val.resize(nr_d,0.); // dual in the y-direction
        Br_val.resize(nr_p,0.); // primal in the y-direction
        Bt_val.resize(nr_d,0.); // dual in the y-direction
    }
    else if (min_max == 1 && patch->isXmax() ) {
        // BCs at the x-border max
        Bl_val.resize(nr_d,0.);
        Br_val.resize(nr_p,0.);
        Bt_val.resize(nr_d,0.);
    }
    else if (min_max == 2 && patch->isYmin() ) {
        // BCs in the y-border min
        Bl_val.resize(nl_p,0.); // primal in the x-direction
        Br_val.resize(nl_d,0.); // dual in the x-direction
        Bt_val.resize(nl_d,0.); // dual in the x-direction
    }
    else if (min_max == 3 && patch->isYmax() ) {
        // BCs in the y-border max
        Bl_val.resize(nl_p,0.);
        Br_val.resize(nl_d,0.);
        Bt_val.resize(nl_d,0.);
    }
    
    
    
    
    // -----------------------------------------------------
    // Parameters for the Silver-Mueller boundary conditions
    // -----------------------------------------------------

    #ifdef _TODO_RZ
	#endif
    // Xmin boundary
    double theta  = 0.0*conv_deg2rad; //0.0;
    double factor = 1.0 / (1.0 + dt_ov_dl);
    Alpha_SM_Xmin    = 2.0                     * factor;
    Beta_SM_Xmin     = - (1-dt_ov_dl) * factor;
    Gamma_SM_Xmin    = 4.0         * factor;
    Delta_SM_Xmin    = - dt_ov_dr * factor;
    Epsilon_SM_Xmin  = -Icpx / (1.0 + dt_ov_dl);
    // Xmax boundary
    theta         = M_PI;
    factor        = 1.0 / (1.0 + dt_ov_dl);
    Alpha_SM_Xmax    = 2.0                      * factor;
    Beta_SM_Xmax     = - (1.0 -dt_ov_dl)  * factor;
    Gamma_SM_Xmax    = 4.0         * factor;
    Delta_SM_Xmax    = - dt_ov_dr  * factor;
    Epsilon_SM_Xmax  = -Icpx / (1.0 + dt_ov_dl)  * factor;
    

    
}


void ElectroMagnBCRZ_SM::save_fields(Field* my_field, Patch* patch) {
    cField2D* field2D=static_cast<cField2D*>(my_field);
    
    if (min_max == 0 && patch->isXmin() ) {
        if (field2D->name=="Bl"){
            for (unsigned int j=0; j<nr_d; j++) {
                Bl_val[j]=(*field2D)(0,j);
            }
        }
        
        if (field2D->name=="Br"){
            for (unsigned int j=0; j<nr_p; j++) {
                Br_val[j]=(*field2D)(0,j);
            }
        }
        
        if (field2D->name=="Bt"){
            for (unsigned int j=0; j<nr_d; j++) {
                Bt_val[j]=(*field2D)(0,j);
            }
        }
    }
    else if (min_max == 1 && patch->isXmax() ) {
        if (field2D->name=="Bl"){
            for (unsigned int j=0; j<nr_d; j++) {
                Bl_val[j]=(*field2D)(nl_p-1,j);
            }
        }
        
        if (field2D->name=="Br"){
            for (unsigned int j=0; j<nr_p; j++) {
                Br_val[j]=(*field2D)(nl_d-1,j);
            }
        }
        
        if (field2D->name=="Bt"){
            for (unsigned int j=0; j<nr_d; j++) {
                Bt_val[j]=(*field2D)(nl_d-1,j);
            }
        }
    }
    else if (min_max == 2 && patch->isYmin() ) {
        if (field2D->name=="Bl"){
            for (unsigned int i=0; i<nl_p; i++) {
                Bl_val[i]=(*field2D)(i,0);
            }
        }
        
        if (field2D->name=="Br"){
            for (unsigned int i=0; i<nl_d; i++) {
                Br_val[i]=(*field2D)(i,0);
            }
        }
        
        if (field2D->name=="Bt"){
            for (unsigned int i=0; i<nl_d; i++) {
                Bt_val[i]=(*field2D)(i,0);
            }
        }
    }
    else if (min_max == 3 && patch->isYmax() ) {
        if (field2D->name=="Bl"){
            for (unsigned int i=0; i<nl_p; i++) {
                Bl_val[i]=(*field2D)(i,nr_d-1);
            }
        }
        
        if (field2D->name=="Br"){
            for (unsigned int i=0; i<nl_d; i++) {
                Br_val[i]=(*field2D)(i,nr_p-1);
            }
        }
        
        if (field2D->name=="Bt"){
            for (unsigned int i=0; i<nl_d; i++) {
                Bt_val[i]=(*field2D)(i,nr_d-1);
            }
        }
    }
}

void ElectroMagnBCRZ_SM::disableExternalFields()
{
    Bl_val.resize(0);
    Br_val.resize(0);
    Bt_val.resize(0);
}



// ---------------------------------------------------------------------------------------------------------------------
// Apply Boundary Conditions
// ---------------------------------------------------------------------------------------------------------------------
void ElectroMagnBCRZ_SM::apply(ElectroMagn* EMfields, double time_dual, Patch* patch)
{
    // Loop on imode 
    for (unsigned int imode=0 ; imode<Nmode ; imode++) {
		// Static cast of the fields
		cField2D* ElRZ = (static_cast<ElectroMagn3DRZ*>(EMfields))->El_[imode];
		cField2D* ErRZ = (static_cast<ElectroMagn3DRZ*>(EMfields))->Er_[imode];
		cField2D* EtRZ = (static_cast<ElectroMagn3DRZ*>(EMfields))->Et_[imode];
		cField2D* BlRZ = (static_cast<ElectroMagn3DRZ*>(EMfields))->Bl_[imode];
		cField2D* BrRZ = (static_cast<ElectroMagn3DRZ*>(EMfields))->Br_[imode];
		cField2D* BtRZ = (static_cast<ElectroMagn3DRZ*>(EMfields))->Bt_[imode];

		int     j_glob = (static_cast<ElectroMagn3DRZ*>(EMfields))->j_glob_;	 
		if (min_max == 0 && patch->isXmin() ) {		    
		    // for Br^(d,p)
		    vector<double> yp(1);
		    yp[0] = patch->getDomainLocalMin(1) - EMfields->oversize[1]*dr;
		    for (unsigned int j=0 ; j<nr_p ; j++) {
		        
		        double byW = 0.;
		        yp[0] += dr;
		        
		        // Lasers
		        for (unsigned int ilaser=0; ilaser< vecLaser.size(); ilaser++) {
		            byW += vecLaser[ilaser]->getAmplitude0(yp, time_dual, j, 0);
		        }
		        //x= Xmin
				unsigned int i=0;
		        (*BrRZ)(i,j) = Alpha_SM_Xmin   * (*EtRZ)(i,j)
		        +              Beta_SM_Xmin    *( (*BrRZ)(i+1,j))
		        +              Gamma_SM_Xmin   * byW
		        +              Delta_SM_Xmin   *( (*BlRZ)(i,j+1)- (*BlRZ)(i,j));
		        
		    }//j  ---end compute Br
		    
		    // for Bt^(d,d)
		    vector<double> yd(1);
		    yd[0] = patch->getDomainLocalMin(1) - (0.5+EMfields->oversize[1])*dr;
		    for (unsigned int j=0 ; j<nr_d ; j++) {
		        
		        double bzW = 0.;
		        yd[0] += dr;
		        
		        // Lasers
		        for (unsigned int ilaser=0; ilaser< vecLaser.size(); ilaser++) {
		            bzW += vecLaser[ilaser]->getAmplitude1(yd, time_dual, j, 0);
		        }
		        //x=Xmin
				unsigned int i=0;
		        (*BtRZ)(i,j) = -Alpha_SM_Xmin * (*ErRZ)(i,j+1)
		        +               Beta_SM_Xmin  *( (*BtRZ)(i+1,j+1))
		        +               Gamma_SM_Xmin * bzW
				+               Epsilon_SM_Xmin *imode/((j_glob+j-0.5)*dr)*(*BlRZ)(i,j+1) ;
		        
		    }//j  ---end compute Bt
		}
		else if (min_max == 1 && patch->isXmax() ) {
		    // for Br^(d,p)
		    vector<double> yp(1);
		    yp[0] = patch->getDomainLocalMin(1) - EMfields->oversize[1]*dr;
		    for (unsigned int j=0 ; j<nr_p ; j++) {
		        
		        double byE = 0.;
		        yp[0] += dr;
		        
		        // Lasers
		        for (unsigned int ilaser=0; ilaser< vecLaser.size(); ilaser++) {
		            byE += vecLaser[ilaser]->getAmplitude0(yp, time_dual, j, 0);
		        }
				unsigned int i= nl_d-1;
		        (*BrRZ)(i,j) = - Alpha_SM_Xmax   * (*EtRZ)(i-1,j)
		         +                   Beta_SM_Xmax    * (*BrRZ)(i-1,j)
		         +                   Gamma_SM_Xmax   * byE
		         +                   Delta_SM_Xmax   * ((*BlRZ)(i-1,j+1)- (*BlRZ)(i-1,j)); // Check x-index
		    
		        
		    }//j  ---end compute Br
		    
		    // for Bt^(d,d)
		    vector<double> yd(1);
		    yd[0] = patch->getDomainLocalMin(1) - (0.5+EMfields->oversize[1])*dr;
		    for (unsigned int j=0 ; j<nr_d ; j++) {
		        
		        double bzE = 0.;
		        yd[0] += dr;
		        unsigned int i= nl_d-1;
		        // Lasers
		        for (unsigned int ilaser=0; ilaser< vecLaser.size(); ilaser++) {
		            bzE += vecLaser[ilaser]->getAmplitude1(yd, time_dual, j, 0);
		        }
		        
		        (*BtRZ)(i,j) = Alpha_SM_Xmax * (*ErRZ)(i-1,j)
		         +                    Beta_SM_Xmax  * (*BtRZ)(i-1,j)
		         +                    Gamma_SM_Xmax * bzE
				 +					  Epsilon_SM_Xmax * imode /((j_glob+j-0.5)*dr)* (*BlRZ)(i-1,j)	;

		        
		    }//j  ---end compute Bt
		}
		else {
		    ERROR( "No Silver Muller along the axis" );
		}

	}
}
