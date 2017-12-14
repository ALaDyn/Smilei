#include "Interpolator2D2Order.h"

#include <cmath>
#include <iostream>

#include "ElectroMagn.h"
#include "Field2D.h"
#include "Particles.h"

using namespace std;


// ---------------------------------------------------------------------------------------------------------------------
// Creator for Interpolator2D2Order
// ---------------------------------------------------------------------------------------------------------------------
Interpolator2D2Order::Interpolator2D2Order(Params &params, Patch *patch) : Interpolator2D(params, patch)
{

    dx_inv_ = 1.0/params.cell_length[0];
    dy_inv_ = 1.0/params.cell_length[1];

}

// ---------------------------------------------------------------------------------------------------------------------
// 2nd Order Interpolation of the fields at a the particle position (3 nodes are used)
// ---------------------------------------------------------------------------------------------------------------------
void Interpolator2D2Order::operator() (ElectroMagn* EMfields, Particles &particles, int ipart, double* ELoc, double* BLoc)
{
    // Static cast of the electromagnetic fields
    Field2D* Ex2D = static_cast<Field2D*>(EMfields->Ex_);
    Field2D* Ey2D = static_cast<Field2D*>(EMfields->Ey_);
    Field2D* Ez2D = static_cast<Field2D*>(EMfields->Ez_);
    Field2D* Bx2D = static_cast<Field2D*>(EMfields->Bx_m);
    Field2D* By2D = static_cast<Field2D*>(EMfields->By_m);
    Field2D* Bz2D = static_cast<Field2D*>(EMfields->Bz_m);
    
    
    // Normalized particle position
    double xpn = particles.position(0, ipart)*dx_inv_;
    double ypn = particles.position(1, ipart)*dy_inv_;
    
    
    // Indexes of the central nodes
    ip_ = round(xpn);
    id_ = round(xpn+0.5);
    jp_ = round(ypn);
    jd_ = round(ypn+0.5);
    
    
    // Declaration and calculation of the coefficient for interpolation
    double delta2;
    
    deltax   = xpn - (double)id_ + 0.5;
    delta2  = deltax*deltax;
    coeffxd_[0] = 0.5 * (delta2-deltax+0.25);
    coeffxd_[1] = 0.75 - delta2;
    coeffxd_[2] = 0.5 * (delta2+deltax+0.25);
    
    deltax   = xpn - (double)ip_;
    delta2  = deltax*deltax;
    coeffxp_[0] = 0.5 * (delta2-deltax+0.25);
    coeffxp_[1] = 0.75 - delta2;
    coeffxp_[2] = 0.5 * (delta2+deltax+0.25);
    
    deltay   = ypn - (double)jd_ + 0.5;
    delta2  = deltay*deltay;
    coeffyd_[0] = 0.5 * (delta2-deltay+0.25);
    coeffyd_[1] = 0.75 - delta2;
    coeffyd_[2] = 0.5 * (delta2+deltay+0.25);
    
    deltay   = ypn - (double)jp_;
    delta2  = deltay*deltay;
    coeffyp_[0] = 0.5 * (delta2-deltay+0.25);
    coeffyp_[1] = 0.75 - delta2;
    coeffyp_[2] = 0.5 * (delta2+deltay+0.25);
    
    
    //!\todo CHECK if this is correct for both primal & dual grids !!!
    // First index for summation
    ip_ = ip_ - i_domain_begin;
    id_ = id_ - i_domain_begin;
    jp_ = jp_ - j_domain_begin;
    jd_ = jd_ - j_domain_begin;
    
    
    int nparts( particles.size() );    

    // -------------------------
    // Interpolation of Ex^(d,p)
    // -------------------------
    *(ELoc+0*nparts) =  compute( &coeffxd_[1], &coeffyp_[1], Ex2D, id_, jp_);
    
    // -------------------------
    // Interpolation of Ey^(p,d)
    // -------------------------
    *(ELoc+1*nparts) = compute( &coeffxp_[1], &coeffyd_[1], Ey2D, ip_, jd_);
    
    // -------------------------
    // Interpolation of Ez^(p,p)
    // -------------------------
    *(ELoc+2*nparts) = compute( &coeffxp_[1], &coeffyp_[1], Ez2D, ip_, jp_);
    
    // -------------------------
    // Interpolation of Bx^(p,d)
    // -------------------------
    *(BLoc+0*nparts) = compute( &coeffxp_[1], &coeffyd_[1], Bx2D, ip_, jd_);
    
    // -------------------------
    // Interpolation of By^(d,p)
    // -------------------------
    *(BLoc+1*nparts) = compute( &coeffxd_[1], &coeffyp_[1], By2D, id_, jp_);
    
    // -------------------------
    // Interpolation of Bz^(d,d)
    // -------------------------
     *(BLoc+2*nparts) = compute( &coeffxd_[1], &coeffyd_[1], Bz2D, id_, jd_);

} // END Interpolator2D2Order

