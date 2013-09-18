/*
 * SmileiIO.cpp
 *
 *  Created on: 3 juil. 2013
 *      Author: jderouil
 */

#include "SmileiIO.h"

#include "PicParams.h"
#include "SmileiMPI.h"
#include "ElectroMagn.h"
#include "Species.h"

#include <sstream>

using namespace std;

SmileiIO::SmileiIO( PicParams* params, SmileiMPI* smpi )
{
	ostringstream name;
	name.str(""); name << "particles-" << smpi->getRank() << ".h5" ;

	hid_t attribute_id;

	// Create 1 file containing 1 dataset per Species
	partFile_id = H5Fcreate( name.str().c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

	particleSize = params->nDim_particle + 3 + 1;
	hsize_t dims[2] = {0, particleSize};
	hsize_t max_dims[2] = {H5S_UNLIMITED, particleSize};
	hid_t file_space = H5Screate_simple(2, dims, max_dims);

	hid_t plist = H5Pcreate(H5P_DATASET_CREATE);
	H5Pset_layout(plist, H5D_CHUNKED);
	hsize_t chunk_dims[2] = {1, particleSize};
	H5Pset_chunk(plist, 2, chunk_dims);

	nDatasetSpecies = params->n_species;
	partDataset_id = new hid_t[nDatasetSpecies];
	for (unsigned int ispec=0 ; ispec<nDatasetSpecies ; ispec++) {
		ostringstream speciesName;
		speciesName.str(""); speciesName << params->species_param[ispec].species_type;
		partDataset_id[ispec] = H5Dcreate(partFile_id, speciesName.str().c_str(), H5T_NATIVE_FLOAT, file_space, H5P_DEFAULT, plist, H5P_DEFAULT);

		hsize_t scalaire = 1;
		hid_t tmp_space = H5Screate_simple(1, &scalaire, NULL);

		attribute_id = H5Acreate2 (partDataset_id[ispec], "Mass", H5T_IEEE_F64BE, tmp_space,
					   	   	   	   	   	 H5P_DEFAULT, H5P_DEFAULT);
		H5Awrite(attribute_id, H5T_NATIVE_DOUBLE, &params->species_param[ispec].mass);
		H5Aclose(attribute_id);

		attribute_id = H5Acreate2 (partDataset_id[ispec], "Charge", H5T_IEEE_F64BE, tmp_space,
					   	   	   	   	   	 H5P_DEFAULT, H5P_DEFAULT);
		H5Awrite(attribute_id, H5T_NATIVE_DOUBLE, &params->species_param[ispec].charge);
		H5Aclose(attribute_id);

		H5Sclose(tmp_space);
	}

	H5Pclose(plist);
	H5Sclose(file_space);

	dims[0] = 1;
	dims[1] = particleSize;
	partMemSpace = H5Screate_simple(2, dims, NULL);

}

SmileiIO::~SmileiIO()
{
	H5Sclose(partMemSpace);
	for ( unsigned int s=0 ; s<nDatasetSpecies ; s++ )
		H5Dclose(partDataset_id[s]);
	delete partDataset_id;
	H5Fclose(partFile_id);

}

void SmileiIO::writeFields( ElectroMagn* EMfields, double time )
{
	// File opened in constructor
	write( EMfields->Ex_, "ex.h5", time );
	write( EMfields->Ey_, "ey.h5", time );
	write( EMfields->Ez_, "ez.h5", time );
	write( EMfields->Bx_, "bx.h5", time );
	write( EMfields->By_, "by.h5", time );
	write( EMfields->Bz_, "bz.h5", time );
	write( EMfields->Jx_, "jx.h5", time );
	write( EMfields->Jy_, "jy.h5", time );
	write( EMfields->Jz_, "jz.h5", time );
	write( EMfields->rho_, "rho.h5", time );
}


void SmileiIO::writePlasma( vector<Species*> vecSpecies, double time, SmileiMPI* smpi )
{
	int n_species = vecSpecies.size();
	for (int ispec=0 ; ispec<n_species ; ispec++) {
		std::vector<Particle*>* cuParticles = &(vecSpecies[ispec])->particles;
		cout << "write species " << ispec << endl;

		for (unsigned int p=0; p<(vecSpecies[ispec])->getNbrOfParticles(); p++ ) {

			hid_t file_space = H5Dget_space(partDataset_id[ispec]);
			hsize_t dimsO[2];
			H5Sget_simple_extent_dims(file_space, dimsO, NULL);
			H5Sclose(file_space);
			hsize_t dims[2];
			dims[0] = dimsO[0]+1;
			dims[1] = dimsO[1];
			H5Dset_extent(partDataset_id[ispec], dims);

			file_space = H5Dget_space(partDataset_id[ispec]);
			hsize_t start[2];
			hsize_t count[2] = {1, particleSize};
			start[0] = dimsO[0];
			start[1] = 0;
			H5Sselect_hyperslab(file_space, H5S_SELECT_SET, start, NULL, count, NULL);

			H5Dwrite(partDataset_id[ispec], H5T_NATIVE_FLOAT, partMemSpace, file_space, H5P_DEFAULT, &((*cuParticles)[ p ]->position(0)));

			H5Sclose(file_space);


		} // End for p

	} // End for ispec

}
