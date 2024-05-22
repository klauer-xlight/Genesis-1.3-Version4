#include "GenProfile.h"


Profile::Profile()
{
}

Profile::~Profile()
{
  prof.clear();
}

bool Profile::init(int rank, map<string,string> *arg,string element)
{

  ProfileBase *p;
  string label;

  if (element.compare("&profile_const")==0){
    p=(ProfileBase *)new ProfileConst();
    label=p->init(rank,arg);
  } 
  if (element.compare("&profile_gauss")==0){
    p=(ProfileBase *)new ProfileGauss();
    label=p->init(rank,arg);
  } 
  if (element.compare("&profile_polynom")==0){
    p=(ProfileBase *)new ProfilePolynom();
    label=p->init(rank,arg);
  } 
  if (element.compare("&profile_step")==0){
    p=(ProfileBase *)new ProfileStep();
    label=p->init(rank,arg);
  } 
  if (element.compare("&profile_file")==0){
    p=(ProfileBase *)new ProfileFile();
    ProfileFile *f;
    label=p->init(rank,arg);
    f=static_cast<ProfileFile *>(p);
    if (f->names.size()>0){
      map<string,string> derivedArg;
      for (size_t i=0; i < f->names.size(); i++){
	derivedArg.clear();
	derivedArg["label"]=f->names[i];
        stringstream ss(f->xdataset);
	string file;
	char delim='/';             // does not compile it I use double quotation marks
	getline(ss,file,delim);
	string ydata=file+'/'+f->names[i];
	derivedArg["xdata"]=f->xdataset;
	derivedArg["ydata"]=ydata;
	string bstr="false";
	if (f->isTime) {bstr="true";}
	derivedArg["isTime"]=bstr;
	bstr="false";
	if (f->revert) {bstr="true";};
	derivedArg["reverse"]=bstr;
	derivedArg["autoassign"]="false";
	this->init(rank,&derivedArg,"&profile_file");		   
      }
      return true;
    }
  }
  if (element.compare("&profile_file_multi")==0) {
    ProfileFileMulti p;
    p.setup(rank, arg, &prof);

    /* no need to continue as ProfileFileMulti registered all profiles */
    return(true);
  }

  // register profiles for later use (all except "profile_file_multi")
  if (label.size()<1){
    return false;
  } else {
    prof[label]=p;
  }
  if (rank==0) {cout << "Adding profile with label: " << label << endl;}
  return true;
}

bool Profile::check(string label){
  if (label.size() < 1) { return true; }
  if (prof.find(label)!=prof.end()){  
    return true;
  }
  return false;
}

/* This function is called to evaluate the Profile named 'label' at position 's' (central entrance point) */
double Profile::value(double s, double val, string label)
{
  if ((label.size()<1)||(prof.find(label)==prof.end())){  
    return val;
  } else {
    return  prof[label]->value(s);
  }
}






//------------------------------------
// individual profiles


string ProfileConst::init(int rank, map<string,string>*arg)
{

  string label="";
  c0=0;
  map<string,string>::iterator end=arg->end();

  if (arg->find("label")!=end){label = arg->at("label");  arg->erase(arg->find("label"));}
  if (arg->find("c0")!=end)   {c0    = atof(arg->at("c0").c_str());  arg->erase(arg->find("c0"));}

  if (arg->size()!=0){
    if (rank==0){ cout << "*** Error: Unknown elements in &profile_const" << endl; this->usage();}
    return "";
  }
  if ((label.size()<1)&&(rank==0)){
    cout << "*** Error: Label not defined in &profile_const" << endl; this->usage();
  }
  return label;
}

double ProfileConst::value(double z)
{
  return c0;
}

void ProfileConst::usage(){
  cout << "List of keywords for PROFILE_CONST" << endl;
  cout << "&profile_const" << endl;
  cout << " string label = <empty>" << endl;
  cout << " double c0 = 0" << endl;
  cout << "&end" << endl << endl;
  return;
}

//-----------------------

