
#include "MA_SolverRZ_norm.h"

#include "ElectroMagn3DRZ.h"
#include "cField2D.h"
#include <complex>
#include "dcomplex.h"
#include "Patch.h"
MA_SolverRZ_norm::MA_SolverRZ_norm(Params &params)
: SolverRZ(params)
{
}

MA_SolverRZ_norm::~MA_SolverRZ_norm()
{
}

void MA_SolverRZ_norm::operator() ( ElectroMagn* fields )
{
    #ifdef _TODO_RZ
    #endif
    // Loop on modes ?
    for (unsigned int imode=0 ; imode<Nmode ; imode++) {
    
        
    //int imode = 0;
     
    // Static-cast of the fields
    cField2D* ElRZ = (static_cast<ElectroMagn3DRZ*>(fields))->El_[imode];
    cField2D* ErRZ = (static_cast<ElectroMagn3DRZ*>(fields))->Er_[imode];
    cField2D* EtRZ = (static_cast<ElectroMagn3DRZ*>(fields))->Et_[imode];
    cField2D* BlRZ = (static_cast<ElectroMagn3DRZ*>(fields))->Bl_m[imode];
    cField2D* BrRZ = (static_cast<ElectroMagn3DRZ*>(fields))->Br_m[imode];
    cField2D* BtRZ = (static_cast<ElectroMagn3DRZ*>(fields))->Bt_m[imode];
    cField2D* JlRZ = (static_cast<ElectroMagn3DRZ*>(fields))->Jl_[imode];
    cField2D* JrRZ = (static_cast<ElectroMagn3DRZ*>(fields))->Jr_[imode];
    cField2D* JtRZ = (static_cast<ElectroMagn3DRZ*>(fields))->Jt_[imode];
    int j_glob    = (static_cast<ElectroMagn3DRZ*>(fields))->j_glob_;
    bool isYmin = (static_cast<ElectroMagn3DRZ*>(fields))->isYmin;
	bool isXmin = (static_cast<ElectroMagn3DRZ*>(fields))->isXmin;
    // Electric field Elr^(d,p)
    for (unsigned int i=0 ; i<nl_d ; i++) {
        for (unsigned int j=isYmin*3 ; j<nr_p ; j++) {
            (*ElRZ)(i,j) += -dt*(*JlRZ)(i,j)
                +                 dt/((j_glob+j)*dr)*((j+0.5)*(*BtRZ)(i,j+1) - (j-0.5)*(*BtRZ)(i,j) )
                +                 Icpx*dt*(double)imode/((j_glob+j)*dr)*(*BrRZ)(i,j);
             if (std::abs((*ElRZ)(i,j))>1.)
             {
                MESSAGE("ElRZMA");                
                MESSAGE(i);    
                MESSAGE(j);
                MESSAGE((*ElRZ)(i,j));
                }
        }
    }
    //MESSAGE("ElRZ");
    //MESSAGE((*ElRZ)(int(nl_d/2),int(nr_p/2)));
    // Electric field Er^(p,d)
    for (unsigned int i=0 ; i<nl_p ; i++) {
        for (unsigned int j=isYmin*3 ; j<nr_d ; j++) {
            (*ErRZ)(i,j) += -dt*(*JrRZ)(i,j)
                -                  dt_ov_dl * ( (*BtRZ)(i+1,j) - (*BtRZ)(i,j) )
                -                  Icpx*dt*(double)imode/((j_glob+j+0.5)*dr)* (*BlRZ)(i,j);
             if (std::abs((*ErRZ)(i,j))>1.){
                MESSAGE("ErRZMA");                
                MESSAGE(i);
               	MESSAGE(j);    
                MESSAGE((*ErRZ)(i,j));
                }
        }
    }
    //MESSAGE("ErRZ");
    //MESSAGE((*ErRZ)(int(nl_p/2),int(nr_d/2)));
    // Electric field Et^(p,p)
    for (unsigned int i=0 ;  i<nl_p ; i++) {
        for (unsigned int j=isYmin*3 ; j<nr_p ; j++) {
            (*EtRZ)(i,j) += -dt*(*JtRZ)(i,j)
                +                  dt_ov_dl * ( (*BrRZ)(i+1,j) - (*BrRZ)(i,j) )
                -                  dt_ov_dr * ( (*BlRZ)(i,j+1) - (*BlRZ)(i,j) );        
             if (std::abs((*EtRZ)(i,j))>1.){
                MESSAGE("EtRZMA ");
                MESSAGE(i);
                MESSAGE(j);    
                MESSAGE((*EtRZ)(i,j));
             }        
        }
    }
    //MESSAGE("EtRZ");
    //MESSAGE((*EtRZ)(int(nl_p/2),int(nr_p/2)));

    }
}

