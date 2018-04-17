#include "IonizationFromRate.h"

#include <cmath>

#include "Particles.h"
#include "ParticleData.h"
#include "Species.h"

using namespace std;



IonizationFromRate::IonizationFromRate(Params& params, Species * species) : Ionization(params, species) {
    
    DEBUG("Creating the FromRate Ionizaton class");

    maximum_charge_state_ = species->maximum_charge_state;
    ionization_rate = species->ionization_rate;
    
    DEBUG("Finished Creating the FromRate Ionizaton class");

}



void IonizationFromRate::operator() (Particles* particles, unsigned int ipart_min, unsigned int ipart_max, vector<double> *Epart, ElectroMagn* EMfields, Projector* Proj) {
    
    //unsigned int Z, Zp1, newZ, k_times;
    unsigned int Z, k_times;
    vector<double> rate;
    
    // Leave if nothing to do
    if( ipart_min >= ipart_max ) return;
    
#ifdef SMILEI_USE_NUMPY
    
    // Set a python variable "Main.iteration" to itime so that it can be accessed in the filter
    #pragma omp critical
    PyTools::setIteration( itime );
    
    // Run python to evaluate the ionization rate for each particle
    PyArrayObject *ret;
    unsigned int npart = ipart_max - ipart_min;
    ParticleData particleData(npart);
    particleData.startAt( ipart_min );
    //Particles * p = particles;
    particleData.set( particles );
    #pragma omp critical
    ret = (PyArrayObject*)PyObject_CallFunctionObjArgs(ionization_rate, particleData.get(), NULL);
    PyTools::checkPyError();
    if( ret == NULL )
        ERROR("ionization_rate profile has not provided a correct result");
    // Loop the return value and store the particle IDs 
    double* arr = (double*) PyArray_GETPTR1( ret, 0 );

    rate.resize(npart);
    for(unsigned int i=0; i<npart; i++)
        rate[i] = arr[i];
    Py_DECREF(ret);
#endif

    
    for( unsigned int ipart=ipart_min ; ipart<ipart_max; ipart++ ) {
        
        // Current charge state of the ion
        Z = (unsigned int)(particles->charge(ipart));
        
        // If ion already fully ionized then skip
        if (Z==maximum_charge_state_) continue;
        
        // Start of the Monte-Carlo routine  (At the moment, only 1 ionization per timestep is possible)
        // k_times will give the nb of ionization events
        k_times = 0;
        double ran_p = Rand::uniform();
        if ( ran_p < 1.0 - exp(-rate[ipart-ipart_min]*dt) ) {
            k_times        = 1;
        }
        
        /* NO IONIZATION CURRENT HERE!
        // Compute ionization current
        factorJion *= TotalIonizPot;
        Jion.x = factorJion * *(Ex+ipart);
        Jion.y = factorJion * *(Ey+ipart);
        Jion.z = factorJion * *(Ez+ipart);
        
        (*Proj)(EMfields->Jx_, EMfields->Jy_, EMfields->Jz_, *particles, ipart, Jion);
         */
        
        // Creation of the new electrons
        // (variable weights are used)
        // -----------------------------
        if (k_times!=0) {
            new_electrons.create_particle();
            //new_electrons.initialize( new_electrons.size()+1, new_electrons.dimension() );
            int idNew = new_electrons.size() - 1;
            for (unsigned int i=0; i<new_electrons.dimension(); i++) {
                new_electrons.position(i,idNew)=particles->position(i, ipart);
            }
            for (unsigned int i=0; i<3; i++) {
                new_electrons.momentum(i,idNew) = particles->momentum(i, ipart)*ionized_species_invmass;
            }
            new_electrons.weight(idNew)=double(k_times)*particles->weight(ipart);
            new_electrons.charge(idNew)=-1;
            
            // Increase the charge of the particle
            particles->charge(ipart) += k_times;
        }
        
        
    } // Loop on particles
}