void Interpolator2D2Order::operator() (ElectroMagn* EMfields, Particles &particles, int ipart, LocalFields* ELoc, LocalFields* BLoc, LocalFields* JLoc, double* RhoLoc)
{
    // Interpolate E, B
    // Compute coefficient for ipart position
    // Static cast of the electromagnetic fields
    Field2D* Ex2D = static_cast<Field2D*>(EMfields->Ex_);
    Field2D* Ey2D = static_cast<Field2D*>(EMfields->Ey_);
    Field2D* Ez2D = static_cast<Field2D*>(EMfields->Ez_);
    Field2D* Bx2D = static_cast<Field2D*>(EMfields->Bx_m);
    Field2D* By2D = static_cast<Field2D*>(EMfields->By_m);
    Field2D* Bz2D = static_cast<Field2D*>(EMfields->Bz_m);
    
    
    // Normalized particle position
    double xpn = particles.position(0, ipart)*dx_inv_;
    double ypn = particles.position(1, ipart)*dy_inv_;
    
    
    // Indexes of the central nodes
    ip_ = round(xpn);
    id_ = round(xpn+0.5);
    jp_ = round(ypn);
    jd_ = round(ypn+0.5);
    
    
    // Declaration and calculation of the coefficient for interpolation
    double delta2;
    
    deltax   = xpn - (double)id_ + 0.5;
    delta2  = deltax*deltax;
    coeffxd_[0] = 0.5 * (delta2-deltax+0.25);
    coeffxd_[1] = 0.75 - delta2;
    coeffxd_[2] = 0.5 * (delta2+deltax+0.25);
    
    deltax   = xpn - (double)ip_;
    delta2  = deltax*deltax;
    coeffxp_[0] = 0.5 * (delta2-deltax+0.25);
    coeffxp_[1] = 0.75 - delta2;
    coeffxp_[2] = 0.5 * (delta2+deltax+0.25);
    
    deltay   = ypn - (double)jd_ + 0.5;
    delta2  = deltay*deltay;
    coeffyd_[0] = 0.5 * (delta2-deltay+0.25);
    coeffyd_[1] = 0.75 - delta2;
    coeffyd_[2] = 0.5 * (delta2+deltay+0.25);
    
    deltay   = ypn - (double)jp_;
    delta2  = deltay*deltay;
    coeffyp_[0] = 0.5 * (delta2-deltay+0.25);
    coeffyp_[1] = 0.75 - delta2;
    coeffyp_[2] = 0.5 * (delta2+deltay+0.25);
    
    
    //!\todo CHECK if this is correct for both primal & dual grids !!!
    // First index for summation
    ip_ = ip_ - i_domain_begin;
    id_ = id_ - i_domain_begin;
    jp_ = jp_ - j_domain_begin;
    jd_ = jd_ - j_domain_begin;
    
    
    // -------------------------
    // Interpolation of Ex^(d,p)
    // -------------------------
    (*ELoc).x =  compute( &coeffxd_[1], &coeffyp_[1], Ex2D, id_, jp_);
    
    // -------------------------
    // Interpolation of Ey^(p,d)
    // -------------------------
    (*ELoc).y = compute( &coeffxp_[1], &coeffyd_[1], Ey2D, ip_, jd_);
    
    // -------------------------
    // Interpolation of Ez^(p,p)
    // -------------------------
    (*ELoc).z = compute( &coeffxp_[1], &coeffyp_[1], Ez2D, ip_, jp_);
    
    // -------------------------
    // Interpolation of Bx^(p,d)
    // -------------------------
    (*BLoc).x = compute( &coeffxp_[1], &coeffyd_[1], Bx2D, ip_, jd_);
    
    // -------------------------
    // Interpolation of By^(d,p)
    // -------------------------
    (*BLoc).y = compute( &coeffxd_[1], &coeffyp_[1], By2D, id_, jp_);
    
    // -------------------------
    // Interpolation of Bz^(d,d)
    // -------------------------
    (*BLoc).z = compute( &coeffxd_[1], &coeffyd_[1], Bz2D, id_, jd_);
    
    // Static cast of the electromagnetic fields
    Field2D* Jx2D = static_cast<Field2D*>(EMfields->Jx_);
    Field2D* Jy2D = static_cast<Field2D*>(EMfields->Jy_);
    Field2D* Jz2D = static_cast<Field2D*>(EMfields->Jz_);
    Field2D* Rho2D= static_cast<Field2D*>(EMfields->rho_);
    
    
    // -------------------------
    // Interpolation of Jx^(d,p)
    // -------------------------
    (*JLoc).x = compute( &coeffxd_[1], &coeffyp_[1], Jx2D, id_, jp_);
    
    // -------------------------
    // Interpolation of Jy^(p,d)
    // -------------------------
    (*JLoc).y = compute( &coeffxp_[1], &coeffyd_[1], Jy2D, ip_, jd_);
    
    // -------------------------
    // Interpolation of Ez^(p,p)
    // -------------------------
    (*JLoc).z = compute( &coeffxp_[1], &coeffyp_[1], Jz2D, ip_, jp_);
    
    // -------------------------
    // Interpolation of Rho^(p,p)
    // -------------------------
    (*RhoLoc) = compute( &coeffxp_[1], &coeffyp_[1], Rho2D, ip_, jp_);

}

void Interpolator2D2Order::operator() (ElectroMagn* EMfields, Particles &particles, SmileiMPI* smpi, int *istart, int *iend, int ithread)
{
    std::vector<double> *Epart = &(smpi->dynamics_Epart[ithread]);
    std::vector<double> *Bpart = &(smpi->dynamics_Bpart[ithread]);
    std::vector<int> *iold = &(smpi->dynamics_iold[ithread]);
    std::vector<double> *delta = &(smpi->dynamics_deltaold[ithread]);
    
    //Loop on bin particles
    int nparts( particles.size() );
    for (int ipart=*istart ; ipart<*iend; ipart++ ) {
        //Interpolation on current particle
        (*this)(EMfields, particles, ipart, &(*Epart)[ipart], &(*Bpart)[ipart]);
        //Buffering of iol and delta
        (*iold)[ipart+0*nparts]  = ip_;
        (*iold)[ipart+1*nparts]  = jp_;
        (*delta)[ipart+0*nparts] = deltax;
        (*delta)[ipart+1*nparts] = deltay;
    }

}