string ProfilePolynom::init(int rank, map<string,string>*arg)
{
  string label="";
  map<string,string>::iterator end=arg->end();
  c.resize(5);
  for (size_t i=0; i< c.size();i++){ c[i]=0;}
  

  if (arg->find("label")!=end){label = arg->at("label");  arg->erase(arg->find("label"));}
  if (arg->find("c0")!=end)   {c[0]    = atof(arg->at("c0").c_str());  arg->erase(arg->find("c0"));}
  if (arg->find("c1")!=end)   {c[1]    = atof(arg->at("c1").c_str());  arg->erase(arg->find("c1"));}
  if (arg->find("c2")!=end)   {c[2]    = atof(arg->at("c2").c_str());  arg->erase(arg->find("c2"));}
  if (arg->find("c3")!=end)   {c[3]    = atof(arg->at("c3").c_str());  arg->erase(arg->find("c3"));}
  if (arg->find("c4")!=end)   {c[4]    = atof(arg->at("c4").c_str());  arg->erase(arg->find("c4"));}


  if (arg->size()!=0){
    if (rank==0){ cout << "*** Error: Unknown element in &profile_polynom" << endl; this->usage();}
    return "";
  }
  if ((label.size()<1)&&(rank==0)){
    cout << "*** Error: Label not defined in &profile_polynom" << endl; this->usage();
  }
  return label;
}


double ProfilePolynom::value(double z)
{
  double val=0;
  double zsave=1;
  for (size_t i=0;i<c.size();i++){
    val+=c[i]*zsave;
    zsave*=z;
  }
  return val;
}

void ProfilePolynom::usage(){
  cout << "List of keywords for PROFILE_POLYNOM" << endl;
  cout << "&profile_polynom" << endl;
  cout << " string label = <empty>" << endl;
  cout << " double c0 = 0" << endl;
  cout << " double c1 = 0" << endl;
  cout << " double c2 = 0" << endl;
  cout << " double c3 = 0" << endl;
  cout << " double c4 = 0" << endl;
  cout << "&end" << endl << endl;
  return;
}

//-----------------------

 string ProfileStep::init(int rank, map<string,string>*arg)
{

  string label="";
  c0=0;
  sstart=0;
  send=0;

  map<string,string>::iterator end=arg->end();

  if (arg->find("label")!=end)  {label = arg->at("label");  arg->erase(arg->find("label"));}
  if (arg->find("c0")!=end)     {c0    = atof(arg->at("c0").c_str());  arg->erase(arg->find("c0"));}
  if (arg->find("s_start")!=end){sstart= atof(arg->at("s_start").c_str());  arg->erase(arg->find("s_start"));}
  if (arg->find("s_end")!=end)  {send  = atof(arg->at("s_end").c_str());  arg->erase(arg->find("s_end"));}

  if (arg->size()!=0){
    if (rank==0){ cout << "*** Error: Unknown elements in &profile_step" << endl; this->usage();}
    return "";
  }
  if ((label.size()<1)&&(rank==0)){
    cout << "*** Error: Label not defined in &profile_step" << endl; this->usage();
  }
  return label;
}

double ProfileStep::value(double z)
{
  if ((z>=sstart) && (z <=send)){
    return c0;
  }
  return 0;
}

void ProfileStep::usage(){

  cout << "List of keywords for PROFILE_STEP" << endl;
  cout << "&profile_step" << endl;
  cout << " string label = <empty>" << endl;
  cout << " double c0 = 0" << endl;
  cout << " double s_start = 0" << endl;
  cout << " double s_end = 0" << endl;
  cout << "&end" << endl << endl;
  return;
}

//-----------------------------------

