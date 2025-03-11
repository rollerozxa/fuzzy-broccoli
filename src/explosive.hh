#pragma once

#include "entity.hh"
#include <Box2D/Box2D.h>

#define EXPLOSIVE_MAX_HP 25.f

class explosive : public entity, public b2QueryCallback
{
  private:
    float  hp;
    uint64_t time;

    b2Fixture *found;
    b2Vec2     found_pt;

    void trigger(void);

  public:
    explosive();

    void init();
    void setup();

    void pre_step(void);

    void add_to_world();

    const char* get_name(){
        return "Land mine";
    }

    void damage(float amount)
    {
        if (this->hp > 0.f) {
            this->hp -= amount;
        }
    }

    bool ReportFixture(b2Fixture *f);

    float get_slider_snap(int s);
    float get_slider_value(int s);
    void on_slider_change(int s, float value);
    const char *get_slider_label(int s)
    {
        if (s == 0) {
            return "Threshold";
        } else {
            return "Damage";
        }
    };

    bool triggered;
    uint64_t trigger_time;
};
