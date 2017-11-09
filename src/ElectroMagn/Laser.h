
#ifndef Laser_H
#define Laser_H

#include "PyTools.h"
#include "Profile.h"
#include "Field.h"
#include "Field1D.h"
#include "Field2D.h"

#include <vector>
#include <string>
#include <cmath>

class Params;
class Patch;


// Class for choosing specific profiles
class LaserProfile {
friend class SmileiMPI;
public:
    LaserProfile() {};
    virtual ~LaserProfile() {};
    virtual double getAmplitude(std::vector<double> pos, double t, int j, int k) {return 0.;};
    virtual std::string getInfo() { return "?"; };
    virtual void createFields(Params& params, Patch* patch) {};
    virtual void initFields  (Params& params, Patch* patch) {};
};


//  --------------------------------------------------------------------------------------------------------------------
// Class for holding all information about one laser
//  --------------------------------------------------------------------------------------------------------------------
class Laser {
friend class SmileiMPI;
friend class Patch;
public:
    //! Normal laser constructor
    Laser(Params &params, int ilaser, Patch* patch);
    //! Cloning laser constructor
    Laser(Laser*, Params&);
    ~Laser();
    void clean();
    
    //! Gets the amplitude from both time and space profiles (By)
    inline double getAmplitude0(std::vector<double> pos, double t, int j, int k) {
        return profiles[0]->getAmplitude(pos, t, j, k);
    }
    //! Gets the amplitude from both time and space profiles (Bz)
    inline double getAmplitude1(std::vector<double> pos, double t, int j, int k) {
        return profiles[1]->getAmplitude(pos, t, j, k);
    }
    
    void createFields(Params& params, Patch* patch)
    {
        profiles[0]->createFields(params, patch);
        profiles[1]->createFields(params, patch);
    };
    void initFields(Params& params, Patch* patch)
    {
        profiles[0]->initFields(params, patch);
        profiles[1]->initFields(params, patch);
    };
    
    //! Side (xmin/xmax) from which the laser enters the box
    std::string box_side;
    
    //! Disables the laser
    void disable();
    
protected:
    //! Space and time profiles (Bx and By)
    std::vector<LaserProfile*> profiles;
    
private:
    //! True if spatio-temporal profile (Bx and By)
    std::vector<bool> spacetime;
    

};



// Laser profile for separable space and time
class LaserProfileSeparable : public LaserProfile {
friend class SmileiMPI;
friend class Patch;
public:
    LaserProfileSeparable(double, Profile*, Profile*, Profile*, Profile*, bool);
    LaserProfileSeparable(LaserProfileSeparable*);
    ~LaserProfileSeparable();
    void createFields(Params& params, Patch* patch);
    void initFields  (Params& params, Patch* patch);
    double getAmplitude(std::vector<double> pos, double t, int j, int k);
protected:
    Field *space_envelope, *phase;
private:
    bool primal;
    double omega;
    Profile *timeProfile, *chirpProfile, *spaceProfile, *phaseProfile;
};

// Laser profile for non-separable space and time
class LaserProfileNonSeparable : public LaserProfile {
friend class SmileiMPI;
public:
    LaserProfileNonSeparable(Profile * spaceAndTimeProfile)
     : spaceAndTimeProfile(spaceAndTimeProfile) {};
    LaserProfileNonSeparable(LaserProfileNonSeparable* lp)
     : spaceAndTimeProfile( new Profile(lp->spaceAndTimeProfile) ) {};
    ~LaserProfileNonSeparable();
    inline double getAmplitude(std::vector<double> pos, double t, int j, int k) {
        double amp;
        #pragma omp critical
        amp = spaceAndTimeProfile->valueAt(pos, t);
        return amp;
    }
private:
    Profile * spaceAndTimeProfile;
};

// Null laser profile
class LaserProfileNULL : public LaserProfile {
public:
    LaserProfileNULL() {};
    ~LaserProfileNULL() {};
    
    inline double getAmplitude(std::vector<double> pos, double t, int j, int k) {
        return 0.;
    }
};


#endif
