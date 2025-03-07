#pragma once

#include "iomiscgate.hh"

class ysplitter : public i1o2gate
{
  public:
    edevice* solve_electronics();
    const char *get_name(void){return "Y-splitter";};
};
