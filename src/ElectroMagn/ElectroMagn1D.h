#ifndef ELECTROMAGN1D_H
#define ELECTROMAGN1D_H

#include "ElectroMagn.h"

class Params;

//! class ElectroMagn1D containing all information on the electromagnetic fields & currents for 1Dcartesian simulations
class ElectroMagn1D : public ElectroMagn
{
public:
    //! Constructor for ElectroMagn1D
    ElectroMagn1D(Params &params, DomainDecomposition* domain_decomposition, std::vector<Species*>& vecSpecies, Patch* patch);
    ElectroMagn1D( ElectroMagn1D* emFields, Params &params, Patch* patch );
    //! Destructor for ElectroMagn1D
    ~ElectroMagn1D();
    
    //! Oversize
    unsigned int oversize_;
    
    // --------------------------------------
    //  --------- PATCH IN PROGRESS ---------
    // --------------------------------------
    void initPoisson(Patch *patch);
    double compute_r();
    void compute_Ap(Patch *patch);
    void compute_Ap_relativistic_Poisson(Patch* patch, double gamma_mean);
    //Access to Ap
    double compute_pAp();
    void update_pand_r(double r_dot_r, double p_dot_Ap);
    void update_p(double rnew_dot_rnew, double r_dot_r);
    void initE(Patch *patch);
    void centeringE( std::vector<double> E_Add );
    
    double getEx_Xmin() { return (*Ex_)(index_bc_min[0]);}//(*Ex_)     (0); }
    double getEx_Xmax() { return (*Ex_)(index_bc_max[0]);}//(*Ex_)(nx_d-1); }
    
    double getEx_XminYmax() { return 0.; }
    double getEy_XminYmax() { return 0.; }
    double getEx_XmaxYmin() { return 0.; }
    double getEy_XmaxYmin() { return 0.; }
    
    // --------------------------------------
    //  --------- PATCH IN PROGRESS ---------
    // --------------------------------------
    
//    //! Method used to solve Maxwell-Ampere equation
//    void solveMaxwellAmpere();
    
    //! Method used to save the Magnetic fields (used to center them)
    void saveMagneticFields(bool);
    
    //! Method used to center the Magnetic fields (used to push the particles)
    void centerMagneticFields();
    
    //! Method used to apply a single-pass binomial filter on currents
    void binomialCurrentFilter();
    
    //! Creates a new field with the right characteristics, depending on the name
    Field * createField(std::string fieldname);
    
    //! Method used to compute the total charge density and currents by summing over all species
    void computeTotalRhoJ();
    
    //! Number of nodes on the primal grid
    unsigned int nx_p;
    
     //! Number of nodes on the dual grid
    unsigned int nx_d;
    
    //! Spatial step dx for 1Dcartesian simulations
    double dx;
    
    //! Ratio of the time-step by the spatial-step dt/dx for 1Dcartesian simulations
    double dt_ov_dx;
    
    //! Ratio of the spatial-step by the time-step dx/dt for 1Dcartesian simulations
    double dx_ov_dt;
    
    //! compute Poynting on borders
    void computePoynting();
    
    //! Method used to impose external fields
    void applyExternalField(Field*, Profile*, Patch*);
    
    void initAntennas(Patch* patch);
    
private:
    //! Initialize quantities needed in the creators of ElectroMagn1D
    void initElectroMagn1DQuantities(Params &params, Patch* patch);
};

#endif
