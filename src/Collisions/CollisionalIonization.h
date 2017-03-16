
#ifndef COLLISIONALIONIZATION_H
#define COLLISIONALIONIZATION_H

#include <vector>

#include "Tools.h"
#include "Species.h"
#include "Params.h"

class Patch;

class CollisionalIonization
{

public:
    //! Constructor
    CollisionalIonization(int, int, double);
    //! Cloning Constructor
    CollisionalIonization(CollisionalIonization*);
    //! Destructor
    virtual ~CollisionalIonization() {};
    
    //! Initializes the arrays in the database and returns the index of these arrays in the DB
    virtual unsigned int createDatabase(double);
    //! Assigns the correct databases
    virtual void assignDatabase(unsigned int);
    
    //! Gets the k-th binding energy of any neutral or ionized atom with atomic number Z and charge Zstar
    double binding_energy(int Zstar, int k);
    
    //! Coefficients used for interpolating the energy over a given initial list
    static const double a1, a2, npointsm1;
    static const int npoints;
    
    //! Methods to prepare the ionization
    inline void prepare1(int Z_firstgroup) {
        electronFirst = Z_firstgroup==0 ? true : false;
        ne = 0.; ni = 0.; nei = 0.;
        };
    virtual void prepare2(Particles *p1, int i1, Particles *p2, int i2, bool);
    virtual void prepare3(double, double);
    //! Method to apply the ionization
    virtual void apply(Particles *p1, int i1, Particles *p2, int i2);
    //! Method to finish the ionization and put new electrons in place
    virtual void finish(Species *s1, Species *s2, Params&, Patch*);
    
    //! Local table of integrated cross-section
    std::vector<std::vector<double> > * crossSection;
    //! Local table of average secondary electron energy
    std::vector<std::vector<double> > * transferredEnergy;
    //! Local table of average incident electron energy lost
    std::vector<std::vector<double> > * lostEnergy;
    
    //! New electrons temporary species
    Particles new_electrons;
    
    //! Index of the atomic number in the databases
    unsigned int dataBaseIndex;

private:

    //! Simulation dimension
    int nDim;
    
    //! Atomic number
    int atomic_number;
    
    //! Global table of atomic numbers
    static std::vector<int> DB_Z;
    //! Global table of integrated cross-section
    static std::vector<std::vector<std::vector<double> > > DB_crossSection;
    //! Global table of average secondary electron energy
    static std::vector<std::vector<std::vector<double> > > DB_transferredEnergy;
    //! Global table of average incident electron energy lost
    static std::vector<std::vector<std::vector<double> > > DB_lostEnergy;
    
    //! True if first group of species is the electron
    bool electronFirst;
    //! Ionizing electron density
    double ne;
    //! Ionizing "hybrid" density
    double nei;
    //! Ion density
    double ni;
    //! Coefficient for the ionization frequency
    double coeff;
    
    //! Current ionization rate array (one cell per number of ionization events)
    std::vector<double> rate;
    std::vector<double> irate;
    //! Current ionization probability array (one cell per number of ionization events)
    std::vector<double> prob;
    
    //! Method called by ::apply to calculate the ionization, being sure that electrons are the first species
    void calculate(double, double, double, Particles *pe, int ie, Particles *pi, int ii);
    
};

//! Class to make empty ionization objects
class CollisionalNoIonization : public CollisionalIonization
{
public:
    CollisionalNoIonization() : CollisionalIonization(0,0,0.) {};
    ~CollisionalNoIonization(){};
    
    virtual unsigned int createDatabase(double) { return 0; };
    virtual void assignDatabase(unsigned int) {};
    
    void prepare2(Particles*, int, Particles*, int, bool){};
    void prepare3(double, int){};
    void apply(Particles*, int, Particles*, int){};
    void finish(Species*, Species*, Params&, Patch*) {};
};


//! Class to hold the databases

#endif
