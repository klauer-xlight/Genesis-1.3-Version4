#include "Field.h"
#include "genesis_fortran_common.h"

#ifdef VTRACE
#include "vt_user.h"
#endif


Field::~Field(){}
Field::Field(){
  harm=1;
  polarization=false;
}


void Field::init(int nsize, int ngrid_in, double dgrid_in, double xlambda0, double slicelen_in, double s0_in, int harm_in)
{  

  slicelength=slicelen_in;
  s0=s0_in;


  harm=harm_in;   // harmonics
  xlambda=xlambda0/static_cast<double>(harm);   // wavelength : xlambda0 is the reference wavelength of the fundamental
  ngrid=ngrid_in;

  gridmax=dgrid_in;
  dgrid=2*gridmax/static_cast<double>(ngrid-1); // grid pointe separation

  if (field.size()!=nsize){                  // allocate the memory in advance
     field.resize(nsize);
  }
  
  if (field[0].size()!=ngrid*ngrid){
    for (int i=0;i<nsize;i++){
        field[i].resize(ngrid*ngrid); 
    } 
  } 
   
  xks=4.*asin(1)/xlambda;
  first=0;                                // pointer to slice which correspond to first in the time window
  dz_save=0;

  
  return;
}


// at each run the buffer should be cleared.
void Field::initDiagnostics(int nz)
{
  idx=0;
  int ns=field.size();


  power.resize(ns*nz);
  xavg.resize(ns*nz);
  xsig.resize(ns*nz);
  yavg.resize(ns*nz);
  ysig.resize(ns*nz);
  nf_intensity.resize(ns*nz);
  nf_phi.resize(ns*nz);
  ff_intensity.resize(ns*nz);
  ff_phi.resize(ns*nz);

}


void Field::setStepsize(double delz)
{

  if (delz!=dz_save){
    dz_save=delz;
    //getdiag_(&delz,&dgrid,&xks,&ngrid);
  }

}

bool Field::getLLGridpoint(double x, double y, double *wx, double *wy, int *idx){

      if ((x>-gridmax) && (x < gridmax) && (y>-gridmax) && (y < gridmax)){
           *wx = (x+gridmax)/dgrid;
           *wy = (y+gridmax)/dgrid;
           int ix= static_cast<int> (floor(*wx));
           int iy= static_cast<int> (floor(*wy));
           *wx=1+floor(*wx)-*wx;
           *wy=1+floor(*wy)-*wy;
           *idx=ix+iy*ngrid;
           return true;
      } else {
           return false;
      }
}


void Field::track(double delz, Beam *beam, Undulator *und)
{
  
#ifdef VTRACE
  VT_TRACER("Field_Tracking");
#endif  


  solver.getDiag(delz,dgrid,xks,ngrid);  // check whether step size has changed and recalculate aux arrays
  solver.advance(delz,this,beam,und);
  return;

}


bool Field::subharmonicConversion(int harm_in, bool resample)
{
  // note that only an existing harmonic will be preserved
  harm=harm_in;
  if ((!resample)|| (harm_in==1)){
    return true;
  }

  slicelength*=static_cast<double>(harm_in);
  int nsize=field.size();


  if ((nsize % harm) !=0) { return false;} 

  for (int i=0; i<nsize/harm;i++){
    int jstart=0;
    if (i==0){jstart=1;}
    for (int j=jstart; j<harm;j++){
      int idx=i*harm+j;  // that is the slice where the particles are copied from
      for (int k=0; k<ngrid*ngrid;k++){
	field[i].at(k)+=field[idx].at(k);  // add field to slice
	field[idx].at(k)=0;
      }
    }
  }

  field.resize(nsize/harm);
  double scl=1./static_cast<double>(harm);
  for (int i=0; i<field.size();i++){
     for (int k=0; k<ngrid*ngrid;k++){
       field[i].at(k)*scl;  // do mean average because field were added up.
     }
  }
  
  if ((first % harm) != 0){
    first-=(first%harm);
  }
  first=first/harm;
  return true;
}



bool Field::harmonicConversion(int harm_in, bool resample)
{
  // note that only an existing harmonic will be preserved
  harm=1;
  if ((!resample)|| (harm_in==1)){
    return true;
  }

  slicelength/=static_cast<double>(harm_in);

  int nloc=field.size();
  field.resize(nloc*harm_in);
  for (int i=nloc;i<nloc*harm_in;i++){
    field.at(i).resize(ngrid*ngrid); // allocate space
  }

  // copy old slices into full field record
  for (int i=nloc;i>0;i--){
    for (int j=0;j<harm_in;j++){
      int idx=i*harm_in-1-j;
      for (int k=0; k<ngrid*ngrid;k++){
	field[idx].at(k)=field[(i-1)].at(k);
      }
    }
  }
  cout << "Slicing field" << endl;
  first*=harm_in; // adjust pointer of first slice
  return true;
}



//  diagnostic part


void Field::diagnostics(bool output)
{

  if (!output) { return; }

  double shift=-0.5*static_cast<double> (ngrid);
  complex<double> loc;
  int ds=field.size();
  int ioff=idx*ds;

  for (int is=0; is < ds; is++){
    int islice= (is+first) % ds ;   // include the rotation due to slippage
 
    double bpower=0;
    double bxavg=0;
    double byavg=0;
    double bxsig=0;
    double bysig=0;
    double bintensity=0;   
    double bfarfield=0;
    double bphinf=0;
    double bphiff=0;
    complex<double> ff = complex<double> (0,0);

    for (int iy=0;iy<ngrid;iy++){
      double dy=static_cast<double>(iy)+shift;
      for (int ix=0;ix<ngrid;ix++){
        double dx=static_cast<double>(ix)+shift;
	int i=iy*ngrid+ix;
        loc=field.at(islice).at(i);
        double wei=loc.real()*loc.real()+loc.imag()*loc.imag();
        ff+=loc;
        bpower+=wei;
        bxavg+=dx*wei;
        bxsig+=dx*dx*wei;
        byavg+=dy*wei;
        bysig+=dy*dy*wei;
      }
    }
    
    if (bpower>0){
      bxavg/=bpower;
      bxsig=sqrt(abs(bxsig/bpower-bxavg*bxavg));
      byavg/=bpower;
      bysig=sqrt(abs(bysig/bpower-byavg*byavg));
    }

    bfarfield=ff.real()*ff.real()+ff.imag()*ff.imag();
    if (bfarfield > 0){
      bphiff=atan2(ff.imag(),ff.real());
    }
    int i=(ngrid*ngrid-1)/2;
    loc=field.at(islice).at(i);
    bintensity=loc.real()*loc.real()+loc.imag()*loc.imag();
    if (bintensity > 0) {
      bphinf=atan2(loc.imag(),loc.real()); 
    }

    double ks=4.*asin(1)/xlambda;
    double scl=dgrid*eev/ks;
    bpower*=scl*scl/vacimp; // scale to W
    bxavg*=dgrid;
    bxsig*=dgrid;
    byavg*=dgrid;
    bysig*=dgrid;
    bintensity*=eev*eev/ks/ks/vacimp;  // scale to W/m^2

    power[ioff+is]=bpower;
    xavg[ioff+is] =bxavg;
    xsig[ioff+is] =bxsig;
    yavg[ioff+is] =byavg;
    ysig[ioff+is] =bysig;
    nf_intensity[ioff+is]=bintensity;
    nf_phi[ioff+is]=bphinf;
    ff_intensity[ioff+is]=bfarfield;
    ff_phi[ioff+is]=bphiff;

  }
  
  idx++;
  
}
