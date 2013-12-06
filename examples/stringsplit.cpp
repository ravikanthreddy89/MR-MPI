#include <string>
#include <vector>
#include <iostream>
#include <istream>
#include <ostream>
#include <iterator>
#include <sstream>
#include <algorithm>

int main()
{
  //std::string str = "The quick brown fox";
    std::string str="123456	34";
  // construct a stream from the string
    std::stringstream strstr(str);
 
  // use stream iterators to copy the stream to the vector as whitespace separated strings
    std::istream_iterator<std::string> it(strstr);
    std::istream_iterator<std::string> end;
    std::vector<std::string> results(it, end);

   // long number=*((long *)results[0].c_str());
     long number = atol((const char *)results[0].c_str());
   std::cout<<number<<std::endl; 

   std::cout<<sizeof(results[0])<<std::endl;
   std::cout<<sizeof(results[1])<<std::endl;

   std::cout<<results[0]<<std::endl;
   std::cout<<results[1]<<std::endl;
 // send the vector to stdout.
    std::ostream_iterator<std::string> oit(std::cout);
   std::copy(results.begin(), results.end(), oit);
  }
