#include "QuietLoading.h"

QuietLoading::QuietLoading(){
  sx=nullptr;
  sy=nullptr;
  st=nullptr;
  spx=nullptr;
  spy=nullptr;
  sg=nullptr;
}
QuietLoading::~QuietLoading()= default;



void QuietLoading::init(bool one4one, int *base)
{
  delete sx;
  delete sy;
  delete spx;
  delete spy;
  delete st;
  delete sg;

  if (one4one){
     RandomU rseed(base[0]);
     double val;
     for (size_t i=0; i<=base[1];i++){
        val=rseed.getElement();
     }
     val*=1e9;
     int locseed=static_cast<int> (round(val));
     st  = (Sequence *) new RandomU (locseed);
     sg  = st;
     sx  = st;
     sy  = st;
     spx = st;
     spy = st;

  } else {
    st  = (Sequence *) new Hammerslay (base[0]);
    sg  = (Sequence *) new Hammerslay (base[1]);
    sx  = (Sequence *) new Hammerslay (base[2]);
    sy  = (Sequence *) new Hammerslay (base[3]);
    spx = (Sequence *) new Hammerslay (base[4]);
    spy = (Sequence *) new Hammerslay (base[5]);
  }

}

void QuietLoading::loadQuiet(Particle *beam, BeamSlice *slice, size_t npart, size_t nbins, double theta0, size_t islice)
{


  // resets Hammersley sequence but does nothing for random sequence;
  Sequence *seed = new RandomU(islice);
  int iseed=static_cast<int>(round(seed->getElement()*1e9));
  delete seed;

  // initialize the sequence to new values to avoid that all core shave the same distribution
  st->set(iseed);
  sg->set(iseed);
  sx->set(iseed);
  sy->set(iseed);
  spx->set(iseed);
  spy->set(iseed);

  size_t mpart=npart/nbins;

  Inverfc erf;

  double dtheta=1./static_cast<double>(nbins);
  // raw distribution
   for (size_t i=0;i <mpart; i++){
    beam[i].theta=st->getElement()*dtheta;
    beam[i].gamma=erf.value(2*sg->getElement());
    beam[i].x    =erf.value(2*sx->getElement());
    beam[i].y    =erf.value(2*sy->getElement());
    beam[i].px   =erf.value(2*spx->getElement());
    beam[i].py   =erf.value(2*spy->getElement());
  }


  // normalization

  double z=0;
  double zz=0;

  double norm=0;
  if (mpart>0){
    norm=1./static_cast<double>(mpart);
  }

  // energy

  for (size_t i=0; i<mpart; i++){
    z +=beam[i].gamma;
    zz+=beam[i].gamma*beam[i].gamma;
  }

  z*=norm;
  zz=sqrt(fabs(zz*norm-z*z));

  if (zz>0){ zz=1/zz;}
  zz*=slice->delgam;

  for (size_t i=0; i<mpart; i++){
    beam[i].gamma= (beam[i].gamma-z)*zz+slice->gamma;
  }

  // x and px correlation;
  z=0;
  zz=0;
  double p=0;
  double zp=0;

  for (size_t i=0; i<mpart;i++){
    z +=beam[i].x;
    zz+=beam[i].x*beam[i].x;
    p +=beam[i].px;
    zp+=beam[i].x*beam[i].px;
  }
  z*=norm;
  p*=norm;
  zz=sqrt(fabs(zz*norm-z*z));
  if (zz>0) {zz=1./zz;}
  zp=(zp*norm-z*p)*zz*zz;

  for (size_t i=0; i<mpart;i++){
    beam[i].px=beam[i].px-p;
    beam[i].px-=zp*beam[i].x;
    beam[i].x=(beam[i].x-z)*zz;
  }

  // y and py correlation;
  z=0;
  zz=0;
  p=0;
  zp=0;

  for (size_t i=0; i<mpart;i++){
    z +=beam[i].y;
    zz+=beam[i].y*beam[i].y;
    p +=beam[i].py;
    zp+=beam[i].y*beam[i].py;
  }
  z*=norm;
  p*=norm;
  zz=sqrt(fabs(zz*norm-z*z));
  if (zz>0) {zz=1./zz;}
  zp=(zp*norm-z*p)*zz*zz;

  for (size_t i=0; i<mpart;i++){
    beam[i].py=beam[i].py-p;
    beam[i].py-=zp*beam[i].y;
    beam[i].y=(beam[i].y-z)*zz;
  }


  z = 0;
  zz = 0;
  p = 0;
  double pp= 0 ;

  for (size_t i=0; i<mpart;i++){
    z +=beam[i].px;
    zz+=beam[i].px*beam[i].px;
    p +=beam[i].py;
    pp+=beam[i].py*beam[i].py;
  }
  z*=norm;
  p*=norm;
  zz=sqrt(fabs(zz*norm-z*z));
  if (zz>0) {zz=1./zz;}
  pp=sqrt(fabs(pp*norm-p*p));
  if (pp>0) {pp=1./pp;}

  for (size_t i=0; i<mpart;i++){
    beam[i].px=(beam[i].px-z)*zz;
    beam[i].py=(beam[i].py-p)*pp;
  }


  // scale to physical size

  double sigx=sqrt(slice->ex*slice->betax/slice->gamma);
  double sigy=sqrt(slice->ey*slice->betay/slice->gamma);
  double sigpx=sqrt(slice->ex/slice->betax/slice->gamma);
  double sigpy=sqrt(slice->ey/slice->betay/slice->gamma);
  double corx=-slice->alphax/slice->betax;
  double cory=-slice->alphay/slice->betay;

  for (size_t i=0; i<mpart;i++){
    beam[i].x *=sigx;
    beam[i].y *=sigy;
    beam[i].px=sigpx*beam[i].px+corx*beam[i].x;
    beam[i].py=sigpy*beam[i].py+cory*beam[i].y;
    beam[i].px*=beam[i].gamma;
    beam[i].py*=beam[i].gamma;
    beam[i].x +=slice->xcen;
    beam[i].y +=slice->ycen;
    beam[i].px+=slice->pxcen;
    beam[i].py+=slice->pycen;
  }

  // mirror particles for quiet loading

  for (size_t i=mpart; i>0; i--){
    size_t i1=i-1;
    size_t i2=nbins*i1;
    for (size_t j=0;j<nbins;j++){
      beam[i2+j].gamma=beam[i1].gamma;
      beam[i2+j].x    =beam[i1].x;
      beam[i2+j].y    =beam[i1].y;
      beam[i2+j].px   =beam[i1].px;
      beam[i2+j].py   =beam[i1].py;
      beam[i2+j].theta=beam[i1].theta+j*dtheta;
    }
  }


  // scale phase

  for (size_t i=0;i<npart;i++){
    beam[i].theta*=theta0;
  }

  double pi = 2*asin(1.);
  double blocal = slice->bunch;
  // bunching and energy modulation
  if (blocal !=0){
      double btar = blocal;  // target value
      double blow = 0;  // assume it initial guess
      double bupp = 0.9999;
      double bmid = 0;
      while ((bupp-blow) > 1e-6){
          bmid = 0.5*(blow+bupp);
          double bprobe = sin((1-bmid)*pi)/((1-bmid)*pi);
          if (btar<bprobe){
              bupp = bmid;
          } else {
              blow = bmid;
          }
      }
      blocal = bmid;
  }

  if ((blocal!=0)||(slice->emod!=0)) {
      if (slice->bunch > 0.1) {
          for (size_t i=0; i < npart; i++) {
              beam[i].gamma -= slice->emod * sin(beam[i].theta - slice->emodphase);
              beam[i].theta += -blocal * beam[i].theta + blocal * pi + slice->bunchphase+pi;
          }
      } else {
          for (size_t i=0; i < npart; i++) {
              beam[i].gamma -= slice->emod * sin(beam[i].theta - slice->emodphase);
              beam[i].theta-=2*slice->bunch*sin(beam[i].theta-slice->bunchphase);
          }
      }
  }
}
