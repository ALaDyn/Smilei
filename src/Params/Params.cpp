#include <algorithm>
#include <cmath>
#include <ctime>
#include <iomanip>

#define SMILEI_IMPORT_ARRAY

#include "PyTools.h"
#include "Params.h"
#include "Species.h"
#include "Tools.h"
#include "SmileiMPI.h"
#include "H5.h"

#include "pyinit.pyh"
#include "pyprofiles.pyh"
#include "pycontrol.pyh"

using namespace std;

namespace Rand
{
    std::random_device device;
    std::mt19937 gen(device());

    std::uniform_real_distribution<double> uniform_distribution(0., 1.);
    double uniform() {
        return uniform_distribution(gen);
    }

    std::uniform_real_distribution<double> uniform_distribution1(0., 1.-1e-11);
    double uniform1() {
        return uniform_distribution1(gen);
    }

    std::uniform_real_distribution<double> uniform_distribution2(-1., 1.);
    double uniform2() {
        return uniform_distribution2(gen);
    }
    double normal(double stddev) {
        std::normal_distribution<double> normal_distribution(0., stddev);
        return normal_distribution(gen);
    }
}

#define DO_EXPAND(VAL)  VAL ## 1
#define EXPAND(VAL)     DO_EXPAND(VAL)
#ifdef SMILEI_USE_NUMPY
#if !defined(NUMPY_IMPORT_ARRAY_RETVAL) || (EXPAND(NUMPY_IMPORT_ARRAY_RETVAL) == 1)
    void smilei_import_array() { // python 2
        import_array();
    }
#else
    void* smilei_import_array() { // hack for python3
        import_array();
        return NULL;
    }
#endif
#endif


