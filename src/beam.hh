#pragma once

#include "composable.hh"

#define BEAM_THIN    1

class beam_ray_cb;

class beam : public composable
{
  private:
    connection c[2];
    int btype;

    m rubber_material;

  public:
    beam(int btype);

    const char* get_name(){
        return "Plank";
    }
    void on_load(bool created);

    void find_pairs();
    connection* load_connection(connection &conn);

    float get_slider_snap(int s){if (s==0)return 1.f / 3.f; else return .05f;};
    float get_slider_value(int s) {
        return this->properties[0].v.i / 3.f;
    }
    const char *get_slider_label(int s){return "Size";};
    void on_slider_change(int s, float value);
    void update_fixture();
    void tick();

    int get_beam_type(){return this->btype;};

    bool do_update_fixture;

    friend class beam_ray_cb;
};
