#ifndef SOLVERRZ_H
#define SOLVERRZ_H

#include "Solver.h"

//  --------------------------------------------------------------------------------------------------------------------
//! Class SolverRZ
//  --------------------------------------------------------------------------------------------------------------------
class SolverRZ : public Solver
{

public:
    //! Creator for Solver
    SolverRZ(Params &params) : Solver(params) {
	nl_p = params.n_space[0]+1+2*params.oversize[0];
	nl_d = params.n_space[0]+2+2*params.oversize[0];
	nr_p = params.n_space[1]+1+2*params.oversize[1];
	nr_d = params.n_space[1]+2+2*params.oversize[1];
	Nmode= params.Nmode;
    dt = params.timestep;
	dr = params.cell_length[1];
	dt_ov_dl = params.timestep / params.cell_length[0];
	dt_ov_dr = params.timestep / params.cell_length[1];

    };
    virtual ~SolverRZ() {};

    //! Overloading of () operator
    virtual void operator()( ElectroMagn* fields) = 0;

protected:
    unsigned int nl_p;
    unsigned int nl_d;
    unsigned int nr_p;
    unsigned int nr_d;
	unsigned int Nmode;
    double dt;
	double dr;
    double dt_ov_dl;
    double dt_ov_dr;

};//END class

#endif

