
#include "MA_Solver3D_norm.h"

#include "ElectroMagn.h"
#include "Field3D.h"

MA_Solver3D_norm::MA_Solver3D_norm( Params &params )
    : Solver3D( params )
{
    // EMPTY
}

MA_Solver3D_norm::~MA_Solver3D_norm()
{
    // EMPTY
}

void MA_Solver3D_norm::operator()( ElectroMagn *fields )
{
    double *const __restrict__ Ex3D       = fields->Ex_->data();
    double *const __restrict__ Ey3D       = fields->Ey_->data();
    double *const __restrict__ Ez3D       = fields->Ez_->data();
    const double *const __restrict__ Bx3D = fields->Bx_->data();
    const double *const __restrict__ By3D = fields->By_->data();
    const double *const __restrict__ Bz3D = fields->Bz_->data();
    const double *const __restrict__ Jx3D = fields->Jx_->data();
    const double *const __restrict__ Jy3D = fields->Jy_->data();
    const double *const __restrict__ Jz3D = fields->Jz_->data();

    // Electric field Ex^(d,p,p)
#if defined( _GPU )
    const int sizeofEx = fields->Ex_->globalDims_;
    const int sizeofEy = fields->Ey_->globalDims_;
    const int sizeofEz = fields->Ez_->globalDims_;
    const int sizeofBx = fields->Bx_->globalDims_;
    const int sizeofBy = fields->By_->globalDims_;
    const int sizeofBz = fields->Bz_->globalDims_;

    #pragma acc parallel present( Ex3D[0:sizeofEx], Jx3D[0:sizeofEx], By3D[0:sizeofBy], Bz3D[0:sizeofBz] )
    #pragma acc loop gang
#elif defined( SMILEI_ACCELERATOR_GPU_OMP )
    #pragma omp target
    #pragma omp teams
    #pragma omp distribute parallel for collapse( 3 )
#endif
    for( unsigned int i=0 ; i<nx_d ; i++ ) {
#ifdef _GPU
        #pragma acc loop worker
#endif
        for( unsigned int j=0 ; j<ny_p ; j++ ) {
#ifdef _GPU
            #pragma acc loop vector
#endif
            for( unsigned int k=0 ; k<nz_p ; k++ ) {
                Ex3D[ i*(ny_p*nz_p) + j*(nz_p) + k ] += -dt*Jx3D[ i*(ny_p*nz_p) + j*(nz_p) + k ]
                    +                 dt_ov_dy * ( Bz3D[ i*(ny_d*nz_p) + (j+1)*(nz_p) + k   ] - Bz3D[ i*(ny_d*nz_p) + j*(nz_p) + k ] )
                    -                 dt_ov_dz * ( By3D[ i*(ny_p*nz_d) +  j   *(nz_d) + k+1 ] - By3D[ i*(ny_p*nz_d) + j*(nz_d) + k ] );
            }
        }
    }
    
    // Electric field Ey^(p,d,p)
#if defined( _GPU )
    #pragma acc parallel present( Ey3D[0:sizeofEy], Jy3D[0:sizeofEy], Bx3D[0:sizeofBx], Bz3D[0:sizeofBz] )
    #pragma acc loop gang
#elif defined( SMILEI_ACCELERATOR_GPU_OMP )
    #pragma omp target
    #pragma omp teams distribute parallel for collapse( 3 )
#endif
    for( unsigned int i=0 ; i<nx_p ; i++ ) {
#ifdef _GPU
        #pragma acc loop worker
#endif
        for( unsigned int j=0 ; j<ny_d ; j++ ) {
#ifdef _GPU
            #pragma acc loop vector
#endif
            for( unsigned int k=0 ; k<nz_p ; k++ ) {
                Ey3D[ i*(ny_d*nz_p) + j*(nz_p) + k ] += -dt*Jy3D[ i*(ny_d*nz_p) + j*(nz_p) + k ]
                    -                  dt_ov_dx * ( Bz3D[ (i+1)*(ny_d*nz_p) + j*(nz_p) + k   ] - Bz3D[ i*(ny_d*nz_p) + j*(nz_p) + k ] )
                    +                  dt_ov_dz * ( Bx3D[  i   *(ny_d*nz_d) + j*(nz_d) + k+1 ] - Bx3D[ i*(ny_d*nz_d) + j*(nz_d) + k ] );
            }
        }
    }
    
    // Electric field Ez^(p,p,d)
#if defined( _GPU )
    #pragma acc parallel present( Ez3D[0:sizeofEz], Jz3D[0:sizeofEz], Bx3D[0:sizeofBx], By3D[0:sizeofBy] )
    #pragma acc loop gang
#elif defined( SMILEI_ACCELERATOR_GPU_OMP )
    #pragma omp target
    #pragma omp teams distribute parallel for collapse( 3 )
#endif
    for( unsigned int i=0 ;  i<nx_p ; i++ ) {
#ifdef _GPU
    #pragma acc loop worker
#endif
        for( unsigned int j=0 ; j<ny_p ; j++ ) {
#ifdef _GPU
            #pragma acc loop vector
#endif
            for( unsigned int k=0 ; k<nz_d ; k++ ) {
                Ez3D[ i*(ny_p*nz_d) + j*(nz_d) + k ] += -dt*Jz3D[ i*(ny_p*nz_d) + j*(nz_d) + k ]
                    +                  dt_ov_dx * ( By3D[ (i+1)*(ny_p*nz_d) +  j   *(nz_d) + k ] - By3D[ i*(ny_p*nz_d) + j*(nz_d) + k ] )
                    -                  dt_ov_dy * ( Bx3D[  i   *(ny_d*nz_d) + (j+1)*(nz_d) + k ] - Bx3D[ i*(ny_d*nz_d) + j*(nz_d) + k ] );
            }
        }
    }
}
