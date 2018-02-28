#ifndef INTERPOLATOR1D_H
#define INTERPOLATOR1D_H


#include "Interpolator.h"

//  --------------------------------------------------------------------------------------------------------------------
//! Class Interpolator 1D
//  --------------------------------------------------------------------------------------------------------------------
class Interpolator1D : public Interpolator
{
public:
    Interpolator1D(Params &params, Patch *patch) ;
    
    virtual ~Interpolator1D() override {};
    
    virtual void operator() (ElectroMagn* EMfields, Particles &particles, SmileiMPI* smpi, int *istart, int *iend, int ithread) override = 0;
    virtual void operator() (ElectroMagn* EMfields, Particles &particles, SmileiMPI* smpi, int *istart, int *iend, int ithread, LocalFields* JLoc, double* RhoLoc) override = 0;
    virtual void operator() (ElectroMagn* EMfields, Particles &particles, double *buffer, int offset, std::vector<unsigned int> * selection) override = 0;

protected:
    //! Inverse of the spatial-step
    double dx_inv_;
    unsigned int index_domain_begin;
};

#endif