// ---------------------------------------------------------------------------------------------------------------------
// Params : open & parse the input data file, test that parameters are coherent
// ---------------------------------------------------------------------------------------------------------------------
Params::Params(SmileiMPI* smpi, std::vector<std::string> namelistsFiles) :
namelist("")
{
    
    MESSAGE("HDF5 version "<<H5_VERS_MAJOR << "." << H5_VERS_MINOR << "." << H5_VERS_RELEASE);
    
    if((((H5_VERS_MAJOR==1) && (H5_VERS_MINOR==8) && (H5_VERS_RELEASE>16)) || \
        ((H5_VERS_MAJOR==1) && (H5_VERS_MINOR>8)) || \
        (H5_VERS_MAJOR>1))) {
        WARNING("Smilei suggests using HDF5 version 1.8.16");
        WARNING("Newer version are not tested and may cause the code to behave incorrectly");
        WARNING("See http://hdf-forum.184993.n3.nabble.com/Segmentation-fault-using-H5Dset-extent-in-parallel-td4029082.html");
    }
    
    
    if (namelistsFiles.size()==0) ERROR("No namelists given!");
    
    //string commandLineStr("");
    //for (unsigned int i=0;i<namelistsFiles.size();i++) commandLineStr+="\""+namelistsFiles[i]+"\" ";
    //MESSAGE(1,commandLineStr);
    
    //init Python
    PyTools::openPython();
#ifdef SMILEI_USE_NUMPY
    smilei_import_array();
    // Workaround some numpy multithreading bug
    // https://github.com/numpy/numpy/issues/5856
    // We basically call the command numpy.seterr(all="ignore")
    PyObject* numpy = PyImport_ImportModule("numpy");
    PyObject* seterr = PyObject_GetAttrString(numpy, "seterr");
    PyObject* args = PyTuple_New(0);
    PyObject* kwargs = Py_BuildValue("{s:s}", "all", "ignore");
    PyObject* ret = PyObject_Call(seterr, args, kwargs);
    Py_DECREF(ret);
    Py_DECREF(args);
    Py_DECREF(kwargs);
    Py_DECREF(seterr);
    Py_DECREF(numpy);
#endif
    
    // Print python version
    MESSAGE(1, "Python version "<<PyTools::python_version());
    
    // First, we tell python to filter the ctrl-C kill command (or it would prevent to kill the code execution).
    // This is done separately from other scripts because we don't want it in the concatenated python namelist.
    PyTools::checkPyError();
    string command = "import signal\nsignal.signal(signal.SIGINT, signal.SIG_DFL)";
    if( !PyRun_SimpleString(command.c_str()) ) PyTools::checkPyError();
    
    // Running pyinit.py
    runScript(string(reinterpret_cast<const char*>(pyinit_py), pyinit_py_len), "pyinit.py");
    
    runScript(Tools::merge("smilei_version='",string(__VERSION),"'\n"), string(__VERSION));
    
    // Running pyprofiles.py
    runScript(string(reinterpret_cast<const char*>(pyprofiles_py), pyprofiles_py_len), "pyprofiles.py");
    
    // here we add the rank, in case some script need it
    PyModule_AddIntConstant(PyImport_AddModule("__main__"), "smilei_mpi_rank", smpi->getRank());
    
    // here we add the MPI size, in case some script need it
    PyModule_AddIntConstant(PyImport_AddModule("__main__"), "smilei_mpi_size", smpi->getSize());
    
    // here we add the larget int, important to get a valid seed for randomization
    PyModule_AddIntConstant(PyImport_AddModule("__main__"), "smilei_rand_max", RAND_MAX);
    
    // Running the namelists
    for (vector<string>::iterator it=namelistsFiles.begin(); it!=namelistsFiles.end(); it++) {
        string strNamelist="";
        if (smpi->isMaster()) {
            ifstream istr(it->c_str());
            // If file
            if (istr.is_open()) {
                std::stringstream buffer;
                buffer << istr.rdbuf();
                strNamelist+=buffer.str();
            // If command
            } else {
                string command = *it;
                // Remove quotes
                unsigned int s = command.size();
                if( s>1 && command.substr(0,1)=="\"" && command.substr(s-1,1)=="\"" )
                    command = command.substr(1, s - 2);
                // Add to namelist
                strNamelist = Tools::merge("# Smilei:) From command line:\n" , command);
            }
            strNamelist +="\n";
        }
        smpi->bcast(strNamelist);
        runScript(strNamelist,(*it));
    }
    // Running pycontrol.py
    runScript(string(reinterpret_cast<const char*>(pycontrol_py), pycontrol_py_len),"pycontrol.py");
    
    smpi->barrier();
    
    // Error if no block Main() exists
    if( PyTools::nComponents("Main") == 0 )
        ERROR("Block Main() not defined");
    
    // CHECK namelist on python side
    PyTools::runPyFunction("_smilei_check");
    smpi->barrier();
    
    // Python makes the checkpoint dir tree
    if( ! smpi->test_mode ) {
        PyTools::runPyFunction("_prepare_checkpoint_dir");
        smpi->barrier();
    }
    
    // Now the string "namelist" contains all the python files concatenated
    // It is written as a file: smilei.py
    if (smpi->isMaster()) {
        ofstream out_namelist("smilei.py");
        if (out_namelist.is_open()) {
            out_namelist << "# coding: utf-8" << endl << endl ;
            out_namelist << namelist;
            out_namelist.close();
        }
    }
    
    // random seed
    unsigned int random_seed=0;
    if (PyTools::extract("random_seed", random_seed, "Main")) {
        Rand::gen = std::mt19937(random_seed);
    }
    
    // --------------
    // Stop & Restart
    // --------------
    
    restart = false;
    std::vector<std::string> _unused_restart_files;
    if( PyTools::nComponents("Checkpoints")>0 && PyTools::extract("restart_files", _unused_restart_files, "Checkpoints")) {
        MESSAGE(1,"Code will restart");
        restart=true;
    }
    
    // ---------------------
    // Normalisation & units
    // ---------------------
    
    reference_angular_frequency_SI = 0.;
    PyTools::extract("reference_angular_frequency_SI",reference_angular_frequency_SI, "Main");
    
    
    // -------------------
    // Simulation box info
    // -------------------
    
    // geometry of the simulation
    geometry = "";
    if( !PyTools::extract("geometry", geometry, "Main") )
        ERROR("Parameter Main.geometry is required");
    if (geometry!="1Dcartesian" && geometry!="2Dcartesian" && geometry!="3Dcartesian" && geometry!="3drz") {
        ERROR("Main.geometry `" << geometry << "` invalid");
    }
    setDimensions();
    
    // interpolation order
    PyTools::extract("interpolation_order", interpolation_order, "Main");
    if (interpolation_order!=2 && interpolation_order!=4) {
        ERROR("Main.interpolation_order " << interpolation_order << " not defined");
    }
    
    //!\todo (MG to JD) Please check if this parameter should still appear here
    // Disabled, not compatible for now with particles sort
    // if ( !PyTools::extract("exchange_particles_each", exchange_particles_each) )
    exchange_particles_each = 1;
    
    PyTools::extract("every_clean_particles_overhead", every_clean_particles_overhead, "Main");
    
    // TIME & SPACE RESOLUTION/TIME-STEPS
    
    // reads timestep & cell_length
    PyTools::extract("timestep", timestep, "Main");
    res_time = 1.0/timestep;
    
    PyTools::extract("cell_length",cell_length, "Main");
    if (cell_length.size()!=nDim_field) {
        ERROR("Dimension of cell_length ("<< cell_length.size() << ") != " << nDim_field << " for geometry " << geometry);
    }
    res_space.resize(nDim_field);
    for (unsigned int i=0;i<nDim_field;i++){
        res_space[i] = 1.0/cell_length[i];
    }
    
    
    // simulation duration & length
    PyTools::extract("simulation_time", simulation_time, "Main");
    
    PyTools::extract("grid_length",grid_length, "Main");
    if (grid_length.size()!=nDim_field) {
        ERROR("Dimension of grid_length ("<< grid_length.size() << ") != " << nDim_field << " for geometry " << geometry);
    }
    
    
    //! Boundary conditions for ElectroMagnetic Fields
    if( !PyTools::extract("EM_boundary_conditions", EM_BCs, "Main")  )
        ERROR("Electromagnetic boundary conditions (EM_boundary_conditions) not defined" );
    
    if( EM_BCs.size() == 0 ) {
        ERROR("EM_boundary_conditions cannot be empty");
    } else if( EM_BCs.size() == 1 ) {
        while( EM_BCs.size() < nDim_field ) EM_BCs.push_back( EM_BCs[0] );
    } else if( EM_BCs.size() != nDim_field ) {
        ERROR("EM_boundary_conditions must be the same size as the number of dimensions");
    }
    
    for( unsigned int iDim=0; iDim<nDim_field; iDim++ ) {
        if( EM_BCs[iDim].size() == 1 ) // if just one type is specified, then take the same bc type in a given dimension
            EM_BCs[iDim].push_back( EM_BCs[iDim][0] );
        else if ( (EM_BCs[iDim][0] != EM_BCs[iDim][1]) &&  (EM_BCs[iDim][0] == "periodic" || EM_BCs[iDim][1] == "periodic") )
            ERROR("EM_boundary_conditions along "<<"xyz"[iDim]<<" cannot be periodic only on one side");
    }
    
    // -----------------------------------
    // MAXWELL SOLVERS & FILTERING OPTIONS
    // -----------------------------------
    
    time_fields_frozen=0.0;
    PyTools::extract("time_fields_frozen", time_fields_frozen, "Main");
    
    // Poisson Solver
    PyTools::extract("solve_poisson", solve_poisson, "Main");
    PyTools::extract("poisson_max_iteration", poisson_max_iteration, "Main");
    PyTools::extract("poisson_max_error", poisson_max_error, "Main");
    
    // PXR parameters
    PyTools::extract("is_spectral", is_spectral, "Main");
    PyTools::extract("is_pxr", is_pxr, "Main");
    
    // Maxwell Solver
    PyTools::extract("maxwell_solver", maxwell_sol, "Main");
    
    // Current filter properties
    currentFilter_passes = 0;
    int nCurrentFilter = PyTools::nComponents("CurrentFilter");
    for (int ifilt = 0; ifilt < nCurrentFilter; ifilt++) {
        string model;
        PyTools::extract("model", model, "CurrentFilter", ifilt);
        if( model != "binomial" )
            ERROR("Currently, only the `binomial` model is available in CurrentFilter()");
        PyTools::extract("passes", currentFilter_passes, "CurrentFilter", ifilt);
    }
    
    // Field filter properties
    Friedman_filter = false;
    Friedman_theta = 0;
    int nFieldFilter = PyTools::nComponents("FieldFilter");
    for (int ifilt = 0; ifilt < nFieldFilter; ifilt++) {
        string model;
        PyTools::extract("model", model, "FieldFilter", ifilt);
        if( model != "Friedman" )
            ERROR("Currently, only the `Friedman` model is available in FieldFilter()");
        if( geometry != "2Dcartesian" )
            ERROR("Currently, the `Friedman` field filter is only availble in `2Dcartesian` geometry");
        Friedman_filter = true;
        PyTools::extract("theta", Friedman_theta, "FieldFilter", ifilt);
        if ( Friedman_filter && (Friedman_theta==0.) )
            WARNING("Friedman filter is applied but parameter theta is set to zero");
        if ( (Friedman_theta<0.) || (Friedman_theta>1.) )
            ERROR("Friedman filter theta = " << Friedman_theta << " must be between 0 and 1");
    }
    
    
    // testing the CFL condition
    //!\todo (MG) CFL cond. depends on the Maxwell solv. ==> HERE JUST DONE FOR YEE!!!
    double res_space2=0;
    for (unsigned int i=0; i<nDim_field; i++) {
        res_space2 += res_space[i]*res_space[i];
    }
    dtCFL=1.0/sqrt(res_space2);
    if ( timestep>dtCFL ) {
        WARNING("CFL problem: timestep=" << timestep << " should be smaller than " << dtCFL);
    }
    
    
    
    // clrw
    PyTools::extract("clrw",clrw, "Main");
    
    
    
    // --------------------
    // Number of patches
    // --------------------
    if ( !PyTools::extract("number_of_patches", number_of_patches, "Main") ) {
        ERROR("The parameter `number_of_patches` must be defined as a list of integers");
    }
    for ( unsigned int iDim=0 ; iDim<nDim_field ; iDim++ )
        if( (number_of_patches[iDim] & (number_of_patches[iDim]-1)) != 0)
            ERROR("Number of patches in each direction must be a power of 2");
    
    tot_number_of_patches = 1;
    for ( unsigned int iDim=0 ; iDim<nDim_field ; iDim++ )
        tot_number_of_patches *= number_of_patches[iDim];
    
    if ( tot_number_of_patches == (unsigned int)(smpi->getSize()) ){
        one_patch_per_MPI = true;
    } else {
        one_patch_per_MPI = false;
        if (tot_number_of_patches < (unsigned int)(smpi->getSize()))
            ERROR("The total number of patches "<<tot_number_of_patches<<" must be greater or equal to the number of MPI processes "<<smpi->getSize());
    }
#ifdef _OPENMP
    if ( tot_number_of_patches < (unsigned int)(smpi->getSize()*smpi->getOMPMaxThreads()) )
        WARNING( "Resources allocated "<<(smpi->getSize()*smpi->getOMPMaxThreads())<<" underloaded regarding the total number of patches "<<tot_number_of_patches );
#endif

    global_factor.resize( nDim_field, 1 );
    PyTools::extract( "global_factor", global_factor, "Main" );
    norder.resize(nDim_field,1);
    norder.resize(nDim_field,1);
    PyTools::extract( "norder", norder, "Main" ); 
    //norderx=norder[0];
    //nordery=norder[1];
    //norderz=norder[2];


    if( PyTools::nComponents("LoadBalancing")>0 ) {
        // get parameter "every" which describes a timestep selection
        load_balancing_time_selection = new TimeSelection(
            PyTools::extract_py("every", "LoadBalancing"), "Load balancing"
        );
        PyTools::extract("cell_load"  , cell_load      , "LoadBalancing");
        PyTools::extract("frozen_particle_load", frozen_particle_load    , "LoadBalancing");
        PyTools::extract("initial_balance", initial_balance    , "LoadBalancing");
    } else {
        load_balancing_time_selection = new TimeSelection();
    }
    has_load_balancing = (smpi->getSize()>1)  && (! load_balancing_time_selection->isEmpty());
    
    //mi.resize(nDim_field, 0);
    mi.resize(3, 0);
    while ((number_of_patches[0] >> mi[0]) >1) mi[0]++ ;
    if (number_of_patches.size()>1) {
        while ((number_of_patches[1] >> mi[1]) >1) mi[1]++ ;
        if (number_of_patches.size()>2)
            while ((number_of_patches[2] >> mi[2]) >1) mi[2]++ ;
    }
    
    // Read the "print_every" parameter
    print_every = (int)(simulation_time/timestep)/10;
    PyTools::extract("print_every", print_every, "Main");
    if (!print_every) print_every = 1;
    
    // Read the "print_expected_disk_usage" parameter
    if( ! PyTools::extract("print_expected_disk_usage", print_expected_disk_usage, "Main") ) {
        ERROR("The parameter `Main.print_expected_disk_usage` must be True or False");
    }
    
    // -------------------------------------------------------
    // Checking species order
    // -------------------------------------------------------
    // read from python namelist the number of species
    unsigned int tot_species_number = PyTools::nComponents("Species");
    
    double mass, mass2=0;
    
    for (unsigned int ispec = 0; ispec < tot_species_number; ispec++)
    {
        PyTools::extract("mass", mass ,"Species",ispec);
        if (mass == 0)
        {
            for (unsigned int ispec2 = ispec+1; ispec2 < tot_species_number; ispec2++)
            {
                PyTools::extract("mass", mass2 ,"Species",ispec2);
                if (mass2 > 0)
                {
                    ERROR("the photon species (mass==0) should be defined after the particle species (mass>0)");
                }
            }
        }
    }
    
    // -------------------------------------------------------
    // Parameters for the synchrotron-like radiation losses
    // -------------------------------------------------------
    hasMCRadiation = false ;// Default value
    hasLLRadiation = false ;// Default value
    hasNielRadiation = false ;// Default value
    
    
    // Loop over all species to check if the radiation losses are activated
    std::string radiation_model = "none";
    for (unsigned int ispec = 0; ispec < tot_species_number; ispec++) {
    
       PyTools::extract("radiation_model", radiation_model ,"Species",ispec);
    
       if (radiation_model=="Monte-Carlo")
       {
           this->hasMCRadiation = true;
       }
       else if (radiation_model=="Landau-Lifshitz"
            || radiation_model=="corrected-Landau-Lifshitz")
       {
           this->hasLLRadiation = true;
       }
       else if (radiation_model=="Niel")
       {
           this->hasNielRadiation = true;
       }
    }
    
    // -------------------------------------------------------
    // Parameters for the mutliphoton Breit-Wheeler pair decay
    // -------------------------------------------------------
    this->hasMultiphotonBreitWheeler = false ;// Default value
    
    std::vector<std::string> multiphoton_Breit_Wheeler(2);
    for (unsigned int ispec = 0; ispec < tot_species_number; ispec++) {
    
        if (PyTools::extract("multiphoton_Breit_Wheeler", multiphoton_Breit_Wheeler ,"Species",ispec))
        {
            this->hasMultiphotonBreitWheeler = true;
        }
    }
    
    // -------------------------------------------------------
    // Compute useful quantities and introduce normalizations
    // also defines defaults values for the species lengths
    // -------------------------------------------------------
    compute();
    
    // Print
    smpi->barrier();
    if ( smpi->isMaster() ) print_init();
    smpi->barrier();
}

