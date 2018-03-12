
#ifndef SYNCCARTESIANPATCH_H
#define SYNCCARTESIANPATCH_H

class Domain;
class VectorPatch;
class Patch;
class Params;
class SmileiMPI;
class Timers;
class Field;
class ElectroMagn;

class SyncCartesianPatch {
public :

    static void patchedToCartesian( VectorPatch& vecPatches, Domain& domain, Params &params, SmileiMPI* smpi, Timers &timers, int itime );
    static void cartesianToPatches( Domain& domain, VectorPatch& vecPatches, Params &params, SmileiMPI* smpi, Timers &timers, int itime );

    static void sendPatchedToCartesian( ElectroMagn* localfields, unsigned int hindex, int send_to_global_patch_rank, SmileiMPI * smpi, Patch *patch );
    static void finalize_sendPatchedToCartesian( ElectroMagn* localfields, unsigned int hindex, int send_to_global_patch_rank, SmileiMPI* smpi, Patch *patch );
    static void recvPatchedToCartesian( ElectroMagn* globalfields, unsigned int hindex, int local_patch_rank, VectorPatch& vecPatches, Params &params, SmileiMPI* smpi, Domain& domain );

    static void recvCartesianToPatches( ElectroMagn* localfields, unsigned int hindex, int recv_from_global_patch_rank, SmileiMPI* smpi, Patch* patch );
    static void finalize_recvCartesianToPatches( ElectroMagn* localfields, unsigned int hindex, int recv_from_global_patch_rank, SmileiMPI* smpi, Patch* patch );
    static void sendCartesianToPatches( ElectroMagn* globalfields, unsigned int hindex, int local_patch_rank, VectorPatch& vecPatches, Params &params, SmileiMPI* smpi, Domain& domain );


};

#endif
