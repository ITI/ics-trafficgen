#ifndef ITI_DNP3APP_H
#define ITI_DNP3APP_H

#include <boost/thread.hpp>
#include <unordered_map>
#include <string>

struct ThreadSafeUserInput {
    boost::shared_mutex _accessMutex;
    int readCount()
    {
      boost::shared_lock<boost::shared_mutex> lock(_accessMutex);
      // do work here, without anyone having exclusive access
      return currentCount;
    }
    std::string readInputStr(int keyCount) {
        boost::shared_lock<boost::shared_mutex> lock(_accessMutex);
        // do work here, without anyone having exclusive access
        if (keyPresses.count(keyCount) != 0)
            return keyPresses[keyCount];
        return '\0';
    }
    void unconditionalWriter(std::string input_str)
    {
      boost::unique_lock<boost::shared_mutex> lock(_accessMutex);
      // exclusive access
      //limit size of cached keys to 10
      if(keyPresses.size() > 10 && keyPresses.count(currentCount+1-10) != 0){
          keyPresses.erase(currentCount+1-10); //erase the lowest index
      }
      currentCount++;
      keyPresses[currentCount]=input_str;
    }
    private:
        int currentCount = 0;
        std::unordered_map<int, std::string> keyPresses;
};

#endif
