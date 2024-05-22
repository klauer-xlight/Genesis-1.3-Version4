#include "writeFieldHDF5.h"

extern bool MPISingle;

// constructor destructor
WriteFieldHDF5::WriteFieldHDF5() {}
WriteFieldHDF5::~WriteFieldHDF5() {}

bool WriteFieldHDF5::write(string fileroot, vector<Field *> *field)
{
  string file;

  MPI_Comm_rank(MPI_COMM_WORLD, &rank); // assign rank to node
  MPI_Comm_size(MPI_COMM_WORLD, &size); // assign rank to node
  if (MPISingle){
    size=1;
    rank=0;
  }

  // loop over all fields (= all harmonics)
  for (size_t i=0; i<field->size();i++){
    size_t harm=field->at(i)->harm;
    char charm[10];
    snprintf(charm, sizeof(charm), ".h%zu", harm);
    if (harm==1){
      file=fileroot;
    } else {
      file=fileroot+string(charm);
    }
    bool ok=this->writeMain(file,field->at(i));
    if(!ok)
      return(false);
  }

  return(true);
}





bool WriteFieldHDF5::writeMain(string fileroot, Field *field)
{
  string filename;
  filename = fileroot+".fld.h5";
  if (rank == 0) { cout << "Writing field distribution to file: " <<filename << " ..." << endl;} 
  if (!create_outfile(&fid, filename)) {
    return(false);
  }


  s0=rank;
  size_t ntotal=size*field->field.size();
  size_t smin=rank*field->field.size();
  size_t smax=smin+field->field.size();

  // write global data
  hid_t gid=H5Gcreate(fid,"Meta",H5P_DEFAULT,H5P_DEFAULT,H5P_DEFAULT);
  this->writeVersion(gid);
  H5Gclose(gid);

  this->writeGlobal(field->xlambda,field->slicelength,field->s0,field->dgrid,field->ngrid,ntotal);

  /*** if enabled, dump complex field data to file ***/
  int ngrid=field->ngrid;
  vector<double> work;
  work.resize(ngrid*ngrid);

  double ks=4.*asin(1)/field->xlambda;
  double scl=field->dgrid*eev/ks/sqrt(vacimp);

  if(field->dumpFieldEnabled())
  {
    // loop through slices
    for (size_t i=0; i<ntotal;i++)
    {
      s0=-1;
      char name[16];
      snprintf(name, sizeof(name), "slice%6.6zu", i+1);
      gid=H5Gcreate(fid,name,H5P_DEFAULT,H5P_DEFAULT,H5P_DEFAULT);

      if ((i>=smin) && (i<smax)){
        s0=0;    // select the slice which is writing
      }

      int islice= (i+field->first) % field->field.size() ;   // include the rotation due to slippage

      if (s0==0){
        for (size_t j=0; j<ngrid*ngrid;j++){ 
          work[j]=scl*field->field.at(islice).at(j).real();
        }  
      }
      this->writeSingleNode(gid,"field-real"," ",&work);     

      if (s0==0){
        for (size_t j=0; j<ngrid*ngrid;j++){ 
          work[j]=scl*field->field.at(islice).at(j).imag();
        }  
      }
      this->writeSingleNode(gid,"field-imag"," ",&work);     

      H5Gclose(gid);
    }
  }
  else
  {
    if(rank==0) {
      cout << "Info: not dumping field data to .fld.h5 file (disabled by parameter 'exclude_field_dump')" << endl;
    }
  }


  /*** Compute intensity projections ***/
  const double factor_xy=1.;
  const double factor_xz=1., factor_yz=1.; // xz and yz projections have identical scaling factor
  vector<double> local_int_xy(ngrid*ngrid);
  vector<double> glbl_int_xy(ngrid*ngrid);
  vector<double> int_xz(ngrid*field->field.size());
  vector<double> int_yz(ngrid*field->field.size());

  for (int i=smin; i<smax; i++)
  {
    int islice= (i+field->first) % field->field.size() ;   // include the rotation due to slippage

    complex<double> loc;
    int idx_fld;
    int idx_xz, idx_yz;
    double wei;
    for (int iy=0; iy<ngrid; iy++){
      for (int ix=0; ix<ngrid; ix++){
	idx_fld = iy*ngrid + ix;
        loc=field->field.at(islice).at(idx_fld);
        wei=loc.real()*loc.real()+loc.imag()*loc.imag();

        // int_xy data has same dimension as field of slice => can use identical indexing scheme
        local_int_xy[idx_fld] += factor_xy*wei;

        idx_xz = field->field.size()*ix + (i-smin);
        idx_yz = field->field.size()*iy + (i-smin);
        int_xz[idx_xz] += factor_xz*wei;
        int_yz[idx_yz] += factor_yz*wei;
      }
    }
  }

  MPI_Reduce(&local_int_xy[0], &glbl_int_xy[0], local_int_xy.size(), MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

  // rank=0 process has the total sum. So only process 0 should write,
  // this is controlled by 's0' variable (see code of function HDF5Base::writeSingleNode)
  s0 = (rank==0) ? 0 : -1;
  this->writeSingleNode(fid, "int_xy", " ", &glbl_int_xy); // every process calls this function to update meta data, but only process 0 writes (NOTE: for this to work, the data array must have identical size in every MPI process)



  vector<hsize_t> my_count(2);
  vector<hsize_t> my_offset(2);

  my_count[0] = ngrid;
  my_count[1] = field->field.size();
  my_offset[0] = 0;
  my_offset[1] = field->field.size() * rank;

  writeBufferD(fid, "int_xz", " ", &int_xz, &my_count, &my_offset);
  writeBufferD(fid, "int_yz", " ", &int_yz, &my_count, &my_offset);


  H5Fclose(fid);

 
  return(true); // success
}


/*
 * More generic version of HDF5Base::writeBuffer
 * - every MPI process writes dataset of dimension count = {dimX, dimY}
 * - total data size is {dimX, mpisize*dimY} (with size = number of MPI processes)
 * - offset typically should be {0, mpirank*dimY} (i.e. the data regions
 *   must not overlap)
 * Note that this function does not use variable 's0' to control the offset.
 */
void WriteFieldHDF5::writeBufferD(hid_t gid, string dataset, string unit, vector<double> *data, vector<hsize_t> *count, vector<hsize_t> *offset)
{
  if ((count->size()!=2) && (offset->size()!=2))
  {
     cout << "dimension vector: incorrect element count" << endl;
     return;
  }

  // step 1 - calculate the file space and create dataset
  int size=1;
  if (!MPISingle){
     MPI_Comm_size(MPI_COMM_WORLD, &size); // assign rank to node
  }

  // hsize_t dz=data->size()/ds;

  vector<hsize_t> fblock(*count);
  fblock[1] *= size;

  hid_t filespace=H5Screate_simple(2,&fblock[0],NULL);
  hid_t did=H5Dcreate(gid,dataset.c_str(),H5T_NATIVE_DOUBLE,filespace,H5P_DEFAULT,H5P_DEFAULT,H5P_DEFAULT);   
  H5Sclose(filespace);

  // write attribute
  hid_t aid = H5Screate(H5S_SCALAR);
  hid_t atype = H5Tcopy(H5T_C_S1);
  H5Tset_size(atype, unit.size());
  H5Tset_strpad(atype,H5T_STR_NULLTERM);
  hid_t attr = H5Acreate2(did,"unit",atype,aid,H5P_DEFAULT,H5P_DEFAULT);
  H5Awrite(attr,atype,unit.c_str());
  H5Sclose(aid);
  H5Tclose(atype);
  H5Aclose(attr);


  // step 2 - file space
  hid_t memspace=H5Screate_simple(2,&(*count)[0],NULL);

  // step 3 - set up hyperslab for file transfer.
  filespace=H5Dget_space(did);
  H5Sselect_hyperslab(filespace,H5S_SELECT_SET,&(*offset)[0],NULL,&(*count)[0],NULL);



  // step 4 - set up transfer and write
  hid_t pid =  H5Pcreate(H5P_DATASET_XFER);
  if (!MPISingle){
     H5Pset_dxpl_mpio(pid,H5FD_MPIO_COLLECTIVE);    
  }
  H5Dwrite(did,H5T_NATIVE_DOUBLE,memspace,filespace,pid,&data->at(0));

  
  // close all HDF5 stuff 
  H5Sclose(memspace);
  H5Sclose(filespace);
  H5Pclose(pid);
  H5Dclose(did);
}

void WriteFieldHDF5::writeGlobal(double reflen, double slicelen, double s0, double dx, int nx, int count)
{

  
  vector<double> tmp;
  tmp.resize(1);

  tmp[0]=reflen;
  this->writeSingleNode(fid,"wavelength","m",&tmp);
  tmp[0]=slicelen;
  this->writeSingleNode(fid,"slicespacing","m",&tmp);
  tmp[0]=s0;
  this->writeSingleNode(fid,"refposition","m",&tmp);
  tmp[0]=dx;
  this->writeSingleNode(fid,"gridsize","m",&tmp);
  
  vector<int> itmp;
  itmp.resize(1);

  itmp[0]=nx;
  this->writeSingleNodeInt(fid,"gridpoints",&itmp);

  itmp[0]=count;
  this->writeSingleNodeInt(fid,"slicecount",&itmp);
  

  
  return;
}


