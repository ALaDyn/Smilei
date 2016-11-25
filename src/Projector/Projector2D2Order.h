#ifndef PROJECTOR2D2ORDER_H
#define PROJECTOR2D2ORDER_H

#include "Projector2D.h"


class Projector2D2Order : public Projector2D {
public:
    Projector2D2Order(Params&, Patch* patch);
    ~Projector2D2Order();

    //! Project global current densities (EMfields->Jx_/Jy_/Jz_)
    inline void operator() (double* Jx, double* Jy, double* Jz, Particles &particles, unsigned int ipart, double gf, unsigned int bin, std::vector<unsigned int> &b_dim, int* iold, double* deltaold);
    //! Project global current densities (EMfields->Jx_/Jy_/Jz_/rho), diagFields timestep
    inline void operator() (double* Jx, double* Jy, double* Jz, double* rho, Particles &particles, unsigned int ipart, double gf, unsigned int bin, std::vector<unsigned int> &b_dim, int* iold, double* deltaold);

    //! Project global current charge (EMfields->rho_), frozen & diagFields timestep
    void operator() (double* rho, Particles &particles, unsigned int ipart, unsigned int bin, std::vector<unsigned int> &b_dim)  

    //! Project global current densities if Ionization in Species::dynamics,
    void operator() (Field* Jx, Field* Jy, Field* Jz, Particles &particles, int ipart, LocalFields Jion)  

    //!Wrapper
    void operator() (ElectroMagn* EMfields, Particles &particles, SmileiMPI* smpi, int istart, int iend, int ithread, int ibin, int clrw, int diag_flag, std::vector<unsigned int> &b_dim, int ispec)  

private:
    double one_third;
};

#endif