string ProfileGauss::init(int rank, map<string,string>*arg)
{
  string label="";
  c0=0;
  s0=0;
  sig=1;

  map<string,string>::iterator end=arg->end();

  if (arg->find("label")!=end){label = arg->at("label");  arg->erase(arg->find("label"));}
  if (arg->find("c0")!=end)   {c0    = atof(arg->at("c0").c_str());  arg->erase(arg->find("c0"));}
  if (arg->find("s0")!=end)   {s0    = atof(arg->at("s0").c_str());  arg->erase(arg->find("s0"));}
  if (arg->find("sig")!=end)  {sig   = atof(arg->at("sig").c_str());  arg->erase(arg->find("sig"));}

  if (arg->size()!=0){
    if (rank==0){ cout << "*** Error: Unknown elements in &profile_gauss" << endl; this->usage();}
    return "";
  }
  if ((label.size()<1)&&(rank==0)){
    cout << "*** Error: Label not defined in &profile_gauss" << endl; this->usage();
  }
  return label;
}

double ProfileGauss::value(double z)
{
  return c0*exp(-0.5*(z-s0)*(z-s0)/sig/sig);
}

void ProfileGauss::usage(){ 
  cout << "List of keywords for PROFILE_GAUSS" << endl;
  cout << "&profile_gauss" << endl;
  cout << " string label = <empty>" << endl;
  cout << " double c0 = 0" << endl;
  cout << " double s0 = 0" << endl;
  cout << " double sig= 1" << endl;
  cout << "&end" << endl << endl;
  return;
}


//-----------------------------------

string ProfileFile::init(int rank, map<string,string>*arg)
{
  string label="";
  xdataset="";
  ydataset="";
  isTime=false;
  revert=false;
  autoassign=false;

  map<string,string>::iterator end=arg->end();

  if (arg->find("label")!=end){label = arg->at("label");  arg->erase(arg->find("label"));}
  if (arg->find("xdata")!=end)   {xdataset = arg->at("xdata");  arg->erase(arg->find("xdata"));}
  if (arg->find("ydata")!=end)   {ydataset = arg->at("ydata");  arg->erase(arg->find("ydata"));}
  if (arg->find("isTime")!=end)  {isTime = atob(arg->at("isTime").c_str()); arg->erase(arg->find("isTime"));}
  if (arg->find("reverse")!=end) {revert = atob(arg->at("reverse").c_str()); arg->erase(arg->find("reverse"));}
  if (arg->find("autoassign")!=end) {autoassign = atob(arg->at("autoassign").c_str()); arg->erase(arg->find("autoassign"));}
  

  if (arg->size()!=0){
    if (rank==0){ cout << "*** Error: Unknown elements in &profile_file" << endl; this->usage();}
    return "";
  }


  bool success;

  if (autoassign){
    success=this->browseFile(xdataset,&names);
    if (!success) {
      if (rank==0) { cout << "*** Error: Cannot autoparse the HDF5 file for the dataset: " << xdataset  << endl;}
    } 
    return "";
  }

  if ((label.size()<1)&&(rank==0)){
    cout << "*** Error: Label not defined in &profile_file" << endl; this->usage();
  }


  int ndata=-1;

  success=this->simpleReadDouble1D(xdataset,&xdat);
  if (!success){
    if (rank==0){
      cout << "*** Error: Cannot read the HDF5 dataset: " << xdataset << endl;
    }
    return "";
  }

  success=this->simpleReadDouble1D(ydataset,&ydat);
  if (!success){
    if (rank==0){
      cout << "*** Error: Cannot read the HDF5 dataset: " << ydataset << endl;
    }
    return "";
  }

  /* check that
   * . x is monotonically increasing (needed for reversal and interpolation algorithms)
   * . x/y data are of identical dimension (needed for interpolation algorithm)
   */
  bool is_mono=true;
  bool ok=true;
  for(unsigned j=1; j<xdat.size(); j++) {
    if(xdat[j-1]>=xdat[j]) {
      is_mono=false;
      break;
    }
  }
  if(!is_mono) {
    ok=false;
    if(rank==0) {
      cout << "*** Error: &profile_file requires 'xdata' to be monotonically increasing (HDF5 object " << xdataset << ")" << endl;
    }
  }
  if(xdat.size() != ydat.size()) {
    ok=false;
    if(rank==0) {
      cout << "*** Error: &profile_file requires identical dimension of 'xdata' and 'ydata'" << endl;
    }
  }
  if(!ok) {
    return "";
  }


  if (isTime){ 
    for (size_t i=0; i<xdat.size();i++){
      xdat[i]*=299792458.0;         // scale time variable to space varial by multiplying the speed of light
    }  
  }
  
  if (revert){
    double xmin=xdat[0];
    double xmax=xdat[xdat.size()-1];
    reverse(xdat.begin(),xdat.end());
    reverse(ydat.begin(),ydat.end());
    for (size_t i=0;i<xdat.size();i++){
      xdat[i]=-xdat[i]+xmin+xmax;    // get the correct time window
    }
  }
  return label;
}

