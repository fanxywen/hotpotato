
#include <vector>
//#include <boost/serialization/vector.hpp>
#include <iostream>
#include <fstream>
#include <string>

class potato{

    friend std::ostream & operator<< (std::ostream & out, potato & p){
      //    out<<p.hops;
    out<<p.trace;
    return out;
  }
  friend std::istream & operator>>(std::istream & in, potato &p){
    //    in>>p.hops;
    in>>p.trace;
    return in;
  }
 private:
  //  int hops;
  std::string trace;
 public:
 potato(): trace(){}
  //potato(): hops(n), trace(){}
 potato(const potato & rhs): trace(rhs.trace){}
  potato & operator=(const potato& rhs){
    if(this != &rhs){
      //      hops = rhs.hops;
      trace = rhs.trace;
    }
    return *this;
  }
  /*  int getHops(){
    return hops;
    }*/
  std::string& getTrace(){
    return trace;
  }
  void addTrace(int id){
    std::string str = std::to_string(id);
    str = str + ",";
    trace.append(str);
  }
  void printTrace(){
    std::cout<<trace<<std::endl;
  }
  /*  void decreseHops(){
    --hops;
  }
  bool validateHop(){
    if(hops != 0){
      return true;
    } 
    return false;
    }*/
  ~potato(){}
};
//friend class boost::serialization::access;
