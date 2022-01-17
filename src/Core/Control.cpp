#include <sstream>
#include <climits>
#include "Control.h"
#include "writeFieldHDF5.h"
#include "writeBeamHDF5.h"




Control::Control()
{
  nwork=0;
  work=NULL;
}


Control::~Control()
{
  delete[] work;
}


bool Control::applyMarker(Beam *beam, vector<Field*>*field, Undulator *und)
{
  bool sort=false;
  int marker=und->getMarker();

  // possible file names contain number of current integration step
  stringstream sroot;
  string basename;
  int istepz=und->getStep();
  sroot << "." << istepz;
  basename=root+sroot.str();


  if ((marker & 1) != 0){
    WriteFieldHDF5 dump;
    dump.write(basename,field);

    /* register field dump => it will be reported in list of dumps generated during current "&track" command */
    string fn;
    fn = basename + ".fld.h5"; /* file extension as added in WriteFieldHDF5::write (TODO: need to implement proper handling of harmonic field dumping) */
    und->fielddumps_filename.push_back(fn);
    und->fielddumps_intstep.push_back(istepz);
  }
  
  if ((marker & 2) != 0){
    WriteBeamHDF5 dump;
    dump.write(basename,beam);

    /* register beam dump => it will be reported in list of dumps generated during current "&track" command */
    string fn;
    fn = basename + ".par.h5"; /* file extension as added in WriteBeamHDF5::write */
    und->beamdumps_filename.push_back(fn);
    und->beamdumps_intstep.push_back(istepz);
  }
  
  if ((marker & 4) != 0){
    sort=true;   // sorting is deferred after the particles have been pushed by Runge-Kutta
  }

  // bit value 8 is checked in und->advance()
  

  return sort;
}


void Control::output(Beam *beam, vector<Field*> *field, Undulator *und, Diagnostic &diag)
{

  
  Output *out=new Output;

  string file=root.append(".out.h5");
  out->open(file,noffset,nslice);
  
  out->writeGlobal(und,und->getGammaRef(),reflen,sample,slen,one4one,timerun,scanrun,ntotal);
  out->writeLattice(beam,und);

  for (unsigned int i=0; i<field->size();i++){
        out->writeFieldBuffer(field->at(i));
  }
  out->writeBeamBuffer(beam);

  out->close();
 
  delete out;
  return;



}


bool Control::init(int inrank, int insize, const char *file, Beam *beam, vector<Field*> *field, Undulator *und, bool inTime, bool inScan)
{

  rank=inrank;
  size=insize;

  stringstream sroot(file);
  root=sroot.str();
  root.resize(root.size()-7);  // remove the extension ".h5"

  one4one=beam->one4one;
  reflen=beam->reflength;
  sample=beam->slicelength/reflen;

  timerun=inTime;
  scanrun=inScan;
 
 

  // cross check simulation size

  nslice=beam->beam.size();
  noffset=rank*nslice;
  ntotal=size*nslice;  // all cores have the same amount of slices

  slen=ntotal*sample*reflen;


  if (rank==0){
    if(scanrun) { 
       cout << "Scan run with " << ntotal << " slices" << endl; 
    } else {
       if(timerun) { 
         cout << "Time-dependent run with " << ntotal << " slices" << " for a time window of " << slen*1e6 << " microns" << endl; 
       } else { 
         cout << "Steady-state run" << endl;
       }
    }
  }
  


  // initial diagnostic

  if (rank==0) { cout << "Initial analysis of electron beam and radiation field..."  << endl; }


  beam->initDiagnostics(und->outlength());
  beam->diagnostics(true,0);
  beam->diagnosticsStart();
  for (unsigned int i=0; i<field->size();i++){
      field->at(i)->initDiagnostics(und->outlength());
      field->at(i)->diagnostics(true);  // initial values
  }	

  return true;  
}