Params::~Params() {
    if( load_balancing_time_selection ) delete load_balancing_time_selection;
    PyTools::closePython();
}

// ---------------------------------------------------------------------------------------------------------------------
// Compute useful values (normalisation, time/space step, etc...)
// ---------------------------------------------------------------------------------------------------------------------
void Params::compute()
{
    // time-related parameters
    // -----------------------

    // number of time-steps
    n_time   = (int)(simulation_time/timestep);

    // simulation time & time-step value
    double entered_simulation_time = simulation_time;
    simulation_time = (double)(n_time) * timestep;
    if (simulation_time!=entered_simulation_time)
        WARNING("simulation_time has been redefined from " << entered_simulation_time
        << " to " << simulation_time << " to match nxtimestep ("
        << scientific << setprecision(4) << simulation_time - entered_simulation_time<< ")" );


    // grid/cell-related parameters
    // ----------------------------
    n_space.resize(3);
    cell_length.resize(3);
    cell_volume=1.0;
    if (nDim_field==res_space.size() && nDim_field==grid_length.size()) {

        // compute number of cells & normalized lengths
        for (unsigned int i=0; i<nDim_field; i++) {
            n_space[i]         = round(grid_length[i]/cell_length[i]);

            double entered_grid_length = grid_length[i];
            grid_length[i]      = (double)(n_space[i])*cell_length[i]; // ensure that nspace = grid_length/cell_length
            if (grid_length[i]!=entered_grid_length)
                WARNING("grid_length[" << i << "] has been redefined from " << entered_grid_length << " to " << grid_length[i] << " to match n x cell_length (" << scientific << setprecision(4) << grid_length[i]-entered_grid_length <<")");
            cell_volume   *= cell_length[i];
        }
        // create a 3d equivalent of n_space & cell_length
        for (unsigned int i=nDim_field; i<3; i++) {
            n_space[i]=1;
            cell_length[i]=0.0;
        }

    } else {
        ERROR("Problem with the definition of nDim_field");
    }

    //!\todo (MG to JD) Are these 2 lines really necessary ? It seems to me it has just been done before
    n_space.resize(3, 1);
    cell_length.resize(3, 0.);            //! \todo{3 but not real size !!! Pbs in Species::Species}
    n_space_global.resize(3, 1);        //! \todo{3 but not real size !!! Pbs in Species::Species}
    oversize.resize(3, 0);
    patch_dimensions.resize(3, 0.);
    
    //n_space_global.resize(nDim_field, 0);
    n_cell_per_patch = 1;
    for (unsigned int i=0; i<nDim_field; i++){
        oversize[i]  = interpolation_order + (exchange_particles_each-1);;
        n_space_global[i] = n_space[i];
        n_space[i] /= number_of_patches[i];
        if(n_space_global[i]%number_of_patches[i] !=0) ERROR("ERROR in dimension " << i <<". Number of patches = " << number_of_patches[i] << " must divide n_space_global = " << n_space_global[i]);
        if ( n_space[i] <= 2*oversize[i]+1 ) ERROR ( "ERROR in dimension " << i <<". Patches length = "<<n_space[i] << " cells must be at least " << 2*oversize[i] +2 << " cells long. Increase number of cells or reduce number of patches in this direction. " );
        patch_dimensions[i] = n_space[i] * cell_length[i];
        n_cell_per_patch *= n_space[i];
    }
    
    // Set clrw if not set by the user
    if ( clrw == -1 ) {

        // default value
        clrw = n_space[0];

        // check cache issue for interpolation/projection
        int cache_threshold( 3200 ); // sizeof( L2, Sandy Bridge-HASWELL ) / ( 10 * sizeof(double) )
        // Compute the "transversal bin size"
        int bin_size(1);
        for ( unsigned int idim = 1 ; idim < nDim_field ; idim++ )
            bin_size *= ( n_space[idim]+1+2*oversize[idim] );

        // IF Ionize r pair generation : clrw = n_space_x_pp ?
        if ( ( clrw+1+2*oversize[0]) * bin_size > (unsigned int) cache_threshold ) {
            int clrw_max = cache_threshold / bin_size - 1 - 2*oversize[0];
            if ( clrw_max > 0 ) {
                for ( clrw=clrw_max ; clrw > 0 ; clrw-- )
                    if ( ( ( clrw+1+2*oversize[0]) * bin_size <= (unsigned int) cache_threshold ) && (n_space[0]%clrw==0) ) {
                        break;
                    }
            }
            else
                clrw = 1;
            WARNING( "Particles cluster width set to : " << clrw );
        }
    }

    // Verify that clrw divides n_space[0]
    if( n_space[0]%clrw != 0 )
        ERROR("The parameter clrw must divide the number of cells in one patch (in dimension x)");

}


