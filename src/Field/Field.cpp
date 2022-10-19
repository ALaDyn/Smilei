#include "Field.h"

#include "gpu.h"

void Field::put_to( double val )
{
    const bool is_hostptr_mapped_on_device = smilei::tools::gpu::HostDeviceMemoryManagement::IsHostPointerMappedOnDevice( data_ );
    SMILEI_UNUSED( is_hostptr_mapped_on_device );

    if( data_ != nullptr ) {
        // OpenACC needs that redundant pointer value
        double* an_other_data_pointer = data_;
#if defined( _GPU )
    // Test if data exists on GPU, put_to can be used on CPU and GPU during a simulation
    #pragma acc parallel         present( an_other_data_pointer [0:globalDims_] ) if( is_hostptr_mapped_on_device )
    #pragma acc loop gang worker vector
#elif defined( SMILEI_ACCELERATOR_GPU_OMP )
    #pragma omp target if( is_hostptr_mapped_on_device )
    #pragma omp teams
    #pragma omp distribute parallel for
#endif
        for( unsigned int i = 0; i < globalDims_; i++ ) {
            an_other_data_pointer[i] = val;
        }
    }
}