#ifndef DIAGNOSTICFIELDS_H
#define DIAGNOSTICFIELDS_H

#include "Diagnostic.h"


class DiagnosticFields  : public Diagnostic {

public :
    
    DiagnosticFields( Params &params, SmileiMPI* smpi, VectorPatch& vecPatches, int );
    ~DiagnosticFields() override;
    
    virtual void openFile( Params& params, SmileiMPI* smpi, bool newfile ) override;
    
    void closeFile() override;
    
    virtual void init(Params& params, SmileiMPI* smpi, VectorPatch& vecPatches) override;
    
    virtual bool prepare( int itime ) override;
    
    virtual void setFileSplitting( SmileiMPI* smpi, VectorPatch& vecPatches ) = 0;
    
    virtual void run( SmileiMPI* smpi, VectorPatch& vecPatches, int itime ) override;
    
    virtual void writeField(hid_t, int) = 0;
    
    virtual bool needsRhoJs(int itime) override;
    
    bool hasField(std::string field_name, std::vector<std::string> fieldsToDump);

    //! Get memory footprint of current diagnostic
    int getMemFootPrint() override {
        return 0;
    }

protected :
    
    //! Index of this diag
    unsigned int diag_n;
    
    //! Indexes of the fields to be dumped
    std::vector<unsigned int> fields_indexes;
    //! Names of the fields to be dumped
    std::vector<std::string> fields_names;
    
    //! Number of timesteps for time averaging
    int time_average;
    
    //! Inverse of the time average
    double time_average_inv;
    
    //! Property list for collective dataset write, set for // IO.
    hid_t write_plist;
    
    //! Number of cells to skip in each direction
    std::vector<unsigned int> patch_offset_in_grid;
    //! Number of cells in each direction
    std::vector<unsigned int> patch_size;
    //! Number of cells in a patch
    unsigned int total_patch_size;
    //! Buffer for the output of a field
    std::vector<double> data;
    
    //! 1st patch index of vecPatches
    unsigned int refHindex;
    
    hid_t data_group_id, iteration_group_id, filespace, memspace;
    
    //! Total number of patches
    int tot_number_of_patches;
    
    //! Copy patch field to current "data" buffer
    virtual void getField( Patch* patch, unsigned int ) = 0;
    
    //! Temporary dataset that is used for folding the 2D hilbert curve
    hid_t tmp_dset_id;
    
    //! Variable to store the status of a dataset (whether it exists or not)
    htri_t status;
    
    //! Tools for re-reading and re-writing the file in a folded pattern
    hid_t filespace_reread, filespace_firstwrite, memspace_reread, memspace_firstwrite;
    std::vector<double> data_reread, data_rewrite;
    
    //! True if this diagnostic requires the pre-calculation of the particle J & Rho
    bool hasRhoJs;
    
    //! Copy of the timestep
    double timestep;
    
    //! axes labels
    std::vector<std::string> axisLabels;
    //! Spacing of the grid
    std::vector<double> gridSpacing;
    //! Offset of the grid for all fields
    std::vector<double> gridGlobalOffset;
    //! Offset of the grid for each field
    std::vector<double> gridOffset;
    //! Units of each field
    std::vector<std::vector<double> > unitDimension;
    //! field solver description
    std::string fieldSolver, fieldSolverParameters;
    //! boundary conditions names
    std::vector<std::string> fieldBoundary, fieldBoundaryParameters, particleBoundary, particleBoundaryParameters;
    //! current smoothing description
    std::string currentSmoothing, currentSmoothingParameters;
    
    //! converts the boundary condition names to the openPMD format
    void em_bc(std::string, std::string&, std::string&);

};

#endif

