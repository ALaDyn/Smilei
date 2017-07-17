#ifndef ELECTROMAGNFACTORY_H
#define ELECTROMAGNFACTORY_H

#include <sstream>
#include "ElectroMagn.h"
#include "ElectroMagn1D.h"
#include "ElectroMagn2D.h"
#include "ElectroMagn3D.h"
#include "ElectroMagnBC.h"

#include "Patch.h"
#include "Params.h"
#include "Laser.h"
#include "Tools.h"

class ElectroMagnFactory {
public:
    static ElectroMagn* create(Params& params, std::vector<Species*>& vecSpecies,  Patch* patch) {
        ElectroMagn* EMfields = NULL;
        if ( params.geometry == "1d3v" ) {
            EMfields = new ElectroMagn1D(params, vecSpecies, patch);
        }
        else if ( params.geometry == "2d3v" ) {
            EMfields = new ElectroMagn2D(params, vecSpecies, patch);
        }
        else if ( params.geometry == "3d3v" ) {
            EMfields = new ElectroMagn3D(params, vecSpecies, patch);
        }
        else if ( params.geometry == "3drz" ) {
            EMfields = new ElectroMagn3DRZ(params, vecSpecies, patch);
        }
        else {
            ERROR( "Unknown geometry : " << params.geometry << "!" );
        }
        
        // -----------------
        // Lasers properties
        // -----------------
        if( patch->isMaster() ) MESSAGE(1, "Laser parameters :");
        int nlaser = PyTools::nComponents("Laser");
        for (int ilaser = 0; ilaser < nlaser; ilaser++) {
            Laser * laser = new Laser(params, ilaser, patch);
            if     ( laser->boxSide == "xmin" && EMfields->emBoundCond[0]) {
                if( patch->isXmin() ) laser->createFields(params, patch);
                EMfields->emBoundCond[0]->vecLaser.push_back( laser );
            }
            else if( laser->boxSide == "xmax" && EMfields->emBoundCond[1]) {
                if( patch->isXmax() ) laser->createFields(params, patch);
                EMfields->emBoundCond[1]->vecLaser.push_back( laser );
            }
            else
                delete laser;
        }
        
        // -----------------
        // ExtFields properties
        // -----------------
        unsigned int numExtFields=PyTools::nComponents("ExtField");
        for (unsigned int n_extfield = 0; n_extfield < numExtFields; n_extfield++) {
            ExtField extField;
            PyObject * profile;
            std::ostringstream name;
            if( !PyTools::extract("field",extField.field,"ExtField",n_extfield)) {
                ERROR("ExtField #"<<n_extfield<<": parameter 'field' not provided'");
            }
            // Now import the profile
            name.str("");
            name << "ExtField[" << n_extfield <<"].profile";
            if (!PyTools::extract_pyProfile("profile",profile,"ExtField",n_extfield))
                ERROR(" ExtField #"<<n_extfield<<": parameter 'profile' not understood");
            extField.profile = new Profile(profile, params.nDim_field, name.str());
            
            EMfields->extFields.push_back(extField);
        }
        
        
        // -----------------
        // Antenna properties
        // -----------------
        unsigned int numAntenna=PyTools::nComponents("Antenna");
        for (unsigned int n_antenna = 0; n_antenna < numAntenna; n_antenna++) {
            Antenna antenna;
            PyObject * profile;
            std::ostringstream name;
            antenna.field = NULL;
            if( !PyTools::extract("field",antenna.fieldName,"Antenna",n_antenna))
                ERROR("Antenna #"<<n_antenna<<": parameter 'field' not provided'");
            if (antenna.fieldName != "Jx" && antenna.fieldName != "Jy" && antenna.fieldName != "Jz")
                ERROR("Antenna #"<<n_antenna<<": parameter 'field' must be one of Jx, Jy, Jz");
            
            // Extract the space profile
            name.str("");
            name << "Antenna[" << n_antenna <<"].space_profile";
            if (!PyTools::extract_pyProfile("space_profile",profile,"Antenna",n_antenna))
                ERROR(" Antenna #"<<n_antenna<<": parameter 'space_profile' not understood");
            antenna.space_profile = new Profile(profile, params.nDim_field, name.str());
            
            // Extract the time profile
            name.str("");
            name << "Antenna[" << n_antenna <<"].time_profile";
            if (!PyTools::extract_pyProfile("time_profile" ,profile,"Antenna",n_antenna))
                ERROR(" Antenna #"<<n_antenna<<": parameter 'time_profile' not understood");
            antenna.time_profile =  new Profile(profile, 1, name.str());
            
            EMfields->antennas.push_back(antenna);
        }
        
        
        EMfields->finishInitialization(vecSpecies.size(), patch);
        
        return EMfields;
    }
    
    
    static ElectroMagn* clone(ElectroMagn* EMfields, Params& params, std::vector<Species*>& vecSpecies,  Patch* patch)
    {
        ElectroMagn* newEMfields = NULL;
        if ( params.geometry == "1d3v" ) {
            newEMfields = new ElectroMagn1D(static_cast<ElectroMagn1D*>(EMfields), params, patch);
        } else if ( params.geometry == "2d3v" ) {
            newEMfields = new ElectroMagn2D(static_cast<ElectroMagn2D*>(EMfields), params, patch);
        } else if ( params.geometry == "3d3v" ) {
            newEMfields = new ElectroMagn3D(static_cast<ElectroMagn3D*>(EMfields), params, patch);
        }
        
        
        // -----------------
        // Clone time-average fields
        // -----------------
        newEMfields->allFields_avg.resize( EMfields->allFields_avg.size() );
        for( unsigned int idiag=0; idiag<EMfields->allFields_avg.size(); idiag++) {
            for( unsigned int ifield=0; ifield<EMfields->allFields_avg[idiag].size(); ifield++)
                newEMfields->allFields_avg[idiag].push_back(
                    newEMfields->createField( EMfields->allFields_avg[idiag][ifield]->name )
                );
        }
        
        // -----------------
        // Clone Lasers properties
        // -----------------
        int nlaser;
        for( int iBC=0; iBC<2; iBC++ ) { // xmax and xmin
            if(! newEMfields->emBoundCond[iBC]) continue;
            
            newEMfields->emBoundCond[iBC]->vecLaser.resize(0);
            nlaser = EMfields->emBoundCond[iBC]->vecLaser.size();
            // Create lasers one by one
            for (int ilaser = 0; ilaser < nlaser; ilaser++) {
                // Create laser
                Laser * laser = new Laser(EMfields->emBoundCond[iBC]->vecLaser[ilaser], params);
                // If patch is on border, then fill the fields arrays
                if( (iBC==0 && patch->isXmin())
                   || (iBC==1 && patch->isXmax()) )
                    laser->createFields(params, patch);
                // Append the laser to the vector
                newEMfields->emBoundCond[iBC]->vecLaser.push_back( laser );
            }
        }
        
        // -----------------
        // Clone ExtFields properties
        // -----------------
        for (unsigned int n_extfield = 0; n_extfield < EMfields->extFields.size(); n_extfield++) {
            ExtField extField;
            extField.field  = EMfields->extFields[n_extfield].field;
            extField.profile = EMfields->extFields[n_extfield].profile;
            newEMfields->extFields.push_back(extField);
        }
        
        // -----------------
        // Clone Antenna properties
        // -----------------
        for (unsigned int n_antenna = 0; n_antenna < EMfields->antennas.size(); n_antenna++) {
            Antenna antenna;
            antenna.field = NULL;
            antenna.fieldName     = EMfields->antennas[n_antenna].fieldName    ;
            antenna.space_profile = EMfields->antennas[n_antenna].space_profile;
            antenna.time_profile  = EMfields->antennas[n_antenna].time_profile ;
            newEMfields->antennas.push_back(antenna);
        }
        
        
        newEMfields->finishInitialization(vecSpecies.size(), patch);
        
        return newEMfields;
    }
    
};

#endif