double ProfileFile::value(double z)
{
  if (z<xdat[0]){ return ydat[0]; }
  if (z>=xdat[xdat.size()-1]){ return ydat[xdat.size()-1]; }

  int idx=0;
  while(z>=xdat.at(idx)){
    idx++;
  }
  idx--;

  double wei=(z-xdat.at(idx))/(xdat.at(idx+1)-xdat.at(idx));
  double val=ydat.at(idx)*(1-wei)+wei*ydat.at(idx+1);
  return val;
}

void ProfileFile::usage(){ 
  cout << "List of keywords for PROFILE_FILE" << endl;
  cout << "&profile_file" << endl;
  cout << " string label = <empty>" << endl;
  cout << " string xdata = <empty>" << endl;
  cout << " string ydata = <empty>" << endl;
  cout << " bool isTime  = false" << endl;
  cout << " bool reverse = false" << endl;
  cout << " bool autoassign = false"  << endl;
  cout << "&end" << endl << endl;
  return;
}

//-----------------------------------

/*
 * ProfileFileMulti
 *
 * Example
 * &profile_file_multi
 *    file = beam.h5
 *    xdata = s
 *    ydata = gamma, delgam, current, ex, ey, betax, betay, alphax, alphay, xcenter, ycenter, pxcenter, pycenter, bunch, bunchphase, emod, emodphase
 *    label_prefix = prof
 * &end
 *
 * Generates profile objects 'prof.gamma', 'prof.delgam', 'prof.current', etc., each one corresponding to one &profile_file.
 */
void ProfileFileMulti::usage()
{
	cout << "List of keywords for PROFILE_FILE_MULTI" << endl;
	cout << "&profile_file_multi" << endl;
	cout << " string file = <empty>" << endl;
	cout << " string label_prefix = <empty>" << endl;
	cout << " string xdata = <empty>" << endl;
	cout << " string ydata = <empty>" << endl;
	cout << " bool isTime = false" << endl;
	cout << " bool reverse = false" << endl;
	cout << "&end" << endl << endl;
	return;
}

