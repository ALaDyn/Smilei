
#ifndef ELECTROMAGNBCRZ_BM_H
#define ELECTROMAGNBCRZ_BM_H


#include <vector>
#include <complex>
#include "Tools.h"
#include "ElectroMagnBC.h"
#include "ElectroMagn3DRZ.h"
#include "cField2D.h"


class Params;
class ElectroMagn;
class Field;

class ElectroMagnBCRZ_BM : public ElectroMagnBC {
public:
    
    ElectroMagnBCRZ_BM( Params &params, Patch* patch, unsigned int _min_max );
    ~ElectroMagnBCRZ_BM() {};
    
    virtual void apply(ElectroMagn* EMfields, double time_dual, Patch* patch) override;
    
    void save_fields(Field*, Patch* patch) override;
    void disableExternalFields() override;

    //! Save external fields for Buneman EM Boundary condition
    std::vector< std::complex<double> > Bl_val,  Br_val,  Bt_val;
    
private:
    
    
    //! Conversion factor from degree to radian
    double conv_deg2rad;
    
    //! Number of nodes on the primal grid in the x-direction
    unsigned int nl_p;
    
    //! Number of nodes on the dual grid in the x-direction
    unsigned int nl_d;
    
    //! Number of nodes on the primal grid in the y-direction
    unsigned int nr_p;
    
    //! Number of nodes on the dual grid in the y-direction
    unsigned int nr_d;
    
    //! Spatial step dx for 2D3V cartesian simulations
    double dl;
    
    //! Spatial step dy for 2D3V cartesian simulations
    double dr;
    
    //! Ratio of the time-step by the spatial-step dt/dx for 2D3V cartesian simulations
    double dt_ov_dl;
    
    //! Ratio of the time-step by the spatial-step dt/dy for 2D3V cartesian simulations
    double dt_ov_dr;
    
    //! Ratio of the spatial-step by the time-step dx/dt for 2D3V cartesian simulations
    double dr_ov_dt;
    
    //! Ratio of the spatial-step by the time-step dy/dt for 2D3V cartesian simulations
    double dl_ov_dt;
   
    
    //! Constant used for the Buneman boundary conditions (+R)
    double Alpha_Br_Rmax;
    
    //! Constant used for the Buneman boundary conditions (+R)
    double Beta_Br_Rmax;
    
    //! Constant used for the Buneman boundary conditions (+R)
    double Gamma_Br_Rmax;
    //! Constant used for the Buneman boundary conditions (+R)
    double CB_BM;
    //! Constant used for the Buneman boundary conditions (+R)
    double CE_BM;
	//! Number of modes
	unsigned int Nmode;
    
};

#endif

