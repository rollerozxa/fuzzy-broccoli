#pragma once

#include "entity.hh"

class ball : public entity
{
  private:
    int btype;

  public:
    ball();

    void add_to_world();
    const char* get_name(){
        return "Ball";
    };
};
