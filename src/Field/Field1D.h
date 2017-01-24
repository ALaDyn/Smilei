#ifndef FIELD1D_H
#define FIELD1D_H

#include <cmath>
#include <vector>

#include "Field.h"
#include "Tools.h"

//! class Field1D used to defined a 1d vector
class Field1D : public Field
{

public:
    //! Constructor for Field1D: with no input argument
    Field1D();

    //! Constructor for Field1D: with the vector dimension as input argument
    Field1D( std::vector<unsigned int> dims );
    
    //! Constructor, isPrimal define if mainDim is Primal or Dual
    Field1D( std::vector<unsigned int> dims, unsigned int mainDim, bool isPrimal );

    //! Constructor for Field1D: with the vector dimension and filename for the dump as input argument
    Field1D( std::vector<unsigned int> dims, std::string name );
    //! Constructor, isPrimal define if mainDim is Primal or Dual and a name
    Field1D( std::vector<unsigned int> dims, unsigned int mainDim, bool isPrimal, std::string name );

    //! Constructor, without allocating
    Field1D( std::string name, std::vector<unsigned int> dims );

    //! Destructor for Field1D
    ~Field1D();

    //! Method used to allocate a Field1D
    void allocateDims();
    void deallocateDims();
    //! a Field1D can also be initialized win an unsigned int 
    void allocateDims(unsigned int dims1);
    //! 1D method used to allocate Field, isPrimal define if mainDim is Primal or Dual
    void allocateDims(unsigned int mainDim, bool isPrimal);
    
    inline void allocateDims(std::vector<unsigned int> dims) {
        dims_ = dims;
        allocateDims();
    };
    
    inline void allocateDims(std::vector<unsigned int> dims, unsigned int mainDim, bool isPrimal) {
        dims_ = dims;
        allocateDims(mainDim, isPrimal);
    };
    
    //! Method used to dump the data contained in a Field1D
    void dump(std::vector<unsigned int> dims);
    
    //! Method to shift field in space
    void shift_x(unsigned int delta);
    
    //! Overloading of the () operator allowing to set a new value for the ith element of a Field1D
    inline double& operator () (unsigned int i)
    {
        DEBUGEXEC(if (i>=dims_[0]) ERROR(name << "Out of limits & "<< i));
        DEBUGEXEC(if (!std::isfinite(data_[i])) ERROR(name << " not finite at i=" << i << " = " << data_[i]));
        return data_[i];
    };
    
    //! Overloading of the () operator allowing to get the value of the ith element of a Field1D
    inline double operator () (unsigned int i) const
    {
        DEBUGEXEC(if (i>=dims_[0]) ERROR(name << "Out of limits "<< i));
        DEBUGEXEC(if (!std::isfinite(data_[i])) ERROR(name << "Not finite "<< i << " = " << data_[i]));
        return data_[i];
    };
    
    //! \todo What is this? (MG)
    //! \todo private/friend/modify (JD)
    // 
    //! Now in Field, all arrays may be viewed as a 1D array
    //double* data_;
    
    
    virtual double norm2(unsigned int istart[3][2], unsigned int bufsize[3][2]);
private:
};


#endif
