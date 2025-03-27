#include "process.h"

#include <unistd.h>

#include <cctype>
#include <sstream>
#include <string>
#include <vector>

using std::string;
using std::to_string;
using std::vector;

// TODO: Return this process's ID
int Process::Pid() { return this->pid_; }

// TODO: Return this process's CPU utilization
float Process::CpuUtilization() { return this->cpu_utilization_; }

// TODO: Return the command that generated this process
string Process::Command() { return this->cmd_; }

// TODO: Return this process's memory utilization
string Process::Ram() { return this->ram_; }

// TODO: Return the user (name) that generated this process
string Process::User() { return this->user_; }

// TODO: Return the age of this process (in seconds)
long int Process::UpTime() { return this->up_time_; }

// TODO: Overload the "less than" comparison operator for Process objects
// REMOVE: [[maybe_unused]] once you define the function
bool Process::operator<(Process const& a) const {
  if(this->cpu_utilization_ < a.cpu_utilization_) {
    return true;
  } 
  return false;
}

Process::Process(int pid) {
  this->pid_ = pid;
  this->user_ = LinuxParser::User(pid);
  this->cmd_ = LinuxParser::Command(pid);
  this->ram_ = LinuxParser::Ram(pid);
  this->up_time_ = LinuxParser::UpTime(pid);

  long hertz = sysconf(_SC_CLK_TCK);
  long second = LinuxParser::UpTime() - (LinuxParser::UpTime(pid) / hertz);
  this->cpu_utilization_ = (float)(LinuxParser::ActiveJiffies(pid) / hertz) / second;
}