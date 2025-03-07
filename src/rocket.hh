#pragma once

#include "entity.hh"
#include "edevice.hh"

class flame_effect;

class rocket : public ecomp_multiconnect, b2QueryCallback
{
  private:
    connection c;
    float thrustmul;
    flame_effect *flames;

  public:
    rocket();

    edevice* solve_electronics(void);
    //void add_to_world();

    void step();
    const char *get_name(){return "Rocket";};

    float get_slider_snap(int s);
    float get_slider_value(int s);
    const char *get_slider_label(int s){return "Thrust";};
    void on_slider_change(int s, float value);
    //void update_effects();
    void setup();
    void on_pause();
    void on_absorb();
    bool ReportFixture(b2Fixture *f);
    void set_thrustmul(float thrustmul);

    void write_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::write_state(lvl, lb);

        lb->w_s_float(this->thrustmul);
    }

    void read_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::read_state(lvl, lb);

        this->thrustmul = lb->r_float();
    }
};