bool ProfileFileMulti::setup(int rank, map<string,string> *arg, map<string, ProfileBase *> *pprof)
{
	map<string,string>::iterator end=arg->end();
	string h5file, label_prefix, xobj, ydatasets;
	bool isTime=false, revert=false;

	vector<string> yobjs;
	vector<double> xdata;
	vector<vector<double> *> ydata;
	bool ok=true;

	if (arg->find("file")!=end)    {h5file = arg->at("file");  arg->erase(arg->find("file"));}
	if (arg->find("label_prefix")!=end) {label_prefix = arg->at("label_prefix");  arg->erase(arg->find("label_prefix"));}
	if (arg->find("xdata")!=end)   {xobj = arg->at("xdata");  arg->erase(arg->find("xdata"));}
	if (arg->find("ydata")!=end)   {ydatasets = arg->at("ydata");  arg->erase(arg->find("ydata"));}
	if (arg->find("isTime")!=end)  {isTime = atob(arg->at("isTime").c_str()); arg->erase(arg->find("isTime"));}
	if (arg->find("reverse")!=end) {revert = atob(arg->at("reverse").c_str()); arg->erase(arg->find("reverse"));}

	if (arg->size()!=0){
		if (rank==0){ cout << "*** Error: Unknown elements in &profile_file_multi" << endl; this->usage();}
		return(false);
	}

	/* verify that all needed info was provided */
	if ((h5file.size()<1) || (xobj.size()<1) || (ydatasets.size()<1) || (label_prefix.size()<1))
	{
		if(rank==0) {
			cout << "*** Error: you must provide arguments file, xdata, ydata, and label_prefix" << endl;
		}
		return(false);
	}

	/* break up ydata argument into list of objects to obtain from HDF5 file */
	chop(ydatasets, &yobjs);


	/* open HDF5 file for read (code from "HDF5Base::simpleReadDouble1D") */
	hid_t fid=H5Fopen(h5file.c_str(),H5F_ACC_RDONLY,H5P_DEFAULT);

	/* obtain xdata (common to add y data sets) */
	int nx = getDatasetSize(fid, (char *) xobj.c_str());
	xdata.resize(nx);
	readDataDouble(fid, (char *) xobj.c_str(), &xdata.at(0), nx);
	bool is_mono=true;
	for(unsigned j=1; j<nx; j++) {
		if(xdata[j-1]>=xdata[j]) {
			is_mono=false;
			break;
		}
	}
	if(!is_mono) {
		ok=false;
		if(rank==0) {
			cout << "*** Error: &profile_file_multi requires 'xdata' to be monotonically increasing (HDF5 object " << xobj << ")" << endl;
		}
	}

	/* obtain ydata sets and perform sanity checks */
	for(unsigned j=0; j<yobjs.size(); j++)
	{
		vector<double> *pv;
		int ny = getDatasetSize(fid, (char *) yobjs[j].c_str());
		pv = new vector<double>(ny);
		readDataDouble(fid, (char *) yobjs[j].c_str(), &pv->at(0), ny);
		ydata.push_back(pv);

		if (nx!=ny) {
			ok=false;
			if(rank==0) {
				cout << "*** Error: &profile_file_multi detected size mismatch of HDF5 objects " << xobj << " and " << yobjs[j] << endl;
			}
		}
	}
	H5Fclose(fid);

	/* if not ok, tidy up and return */
	if(!ok) {
		for(unsigned j=0; j<ydata.size(); j++)
			delete ydata[j];

		return(false);
	}

	if(isTime) {
		for(unsigned j=0; j<xdata.size(); j++)
			xdata[j]*=299792458.0; // using same speed_of_light value as in ProfileFile, to avoid that different results are obtained with &profile_file and &profile_file_multi
	}
	if(revert) {
		if(rank==0)
			cout << "Info: &profile_file_multi: reverse flag has no effect (not implemented, yet)" << endl;
	}

	/*
	 * generate data container objects and register them
	 * data cont holds vector for xdata and 1 ydata vector, interpolation function
	 */
	for(unsigned j=0; j<yobjs.size(); j++)
	{
		stringstream l;
		ProfileInterpolator *pi;

		l << label_prefix << "." << yobjs[j];

		pi = new ProfileInterpolator(l.str(), &xdata, ydata[j]);
		(*pprof)[l.str()] = pi;
		if (rank==0) {
			cout << "Adding profile with label: " << l.str() << endl;
		}
	}


	for(unsigned j=0; j<ydata.size(); j++)
		delete ydata[j];

	return(true);
}






ProfileInterpolator::ProfileInterpolator(string l, vector<double> *x, vector<double> *y)
{
	xdat_ = *x;
	ydat_ = *y;
	label_ = l; /* keeping 'label' as member variable as this can help during test phase */
}
ProfileInterpolator::~ProfileInterpolator() { }
string ProfileInterpolator::init(int a, map<string,string> *b) {return("");}
void ProfileInterpolator::usage() {}

double ProfileInterpolator::value(double z)
{
	if (z<xdat_[0]){ return ydat_[0]; }
	if (z>=xdat_[xdat_.size()-1]){ return ydat_[xdat_.size()-1]; }

	int idx=0;
	while(z>=xdat_.at(idx)){
		idx++;
	}
	idx--;

	double wei=(z-xdat_.at(idx))/(xdat_.at(idx+1)-xdat_.at(idx));
	double val=ydat_.at(idx)*(1-wei)+wei*ydat_.at(idx+1);
	return val;
}

