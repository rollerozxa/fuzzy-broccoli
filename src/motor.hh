#pragma once

#include "edevice.hh"

#define MOTOR_DIR_LEFT  0
#define MOTOR_DIR_RIGHT 1

#define MOTOR_TYPE_SIMPLE  2

class motor : public ecomp, public b2QueryCallback
{
  private:
    connection c;
    connection c_side[4];
    b2Vec2 q_point;
    entity *q_result;
    b2Fixture *q_fx;
    uint8_t q_frame;
    int mtype;

  public:
    motor(int mtype);
    const char *get_name(){
         return "Simple Motor";

    };
    void on_load(bool created);
    void toggle_axis_rot();
    bool allow_connection(entity *asker, uint8_t fr, b2Vec2 p);
    void find_pairs();
    void connection_create_joint(connection *c);
    bool ReportFixture(b2Fixture *f);
    connection * load_connection(connection &conn);

    struct tms_sprite* get_axis_rot_sprite();
    const char* get_axis_rot_tooltip();

    float get_slider_snap(int s){return .05f;};
    float get_slider_value(int s){return this->properties[s*3].v.f;};
    void on_slider_change(int s, float value);
    const char * get_slider_label(int s){return "Speed vs Torque";}

    edevice* solve_electronics();

    bool get_dir(){return this->properties[2].v.i == 0;};
    void toggle_dir(){this->properties[2].v.i = !this->properties[2].v.i;};
};
