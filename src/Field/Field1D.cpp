#include "Field1D.h"

#include <iostream>
#include <vector>
#include <cstring>

#include "Params.h"
#include "SmileiMPI.h"
#include "Patch.h"

using namespace std;


// ---------------------------------------------------------------------------------------------------------------------
// Creators for Field1D
// ---------------------------------------------------------------------------------------------------------------------

// with no input argument
Field1D::Field1D() : Field()
{
}

// with the dimensions as input argument
Field1D::Field1D(vector<unsigned int> dims) : Field(dims)
{
    allocateDims(dims);
}

// with the dimensions and output (dump) file name as input argument
Field1D::Field1D(vector<unsigned int> dims, string name) : Field(dims, name)
{
    allocateDims(dims);
}

// with the dimensions as input argument
Field1D::Field1D(vector<unsigned int> dims, unsigned int mainDim, bool isPrimal) : Field(dims, mainDim, isPrimal)
{
    allocateDims(dims, mainDim, isPrimal);
}

// with the dimensions and output (dump) file name as input argument
Field1D::Field1D(vector<unsigned int> dims, unsigned int mainDim, bool isPrimal, string name) : Field(dims, mainDim, isPrimal, name)
{
    allocateDims(dims, mainDim, isPrimal);
}

// without allocating
Field1D::Field1D(string name, vector<unsigned int> dims) : Field(dims, name)
{
    data_=NULL;
    dims_ = dims;
}


// ---------------------------------------------------------------------------------------------------------------------
// Destructor for Field1D
// ---------------------------------------------------------------------------------------------------------------------
Field1D::~Field1D()
{
    if (data_!=NULL) {
        delete [] data_;
    }
}


// ---------------------------------------------------------------------------------------------------------------------
// Method used for allocating the dimension of a Field1D
// ---------------------------------------------------------------------------------------------------------------------
void Field1D::allocateDims()
{
    // for Field1D only
    if (dims_.size()!=1) ERROR("Alloc error must be 1 : " << dims_.size());
    
    isDual_.resize( dims_.size(), 0 );
    
    data_ = new double[ dims_[0] ];
    //! \todo{change to memset (JD)}
    for (unsigned int i=0; i<dims_[0]; i++) data_[i]=0.0;
    
    globalDims_ = dims_[0];

}

void Field1D::deallocateDims()
{
    delete [] data_;
    data_=NULL;
}


void Field1D::allocateDims(unsigned int dims1)
{
    vector<unsigned int> dims(1);
    dims[0]=dims1;
    allocateDims(dims);
}



// ---------------------------------------------------------------------------------------------------------------------
// Method used for allocating the dimension of a Field1D
// ---------------------------------------------------------------------------------------------------------------------
void Field1D::allocateDims(unsigned int mainDim, bool isPrimal)
{
    // for Field1D only
    if (dims_.size()!=1) ERROR("Alloc error must be 1 : " << dims_.size());
    
    // isPrimal define if mainDim is Primal or Dual
    isDual_.resize( dims_.size(), 0 );
    for ( unsigned int j=0 ; j<dims_.size() ; j++ ) {
        if ( (j==mainDim) && (!isPrimal) )
            isDual_[j] = 1;
        else if ( (j!=mainDim) && (isPrimal) )
            isDual_[j] = 1;
    }
    
    for ( unsigned int j=0 ; j<dims_.size() ; j++ )
        dims_[j] += isDual_[j];
    
    data_ = new double[ dims_[0] ];
    //! \todo{change to memset (JD)}
    for (unsigned int i=0; i<dims_[0]; i++) data_[i]=0.0;
    
    globalDims_ = dims_[0];

}


// ---------------------------------------------------------------------------------------------------------------------
// Method used to create a dump for a Field1D
// ---------------------------------------------------------------------------------------------------------------------
void Field1D::dump(vector<unsigned int> dims)
{
}


// ---------------------------------------------------------------------------------------------------------------------
// Method to shift field in space
// ---------------------------------------------------------------------------------------------------------------------
void Field1D::shift_x(unsigned int delta)
{
    memmove( &(data_[0]), &(data_[delta]), (dims_[0]-delta)*sizeof(double) );
    //memset ( &(data_[dims_[0]-delta]), 0, delta*sizeof(double));
    for (int i=dims_[0]-delta;i<(int)dims_[0];i++) data_[i] = 0.;

}

double Field1D::norm2(unsigned int istart[3][2], unsigned int bufsize[3][2]) {
    
    double nrj(0.);
    
    int idxlocalstart[1];
    int idxlocalend[1];
    for ( int i=0 ; i<1 ; i++ ) {
        idxlocalstart[i] = istart[i][isDual_[i]];
        idxlocalend[i]   = istart[i][isDual_[i]]+bufsize[i][isDual_[i]];
    }
    for ( int i=idxlocalstart[0] ; i<idxlocalend[0] ; i++ ) {
        nrj += data_[i]*data_[i];
    }
    
    return nrj;
}


void Field1D::push( Field* outField, Params &params, SmileiMPI* smpi, Patch* thisPatch, Patch* outPatch )
{
    Field1D* out1D = static_cast<Field1D*>( outField );

    std::vector<unsigned int> dual =  this->isDual_;

    int iout = thisPatch->Pcoordinates[0]*params.n_space[0] - outPatch->Pcoordinates[0]*params.n_space[0]*params.global_factor[0] ;
 
    for ( unsigned int i = 0 ; i < this->dims_[0] ; i++ ) {
        ( *out1D )( iout+i ) = ( *this )( i );
    }
   
}


void Field1D::pull( Field* inField, Params &params, SmileiMPI* smpi, Patch* inPatch, Patch* thisPatch )
{
    Field1D* in1D  = static_cast<Field1D*>( inField  );

    std::vector<unsigned int> dual =  in1D->isDual_;

    int iin = thisPatch->Pcoordinates[0]*params.n_space[0] - inPatch->Pcoordinates[0]*params.n_space[0]*params.global_factor[0] ;
 
    for ( unsigned int i = 0 ; i < this->dims_[0] ; i++ ) {
        ( *this )( i ) = ( *in1D )( iin+i );
    }    
   
}
