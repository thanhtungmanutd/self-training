#include "format.h"

#include <string>

using std::string;

// TODO: Complete this helper function
// INPUT: Long int measuring seconds
// OUTPUT: HH:MM:SS
// REMOVE: [[maybe_unused]] once you define the function
string Format::ElapsedTime(long seconds) {
  char buffer[50];
  int hrs, mins;
  
  hrs = seconds / 3600;
  mins = (seconds - hrs * 3600) / 60;
  snprintf(buffer, 50, "%02d:%02d:%02d",
              hrs, mins, (int)(seconds%60));
  string s = buffer;
  return s;
}