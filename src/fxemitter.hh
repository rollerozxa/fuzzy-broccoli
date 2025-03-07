#pragma once

#include "entity.hh"

#include <set>

#define FX_EXPLOSION   0
#define FX_HIGHLIGHT   1
#define FX_DESTROYCONN 2
#define FX_SMOKE       3
#define FX_MAGIC       4
#define FX_BREAK       5

#define FX_INVALID 0xdeadbeef

#define FXEMITTER_NUM_EFFECTS 4

#define DISCHARGE_MAX_POINTS 10

#define NUM_SPARKS 3

#define SPARK_DECAY_RATE 2.0f

#define NUM_FIRES 20
#define EXPLOSION_FIRE_DECAY_RATE 2.5f
#define EXPLOSION_DECAY_RATE 4.0f

#define NUM_SMOKE_PARTICLES 3

#define NUM_BREAK_PARTICLES 3
#define BREAK_DECAY_RATE 2.5f

#define NUM_FLAMES 30

class base_effect : public entity
{
  public:
    const char *get_name() { return "Base effect"; }
    base_effect() : entity()
    {
        this->set_flag(ENTITY_IS_OWNED,             true);
        this->set_flag(ENTITY_DO_MSTEP,             true);
        this->set_flag(ENTITY_DO_UPDATE_EFFECTS,    true);
        this->set_flag(ENTITY_FADE_ON_ABSORB,       false);
    }

    void add_to_world(){};
};

struct particle {
    float x;
    float y;
    float z;
    float life;
    float s;
    float a;
};

struct piece {
    float x; float y;
    float vx; float vy;
    float life;
};

struct flame {
    float x;
    float y;
    float vx;
    float vy;
    float life;
    float s;
    float a;
};

class debris : public entity
{
  private:
    b2Vec2 initial_force;
    float life;
  public:
    debris(b2Vec2 force, b2Vec2 pos);
    const char *get_name() { return "Debris"; }
    void add_to_world();
    void step(void);
};

class explosion_effect : public base_effect
{
  private:
    float scale;
    float       life;
    struct particle particles[NUM_FIRES];
    /*struct piece pieces[5];*/
    b2Vec2      trigger_point;
    bool   played_sound;

  public:
    explosion_effect(b2Vec2 pos, int layer, bool with_debris=true, float scale=1.f);
    void mstep();
    void update_effects();
};


class break_effect : public base_effect
{
  private:
    struct piece pieces[3];
    b2Vec2 trigger_point;

  public:
    break_effect(b2Vec2 pos, int layer);
    void mstep();
    void update_effects();
};

class discharge_effect : public base_effect
{
  private:
    b2Vec2 p[2];
    float  life;
    float  start_z;
    float  end_z;
    float  shift_dir;
    float  displ[DISCHARGE_MAX_POINTS];
    int    num_points;

  public:
    float  line_width;

    discharge_effect(b2Vec2 start, b2Vec2 end, float start_z, float end_z, int num_points, float life);
    void set_points(b2Vec2 start, b2Vec2 end, float start_z, float end_z);
    void mstep();
    void update_effects();
};

class flame_effect : public base_effect
{
  private:
      struct flame flames[NUM_FLAMES];
      b2Vec2 v;
      int flame_n;
      float sep;
      float thrustmul;
      float z_offset;
      bool disable_sound;

  public:
      bool done;

      flame_effect(b2Vec2 pos, int layer, bool disable_sound=false);
      void update_pos(b2Vec2 pos, b2Vec2 v);
      void step();
      void update_effects();
      void set_thrustmul(float thrustmul);
      void set_z_offset(float z_offset);
};
