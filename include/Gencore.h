#ifndef __GENESIS_GENCORE__
#define __GENESIS_GENCORE__

#include <iostream>
#include <vector>
#include <math.h>
#include <string>
#include <fstream>
#include <iomanip>
#include <stdio.h>
#include <cstring>

#include <mpi.h>

// genesis headerfiles & classes

#include "Beam.h"
#include "Field.h"
#include "Undulator.h"
#include "Control.h"
#include "Diagnostic.h"
#include "Setup.h"

//extern const double vacimp;
//extern const double eev;

using namespace std;

class Gencore{
 public:
  Gencore(){};
  virtual ~Gencore(){};
  int run(const char *,Beam *, vector<Field *> *, Setup *, Undulator *, bool, bool, FilterDiagnostics &filter);
};


#endif
