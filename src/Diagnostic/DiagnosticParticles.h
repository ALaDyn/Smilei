#ifndef DIAGNOSTICPARTICLES_H
#define DIAGNOSTICPARTICLES_H

#include "Diagnostic.h"

#include "Histogram.h"

class DiagnosticParticles : public Diagnostic {
    friend class SmileiMPI;

public :
    
    //! Default constructor
    DiagnosticParticles( Params &params, SmileiMPI* smpi, Patch* patch, int diagId );
    //! Cloning constructor
    DiagnosticParticles( DiagnosticParticles* );
    //! Default destructor
    ~DiagnosticParticles() override;
    
    void openFile( Params& params, SmileiMPI* smpi, bool newfile ) override;
    
    void closeFile() override;
    
    bool prepare( int timestep ) override;
    
    void run( Patch* patch, int timestep ) override;
    
    void write(int timestep, SmileiMPI* smpi) override;
    
    //! Clear the array
    void clear();
    
     //! Get memory footprint of current diagnostic
    int getMemFootPrint() override {
        int size = output_size*sizeof(double);
        // + data_array + index_array +  axis_array
        // + nparts_max * (sizeof(double)+sizeof(int)+sizeof(double)) 
        return size;
    };
    
private :

    //! number of timesteps during which outputs are averaged
    int time_average;
    
    //! list of the species that will be accounted for
    std::vector<unsigned int> species;
    
    //! quantity to be summed into the output array
    std::string output;
    
    //! vector for saving the output array for time-averaging
    std::vector<double> data_sum;
    
    //! Histogram object
    Histogram * histogram;
    
    int output_size;
    
    //! Minimum and maximum spatial coordinates that are useful for this diag
    std::vector<double> spatial_min, spatial_max;
};

#endif