// ---------------------------------------------------------------------------------------------------------------------
// Set dimensions according to geometry
// ---------------------------------------------------------------------------------------------------------------------
void Params::setDimensions()
{
    if (geometry=="1Dcartesian") {
        nDim_particle=1;
        nDim_field=1;
    } else if (geometry=="2Dcartesian") {
        nDim_particle=2;
        nDim_field=2;
    } else if (geometry=="3Dcartesian") {
        nDim_particle=3;
        nDim_field=3;
    } else if (geometry=="3drz") {
        nDim_particle=3;
        nDim_field=2;
    } else {
        ERROR("Geometry: " << geometry << " not defined");
    }
}



// ---------------------------------------------------------------------------------------------------------------------
// Printing out the data at initialisation
// ---------------------------------------------------------------------------------------------------------------------
void Params::print_init()
{
    TITLE("Geometry: " << geometry);
    MESSAGE(1,"Interpolation order : " <<  interpolation_order);
    MESSAGE(1,"Maxwell solver : " <<  maxwell_sol);
    MESSAGE(1,"(Time resolution, Total simulation time) : (" << res_time << ", " << simulation_time << ")");
    MESSAGE(1,"(Total number of iterations,   timestep) : (" << n_time << ", " << timestep << ")");
    MESSAGE(1,"           timestep  = " << timestep/dtCFL << " * CFL");

    for ( unsigned int i=0 ; i<grid_length.size() ; i++ ){
        MESSAGE(1,"dimension " << i << " - (Spatial resolution, Grid length) : (" << res_space[i] << ", " << grid_length[i] << ")");
        MESSAGE(1,"            - (Number of cells,    Cell length) : " << "(" << n_space_global[i] << ", " << cell_length[i] << ")");
    }

    if( currentFilter_passes > 0 )
        MESSAGE(1, "Binomial current filtering : "<< currentFilter_passes << " passes");
    if( Friedman_filter )
        MESSAGE(1, "Friedman field filtering : theta = " << Friedman_theta);

    if (has_load_balancing){
        TITLE("Load Balancing: ");
        if (initial_balance){
            MESSAGE(1,"Computational load is initially balanced between MPI ranks. (initial_balance = true) ");
        } else{
            MESSAGE(1,"Patches are initially homogeneously distributed between MPI ranks. (initial_balance = false) ");
        }
        MESSAGE(1,"Happens: " << load_balancing_time_selection->info());
        MESSAGE(1,"Cell load coefficient = " << cell_load );
        MESSAGE(1,"Frozen particle load coefficient = " << frozen_particle_load );
    }
}