void Control::applySlippage(double slippage, Field *field)
{
  if (timerun==false) { return; }

 
  // update accumulated slippage
  field->accuslip+=slippage;


  // number of grid points in field supplied by caller
  long long ncells = field->ngrid*field->ngrid;

  // if needed, allocate working space for MPI data transfer
  // NOTE: the size of the buffer is determined by the largest field seen so far (relevant when there are multiple fields of different number of grid points)
  if(nwork < 2*ncells){
    delete[] work;
    nwork = 2*ncells; // 1 complex number <=> 2 doubles
    work=new double [nwork];
  } 
  

  // following routine is applied if the required slippage is alrger than 80% of the sampling size

  int direction=1;
  

  while(abs(field->accuslip)>(sample*0.8)){
      // check for anormal direction of slippage (backwards slippage)
      if (field->accuslip<0) {direction=-1;} 

      field->accuslip-=sample*direction; 

      // get adjacent node before and after in chain
      int rank_next=rank+1;
      int rank_prev=rank-1;
      if (rank_next >= size ) { rank_next=0; }
      if (rank_prev < 0 ) { rank_prev = size-1; }	

      // for inverse direction swap targets
      if (direction<0) {
	int tmp=rank_next;
        rank_next=rank_prev;
        rank_prev=tmp; 
      }

      int tag=1;
   
      // get slice which is transmitted
      int last=(field->first+field->field.size()-1)  %  field->field.size();
      // get first slice for inverse direction
      if (direction<0){
	last=(last+1) % field->field.size();  //  this actually first because it is sent backwards
      }

      // Prevent transfer sizes resulting in overflow (MPI_send argument 'count' has data type 'int').
      // For typical transverse grid sizes, this is not a relevant limitation.
      // (All MPI processes have identical transverse field parameters.)
      if(2*ncells > INT_MAX) {
        if(rank==0) {
          cout << "Large field mesh size results in request for MPI transfer size exceeding INT_MAX, exiting." << endl;
        }
        MPI_Abort(MPI_COMM_WORLD,1);
      }

      MPI_Status status;
      //      MPI_Errhandler_set(MPI_COMM_WORLD,MPI_ERRORS_RETURN);
      //      int ierr;
	
      if (size>1){
        if ( (rank % 2)==0 ){                   // even nodes are sending first and then receiving field
           for (int i=0; i<ncells; i++){
	     work[2*i]  =field->field[last].at(i).real();
	     work[2*i+1]=field->field[last].at(i).imag();
	   }
	   MPI_Send(work,2*ncells, /* <= number of DOUBLES */
               MPI_DOUBLE,rank_next,tag,MPI_COMM_WORLD);
	   MPI_Recv(work,2*ncells, MPI_DOUBLE,rank_prev,tag,MPI_COMM_WORLD,&status);
	   for (int i=0; i<ncells; i++){
	     complex <double> ctemp=complex<double> (work[2*i],work[2*i+1]);
	     field->field[last].at(i)=ctemp;
	   }
	} else {                               // odd nodes are receiving first and then sending

	  MPI_Recv(work,2*ncells, /* <= number of DOUBLES */
              MPI_DOUBLE,rank_prev,tag,MPI_COMM_WORLD,&status);

	  for (int i=0; i<ncells; i++){
	    complex <double> ctemp=complex<double> (work[2*i],work[2*i+1]);
	    work[2*i]  =field->field[last].at(i).real();
	    work[2*i+1]=field->field[last].at(i).imag();
	    field->field[last].at(i)=ctemp;
	  }
	  MPI_Send(work,2*ncells,MPI_DOUBLE,rank_next,tag,MPI_COMM_WORLD);
	}
      }

      // first node has empty field slipped into the time window
      if ((rank==0) && (direction >0)){
        for (int i=0; i<ncells; i++){
	  field->field[last].at(i)=complex<double> (0,0);
        }
      }

      if ((rank==(size-1)) && (direction <0)){
        for (int i=0; i<ncells; i++){
	  field->field[last].at(i)=complex<double> (0,0);
        }
      }

      // last was the last slice to be transmitted to the succeding node and then filled with the 
      // the field from the preceeding node, making it now the start of the field record.
      field->first=last;
      if (direction<0){
	field->first=(last+1) % field->field.size();
      }
  }
}
