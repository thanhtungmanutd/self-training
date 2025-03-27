#include <vector>
#include "processor.h"
#include "linux_parser.h"
#include <iostream>

using namespace LinuxParser;

// TODO: Return the aggregate CPU utilization
float Processor::Utilization() {
  std::string line;
  
  auto total = Jiffies();
  auto idle = IdleJiffies();
  
  auto totald = total - prevtotal;
  auto idled = idle - previdle;
  
  prevtotal = total;
  previdle = idle;
  
  return (float)(totald - idled) / totald;
}