// ---------------------------------------------------------------------------------------------------------------------
// Printing out some data at a given timestep
// ---------------------------------------------------------------------------------------------------------------------
void Params::print_timestep(unsigned int itime, double time_dual, Timer & timer)
{
    double before = timer.getTime();
    timer.update();
    double now = timer.getTime();
    ostringstream my_msg;
    my_msg << "  " << setw(timestep_width) << itime << "/" << n_time << " "
           << "  " << scientific << setprecision(4) << setw(12) << time_dual << " "
           << "  " << scientific << setprecision(4) << setw(12) << now << " "
           << "  " << "(" << scientific << setprecision(4) << setw(12) << now - before << " )"
           ;
    #pragma omp master
    MESSAGE(my_msg.str());
    #pragma omp barrier
}

void Params::print_timestep_headers()
{
    timestep_width = log10(n_time) + 1;
    if( timestep_width<3 ) timestep_width = 3;
    ostringstream my_msg;
    my_msg << setw(timestep_width*2+4) << " timestep "
           << setw(15) << "sim time "
           << setw(15) << "cpu time [s] "
           << "  (" << setw(12) << "diff [s]" << " )"
           ;
    MESSAGE(my_msg.str());
}


// Print information about the MPI aspects
void Params::print_parallelism_params(SmileiMPI* smpi)
{
    if (smpi->isMaster()) {
        MESSAGE(1,"Number of MPI process : " << smpi->getSize() );
        MESSAGE(1,"Number of patches : " );
        for (unsigned int iDim=0 ; iDim<nDim_field ; iDim++)
            MESSAGE(2, "dimension " << iDim << " - number_of_patches : " << number_of_patches[iDim] );

        MESSAGE(1, "Patch size :");
        for (unsigned int iDim=0 ; iDim<nDim_field ; iDim++)
            MESSAGE(2, "dimension " << iDim << " - n_space : " << n_space[iDim] << " cells.");

        MESSAGE(1, "Dynamic load balancing: " << load_balancing_time_selection->info() );
    }

    if (smpi->isMaster()) {
       TITLE("OpenMP");
#ifdef _OPENMP
//    int nthds(0);
//#pragma omp parallel shared(nthds)
//    {
//        nthds = omp_get_num_threads();
//    }
        MESSAGE(1,"Number of thread per MPI process : " << smpi->getOMPMaxThreads() );
#else
        MESSAGE("Disabled");
#endif
    }
}

