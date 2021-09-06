#ifndef __GENESIS_OUTPUT__
#define __GENESIS_OUTPUT__

#include <vector>
#include <iostream>
#include <string>
#include <math.h>
#include <hdf5.h>

#include "HDF5base.h"
#include "Particle.h"
#include "Beam.h"
#include "Field.h"
#include "Undulator.h"

using namespace std;

extern const double vacimp;
extern const double eev;

extern string meta_inputfile;
extern string meta_latfile;

class Output : public HDF5Base {
 public:
   Output();
   virtual ~Output();
   void open(string,int,int);
   void close();
   void writeFieldBuffer(Field *);
   void writeBeamBuffer(Beam *);
   void writeLattice(Beam *, Undulator *);
   void writeGlobal(Undulator *,double,double,double,double,bool,bool,bool,int);
   void writeMeta(Undulator *);
   void reportDumps(hid_t, Undulator *);

 private:
   void write(hsize_t,string,string,double *);
   bool noOutput;
   hid_t fid;   
};


#endif
