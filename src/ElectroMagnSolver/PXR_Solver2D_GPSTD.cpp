
#include "PXR_Solver2D_GPSTD.h"

#include "ElectroMagn.h"
#include "Field2D.h"
#include "interface.h"

PXR_Solver2D_GPSTD::PXR_Solver2D_GPSTD(Params &params)
: Solver2D(params)
{
}

PXR_Solver2D_GPSTD::~PXR_Solver2D_GPSTD()
{
}

void PXR_Solver2D_GPSTD::coupling( Params &params, ElectroMagn* EMfields )
{
    int cdim=2;
    int n0,n1,n2;
    int ov0,ov1,ov2;
    // unable to convert unsigned int to an iso_c_binding supported type 

    n0=(int) (1 +  params.n_space[0]*params.global_factor[0]);
    n1=(int) (1 +  params.n_space[1]*params.global_factor[1]);
    n2=0;
    ov0=(int) params.oversize[0];
    ov1=(int) params.oversize[1];
    ov2=0;
    double dyy = std::numeric_limits<double>::infinity() ;
    int ordery = 0;
    params.norderx = params.norder[0];
    params.norderz = params.norder[1];


    Field2D* Ex2D_pxr = static_cast<Field2D*>( EMfields->Ex_pxr);
    Field2D* Ey2D_pxr = static_cast<Field2D*>( EMfields->Ey_pxr);
    Field2D* Ez2D_pxr = static_cast<Field2D*>( EMfields->Ez_pxr);
    Field2D* Bx2D_pxr = static_cast<Field2D*>( EMfields->Bx_pxr);
    Field2D* By2D_pxr = static_cast<Field2D*>( EMfields->By_pxr);
    Field2D* Bz2D_pxr = static_cast<Field2D*>( EMfields->Bz_pxr);
    Field2D* Jx2D_pxr = static_cast<Field2D*>( EMfields->Jx_pxr);
    Field2D* Jy2D_pxr = static_cast<Field2D*>( EMfields->Jy_pxr);
    Field2D* Jz2D_pxr = static_cast<Field2D*>( EMfields->Jz_pxr);
    Field2D* rho2D_pxr = static_cast<Field2D*>( EMfields->rho_pxr);
    Field2D* rhoold2D_pxr = static_cast<Field2D*>( EMfields->rhoold_pxr);

#ifdef _PICSAR
    picsar::init_params_picsar(&n0,&n1,&n2,
                       &params.cell_length[2],&dyy,&params.cell_length[0],&params.timestep,
                       &ov0,&ov1,&ov2,
                       &params.norderz,&ordery,&params.norderx,
                       &params.is_spectral,
                       &(Ex2D_pxr->data_[0]),
                       &(Ey2D_pxr->data_[0]),
                       &(Ez2D_pxr->data_[0]),
                       &(Bx2D_pxr->data_[0]),
                       &(By2D_pxr->data_[0]),
                       &(Bz2D_pxr->data_[0]),
                       &(Jx2D_pxr->data_[0]),
                       &(Jy2D_pxr->data_[0]),
                       &(Jz2D_pxr->data_[0]),
                       &(rho2D_pxr->data_[0]),
                       &(rhoold2D_pxr->data_[0]),&cdim);
#else
    ERROR( "Smilei not linked with picsar" );
#endif

}

void PXR_Solver2D_GPSTD::operator() ( ElectroMagn* fields )
{
    duplicate_field_into_pxr( fields );

#ifdef _PICSAR
    picsar::push_psatd_ebfield_3d_();
#else
    ERROR( "Smilei not linked with picsar" );
#endif

    duplicate_field_into_smilei( fields );

}