// ---------------------------------------------------------------------------------------------------------------------
// Finds requested species in the list of existing species.
// Returns an array of the numbers of the requested species.
// Note that there might be several species that have the same "name" or "type"
//  so that we have to search for all possibilities.
vector<unsigned int> Params::FindSpecies(vector<Species*>& vecSpecies, vector<string> requested_species)
{
    bool species_found;
    vector<unsigned int> result;
    unsigned int i;
    vector<string> existing_species;

    // Make an array of the existing species names
    existing_species.resize(0);
    for (unsigned int ispec=0 ; ispec<vecSpecies.size() ; ispec++) {
        existing_species.push_back( vecSpecies[ispec]->name );
    }

    // Loop over group of requested species
    for (unsigned int rs=0 ; rs<requested_species.size() ; rs++) {
        species_found = false;
        // Loop over existing species
        for (unsigned int es=0 ; es<existing_species.size() ; es++) {
            if (requested_species[rs] == existing_species[es]) { // if found
                species_found = true;
                // Add to the list and sort
                for (i=0 ; i<result.size() ; i++) {
                    if (es == result[i]) break; // skip if duplicate
                    if (es <  result[i]) {
                        result.insert(result.begin()+i,es); // insert at the right place
                        break;
                    }
                }
                // Put at the end if not put earlier
                if (i == result.size()) result.push_back(es);
            }
        }
        if (!species_found)
            ERROR("Species `" << requested_species[rs] << "` was not found.");
    }

    return result;
}


