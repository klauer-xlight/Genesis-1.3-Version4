// sequence.h: interface for the base clase of sequences such as Hammersley
//             or randomnumber
//
//////////////////////////////////////////////////////////////////////


#ifndef __GENESIS_SEQUENCE__
#define __GENESIS_SEQUENCE__
#include <cmath>
#include <iostream>
#include <vector>

//#include "RandomU.h"
#include "Inverfc.h"

class Sequence
{
public:
	virtual ~Sequence() {};
	virtual double getElement() = 0;
	virtual void set(unsigned int) = 0;
};

class RandomU;

// derived sequences

class SequenceConst : public Sequence{
public:
    ~SequenceConst() {};
    double getElement();
    void set(unsigned int) {};
    void init(double );
private:
    double c;
    int i;
};
inline void SequenceConst::init(double c_in){ c = c_in; i = 0;}
inline double SequenceConst::getElement(){ return c;}

class SequencePolynom : public Sequence{
public:
    ~SequencePolynom() {};
    double getElement();
    void set(unsigned int) {};
    void init(double, double, double, double, double);
private:
    double c0 {0.};
    double c1 {0.};
    double c2 {0.};
    double c3 {0.};
    double c4 {0.};
    unsigned long neval {0};
};
inline void SequencePolynom::init(double c0_in, double c1_in, double c2_in, double c3_in, double c4_in){ c0=c0_in; c1=c1_in; c2=c2_in; c3=c3_in; c4=c4_in; neval = 0;}
inline double SequencePolynom::getElement()
{
  double v;

  v = c0 + c1*neval + c2*neval*neval + c3*neval*neval*neval + c4*neval*neval*neval*neval;
//  if(0==rank) {
//    std::cout << "neval=" << neval << std::endl;
//  }

  neval++;
  return(v);
}

class SequencePower : public Sequence{
public:
    ~SequencePower() {};
    double getElement();
    void set(unsigned int) {};
    void init(double, double, double, int );
private:
    double c,dc,alpha;
    int  i,n;
};

inline void SequencePower::init(double c_in, double dc_in, double a_in, int n_in)
{
    c = c_in;dc = dc_in;alpha = a_in;i = 0;n = n_in;
}

inline double SequencePower::getElement() {
    i++;
    if (i < n) {
        return c;
    }
    return c + dc * pow(static_cast<double>(i - n), alpha);
}


class SequenceList : public Sequence{
public:
    ~SequenceList() {};
    double getElement();
    void set(unsigned int) {};
    void init(std::vector<double> &, double);
private:
    std::vector<double> val;
    double def;
    size_t i;
};

inline void SequenceList::init(std::vector<double> &val_in, double def_in)
{
    i = 0;
    val = val_in;
    def = def_in;
}

inline double SequenceList::getElement() {
    i++;
    if (i > val.size()) {
        return def;
    }
    return val.at(i - 1);
}



class SequenceRandom : public Sequence
{
public:
    ~SequenceRandom(){};
    double getElement();
    void set(unsigned int) {}
    void init(double, double, int, bool);

private:
    double c0,dc;
    int seed;
    bool gauss;
    RandomU *seq;
    Inverfc erf;
};


#endif 
