#include "Parser.h"

Parser::Parser()
{
  is_parse_error=false;
}

Parser::~Parser()
{
}

bool Parser::fail(void)
{
  return (is_parse_error);
}

bool Parser::open(string file, int inrank)
{
  string instring;
  ostringstream os;
  fstream fin;

  rank=inrank;
  fin.open(file.c_str(),ios_base::in);
  if (!fin){
         if (rank==0) {cout << "*** Error: Cannot open main input file: " << file << endl;}
         return false;
  }
  while(getline(fin,instring,'\n')){
	os << instring <<endl;
  }
  fin.close();
  input.str(os.str());
  return true;
}



bool Parser::parse(string *element, map<string,string> *argument)
{
  string instring, list;
  bool accumulate=false;
  
  
  while(getline(input,instring)){    // read line
    this->trim(instring);

    // check for empty lines or comments
    if ((!instring.compare(0,1,"#") || instring.length() < 1)){ continue; } // skip comment and empty rows

    // check for terminating element and then return
    if ((instring.compare("&end")==0)|| (instring.compare("&END")==0) || (instring.compare("&End")==0)){
      if (!accumulate){
        if (rank==0){cout << "*** Error: Termination string outside element definition in input file" << endl;}
        is_parse_error=true;
	return false;
      }
      return this->fillMap(&list,argument);
    }

    // check whether element starts
    if (!instring.compare(0,1,"&")){
      if (accumulate){
        if (rank==0) {cout << "*** Error: Nested elements in main input file" << endl;}
        is_parse_error=true;
	return false;
      }
      accumulate=true;
      for (size_t i=0; i<instring.size();i++){
	instring[i]=tolower(instring[i]);
      }
      *element=instring;   
      continue;
    }

    // add content if in element
    if (accumulate){
       list.append(instring);  // add all content into one string
       list.append(";");
    }
  }


  if(accumulate) {
    // Currently accumulating parameters of namelist, arriving here before reaching &end is considered an error
    if (rank==0){cout << "*** Error: Reached end of input file while parsing namelist '" << *element << "'" << endl;}
    is_parse_error=true;
  }
  
  return false;
}

bool Parser::fillMap(string *list,map<string,string> *map){

  // splits the string into a map with pairs : key and value.
  // keys are converted to lower case
  
  map->clear();
  string key,val;
 
  size_t pos,tpos;
  while ((pos=list->find_first_of(";")) !=string::npos){  // split into individual lines
     val=list->substr(0,pos);
     list->erase(0,pos+1);
     this->trim(key);
     if (val.length()>0){
       tpos=val.find_first_of("=");
       if (tpos ==string::npos){
         if (rank==0){ cout << "*** Error: Invalid format " << val << " in input file" << endl;}
         is_parse_error=true;
         return false;
       } else{
         key=val.substr(0,tpos);
         val.erase(0,tpos+1);
         this->trim(key);
         this->trim(val);
         map->insert(pair<string,string>(key,val));
       }
     }
  }

  is_parse_error=false; 
  return true;
}