//! Run string as python script and add to namelist
void Params::runScript(string command, string name) {
    PyTools::checkPyError();
    namelist+=command;
    if (name.size()>0)  MESSAGE(1,"Parsing " << name);
    int retval=PyRun_SimpleString(command.c_str());
    if (retval==-1) {
        ERROR("error parsing "<< name);
        PyTools::checkPyError();
    }
}

//! run the python functions cleanup (user defined) and _keep_python_running (in pycontrol.py)
void Params::cleanup(SmileiMPI* smpi) {
    // call cleanup function from the user namelist (it can be used to free some memory
    // from the python side) while keeping the interpreter running
    MESSAGE(1,"Checking for cleanup() function:");
    PyTools::runPyFunction("cleanup");
    // this will reset error in python in case cleanup doesn't exists
    PyErr_Clear();

    smpi->barrier();

    // this function is defined in the Python/pyontrol.py file and should return false if we can close
    // the python interpreter
    MESSAGE(1,"Calling python _keep_python_running() :");
    if (PyTools::runPyFunction<bool>("_keep_python_running")) {
        MESSAGE(2,"Keeping Python interpreter alive");
    } else {
        MESSAGE(2,"Closing Python");
        PyErr_Print();
        Py_Finalize();
    }
    smpi->barrier();
}
