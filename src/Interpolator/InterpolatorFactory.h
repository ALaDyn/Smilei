#ifndef INTERPOLATORFACTORY_H
#define INTERPOLATORFACTORY_H

#include "Interpolator.h"
#include "Interpolator1D2Order.h"
#include "Interpolator1D3Order.h"
#include "Interpolator1D4Order.h"
#include "Interpolator2D2Order.h"
#include "Interpolator2D4Order.h"
#include "Interpolator3D2Order.h"
#include "Interpolator3D2Order_env.h"
#include "Interpolator3D4Order.h"
#include "InterpolatorRZ2Order.h"

#ifdef _VECTO
#include "Interpolator2D2OrderV.h"
#include "Interpolator3D2OrderV.h"
#include "Interpolator3D2Order_envV.h"
#endif

#include "Params.h"
#include "Patch.h"

#include "Tools.h"

class InterpolatorFactory {
public:
    static Interpolator* create(Params& params, Patch *patch) {
        Interpolator* Interp = NULL;
        // ---------------
        // 1Dcartesian simulation
        // ---------------
        if ( ( params.geometry == "1Dcartesian" ) && ( params.interpolation_order == 2 ) ) {
            Interp = new Interpolator1D2Order(params, patch);
        }
        else if ( ( params.geometry == "1Dcartesian" ) && ( params.interpolation_order == 4 ) ) {
            Interp = new Interpolator1D4Order(params, patch);
        }
        // ---------------
        // 2Dcartesian simulation
        // ---------------
        else if ( ( params.geometry == "2Dcartesian" ) && ( params.interpolation_order == 2 ) ) {
            if (!params.vecto)
                Interp = new Interpolator2D2Order(params, patch);
#ifdef _VECTO
            else
                Interp = new Interpolator2D2OrderV(params, patch);
#endif
        }
        else if ( ( params.geometry == "2Dcartesian" ) && ( params.interpolation_order == 4 ) ) {
            Interp = new Interpolator2D4Order(params, patch);
        }
        // ---------------
        // 3Dcartesian simulation
        // ---------------
         else if ( ( params.geometry == "3Dcartesian" ) && ( params.interpolation_order == 2 ) ) {
            if (!params.vecto)
                Interp = new Interpolator3D2Order(params, patch);
#ifdef _VECTO
            else
                Interp = new Interpolator3D2OrderV(params, patch);                    
#endif
        }
        else if ( ( params.geometry == "3Dcartesian" ) && ( params.interpolation_order == 4 ) ){
            Interp = new Interpolator3D4Order(params, patch);
        }
        // ---------------
        // 3dRZ simulation
        // ---------------
        else if ( params.geometry == "3drz" ) {
            Interp = new InterpolatorRZ2Order(params, patch);
        }

        else {
            ERROR( "Unknwon parameters : " << params.geometry << ", Order : " << params.interpolation_order );
        }

        return Interp;
    } // end InterpolatorFactory::create


static Interpolator* create_env_interpolator(Params& params, Patch *patch) {
    Interpolator* Interp_envelope = NULL;
    // ---------------
    // 1Dcartesian simulation
    // ---------------
    if ( ( params.geometry == "1Dcartesian" ) && ( params.interpolation_order == 2 ) ) {
        ERROR("Envelope not yet implemented in 1Dcartesian geometry");
    }
    else if ( ( params.geometry == "1Dcartesian" ) && ( params.interpolation_order == 4 ) ) {
        ERROR("Envelope not yet implemented in 1Dcartesian geometry");
    }
    // ---------------
    // 2Dcartesian simulation
    // ---------------
    else if ( ( params.geometry == "2Dcartesian" ) && ( params.interpolation_order == 2 ) ) {
        if (!params.vecto){
            ERROR("Envelope not yet implemented in 2Dcartesian geometry");}
#ifdef _VECTO
        else{
            ERROR("Envelope not yet implemented in 2Dcartesian geometry");}
#endif
    }
    else if ( ( params.geometry == "2Dcartesian" ) && ( params.interpolation_order == 4 ) ) {
        ERROR("Envelope not yet implemented in 2Dcartesian geometry");
    }
    // ---------------
    // 3Dcartesian simulation
    // ---------------
     else if ( ( params.geometry == "3Dcartesian" ) && ( params.interpolation_order == 2 ) ) {
        if (!params.vecto)  
            Interp_envelope = new Interpolator3D2Order_env(params, patch);                   
#ifdef _VECTO
        else
            Interp_envelope = new Interpolator3D2Order_envV(params, patch); 
#endif
    }
    else if ( ( params.geometry == "3Dcartesian" ) && ( params.interpolation_order == 4 ) ) {
        ERROR("Envelope interpolator not yet implemented for order 4");
        //Interp_envelope = new Interpolator3D2Order_env(params, patch);} // end if for envelope model
    }
    // ---------------
    // 3dRZ simulation
    // ---------------
    else if ( params.geometry == "3drz" ) {
        ERROR("Envelope interpolator not yet implemented for quasi-cylindrical geometry");
    }
    else {
        ERROR( "Unknwon parameters : " << params.geometry << ", Order : " << params.interpolation_order );
    }

    return Interp_envelope;
    } // end InterpolatorFactory::create_env_interpolator

};

#endif
