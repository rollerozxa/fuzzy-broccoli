#include "fxemitter.hh"
#include "game.hh"
#include "model.hh"
#include "world.hh"
#include "spritebuffer.hh"
#include "linebuffer.hh"
#include "soundmanager.hh"
#include "ui.hh"

#define DEBRIS_FORCE 300.f

debris::debris(b2Vec2 force, b2Vec2 pos)
{
    this->set_flag(ENTITY_FADE_ON_ABSORB,   false);
    this->set_flag(ENTITY_DO_STEP,          true);

    this->life = 800000 + rand()%700000;
    this->g_id = 0;
    this->_pos = pos;
    this->_angle = 0;
    this->set_mesh(mesh_factory::get_mesh(MODEL_DEBRIS));
    this->set_material(&m_wmotor);
    this->initial_force = force;
}

void
debris::step(void)
{
    this->life -= G->timemul(WORLD_STEP);
    if (this->life <= 0)
        G->absorb(this);
}

void
debris::add_to_world()
{
    this->create_circle(this->get_dynamic_type(), .05f, this->material);
    this->body->ApplyForce(this->initial_force, this->_pos);
}

explosion_effect::explosion_effect(b2Vec2 pos, int layer, bool with_debris, float scale) : base_effect()
{
    this->_pos = pos;
    this->trigger_point = pos;
    this->set_layer(layer);
    this->cull_effects_method = CULL_EFFECTS_BY_POSITION;
    this->update_method = ENTITY_UPDATE_NULL;
    this->scale = scale;

    for (int x=0; x<NUM_FIRES; x++) {
        struct particle *f = &this->particles[x];
        f->a = (float)(rand()%100) / 100.f * M_PI*2.f;
        f->life = (.5f + (rand()%100) / 100.f)*scale;
        f->x = trigger_point.x + (-.75f + (rand()%100) / 100.f * 1.5f)*scale;
        f->y = trigger_point.y + (-.75f + (rand()%100) / 100.f * 1.5f)*scale;
        f->z = this->get_layer()*LAYER_DEPTH + (rand()%100) / 150.f + (1.f-scale);
        f->s = (.75f + rand()%100 / 50.f)*scale;
    }

    /*
    for (int x=0; x<5; x++) {
        struct piece *p = &this->pieces[x];

        p->x = trigger_point.x;
        p->y = trigger_point.y;

        float a = (rand()%100) / 100.f * M_PI*2.f;

        p->vx = 15.f * cosf(a);
        p->vy = 15.f * sinf(a);
        p->life = 0.1f + (rand()%100)/100.f * .25f;
    }
    */

    if (with_debris) {
        float rr = 0.f;
        for (int x=0; x<3; x++) {

            rr += .25f + (rand()%100) / 100.f;
            //float a = ((M_PI*2.f)/(float)100.f) * (rand()%100);
            float a = rr;
            b2Vec2 r = b2Vec2(cosf(a), sinf(a));

            r.x*=DEBRIS_FORCE/12.f;
            r.y*=DEBRIS_FORCE/12.f;
            debris *d = new debris(r, this->trigger_point);
            d->set_layer(layer);
            G->emit(d, this);
        }
    }

    this->life = 1.f;
    this->played_sound = false;
}

void
explosion_effect::mstep()
{
    int num_active = 0;
    for (int x=0; x<NUM_FIRES; x++) {
        struct particle *f = &this->particles[x];
        float s_life = .2f;
        if (f->life > -s_life) {
            f->life -= G->timemul(WORLD_STEP) * 0.000001f * EXPLOSION_FIRE_DECAY_RATE;

            ++num_active;
        }
    }

    if (life > 0.f) {
        this->life -= G->timemul(WORLD_STEP) *  0.000001f * EXPLOSION_DECAY_RATE;
    }

    if (num_active == 0) {
        G->lock();
        G->absorb(this);
        G->unlock();
    }
}

void
explosion_effect::update_effects()
{
    if (!played_sound) {
        if (scale < .75f) {
            G->play_sound(SND_EXPLOSION_LIGHT, this->trigger_point.x, this->trigger_point.y, rand(), 1.f);
        } else {
            G->play_sound(SND_EXPLOSION, this->trigger_point.x, this->trigger_point.y, rand(), 1.f);
        }
        played_sound = true;
    }

    for (int x=0; x<NUM_FIRES; ++x) {
        struct particle *f = &this->particles[x];
        float s_life = .2f;
        if (f->life > -s_life) {
            if (x > NUM_FIRES/4)
                spritebuffer::add(f->x, f->y, f->z,
                        2.f*f->life, 2.f*f->life, 1.f*f->life, 1.f*(f->life+s_life),
                        f->s, f->s, 0, f->a);
            else
                spritebuffer::add2(f->x, f->y, f->z,
                        2.f*f->life, 2.f*f->life, 1.f*f->life, 1.f*(f->life+s_life),
                        f->s, f->s, 0, f->a);
        }
    }

    /*
    // emit shrapnel
    for (int x=0; x<5; x++) {
        struct piece *f = &this->pieces[x];
        if (f->life > -0.1f) {
            spritebuffer::add(f->x, f->y, this->prio*LAYER_DEPTH+1.f,
                    5.0f*f->life, 5.0f*f->life, 4.0f*f->life, f->life+.1f,
                    .18f, .18f, 0);
            f->life-=_tms.dt;

            f->x += f->vx * _tms.dt;
            f->y += f->vy * _tms.dt;
            num_active ++;
        }
    }
    */

    if (this->life > 0.f && this->scale > .75f) {
        spritebuffer::add(this->trigger_point.x, this->trigger_point.y, this->get_layer()*LAYER_DEPTH+0.5f,
                1.f, 1.f, 1.f, .6f*life,
                7.f, 7.f, 1);
    }
}

break_effect::break_effect(b2Vec2 pos, int layer)
{
    this->_pos = pos;
    this->trigger_point = pos;
    this->cull_effects_method = CULL_EFFECTS_BY_POSITION;
    this->update_method = ENTITY_UPDATE_NULL;
    this->prio = layer;

    for (int x=0; x<NUM_BREAK_PARTICLES; x++) {
        struct piece *p = &this->pieces[x];

        p->x = trigger_point.x;
        p->y = trigger_point.y;

        float a = (rand()%100) / 100.f * M_PI*2.f;

        p->vx = 5.f * cosf(a);
        p->vy = 5.f * sinf(a);
        p->life = 0.5f + (rand()%100)/100.f * .5f;
    }
}

void
break_effect::mstep()
{
    int num_active = 0;
    for (int x=0; x<NUM_BREAK_PARTICLES; x++) {
        struct piece *f = &this->pieces[x];
        if (f->life > 0.f) {
            f->life -= G->timemul(WORLD_STEP) * 0.000001f * BREAK_DECAY_RATE;

            f->x += G->timemul(WORLD_STEP) * 0.000001f * f->vx;
            f->y += G->timemul(WORLD_STEP) * 0.000001f * f->vy;

            ++ num_active;
        }
    }

    if (num_active == 0) {
        G->lock();
        G->absorb(this);
        G->unlock();
    }
}

void
break_effect::update_effects()
{
    for (int x=0; x<NUM_BREAK_PARTICLES; x++) {
        struct piece *f = &this->pieces[x];
        if (f->life > 0.f) {
            spritebuffer::add(f->x, f->y, this->prio*LAYER_DEPTH+1.f,
                    0.2f, 0.2f, 0.2f, f->life,
                    .08f, .08f, 0);
        }
    }
}

flame_effect::flame_effect(b2Vec2 pos, int layer, bool _disable_sound/*=false*/)
    : base_effect()
    , disable_sound(_disable_sound)
{
    this->_pos = pos;
    this->set_layer(layer);
    this->thrustmul = 0.f;
    this->done = false;
    this->flame_n = 0;
    this->thrustmul = 1.f;
    this->z_offset = 0.f;
    this->sep = 0.f;

    this->cull_effects_method = CULL_EFFECTS_DISABLE;
    this->update_method = ENTITY_UPDATE_NULL;

    this->set_flag(ENTITY_DO_MSTEP, false);
    this->set_flag(ENTITY_DO_STEP, true);

    memset(this->flames, 0, NUM_FLAMES*sizeof(struct flame));
}

void
flame_effect::update_pos(b2Vec2 pos, b2Vec2 v)
{
    this->sep = b2Distance(this->_pos, pos);

    this->_pos.x = pos.x;
    this->_pos.y = pos.y;
    this->v.x = v.x;
    this->v.y = v.y;
}

void
flame_effect::step()
{
    bool dead = true;
    for (int x=0; x<NUM_FLAMES; ++x) {
        if (this->flames[x].life > 0.f) {
            dead = false;
            break;
        }
    }

    if (this->done && dead) {
        if (!this->disable_sound) {
            sm::stop(&sm::rocket, this);
        }

        G->lock();
        G->absorb(this);
        G->unlock();
    } else {
        if (this->flames[this->flame_n%NUM_FLAMES].life <= 0.f && this->thrustmul > 0.f && !this->done) {
            this->flames[this->flame_n%NUM_FLAMES].a = (float)(rand()%100) / 100.f * M_PI*2.f;
            this->flames[this->flame_n%NUM_FLAMES].x = this->_pos.x;
            this->flames[this->flame_n%NUM_FLAMES].y = this->_pos.y;
            this->flames[this->flame_n%NUM_FLAMES].vx = this->v.x;
            this->flames[this->flame_n%NUM_FLAMES].vy = this->v.y;
            this->flames[this->flame_n%NUM_FLAMES].s = .5f + ((rand()%100) / 100.f)*.5f;

            this->flames[this->flame_n%NUM_FLAMES].life = 1.f * std::max(1.f-(this->sep / 2.f), 0.1f);

            this->flame_n++;
        }

        if (!this->disable_sound) {
            b2Vec2 pos = this->get_position();

            if (this->thrustmul > 0.f) {
                const float vol = tclampf(this->thrustmul+.3f, 0.f, 1.f);

                G->play_sound(SND_ROCKET, pos.x, pos.y, 0, vol, true, this);
            } else {
                sm::stop(&sm::rocket, this);
            }
        }
    }
}

void
flame_effect::update_effects()
{
    for (int x=0; x<NUM_FLAMES; x++) {
        struct flame *f = &this->flames[x];
        if (f->life > 0.f) {
            spritebuffer::add2(f->x, f->y, this->get_layer()*LAYER_DEPTH+this->z_offset,
                    2.f*f->life, 2.f*f->life, 1.f*f->life, 1.f*f->life,
                    f->s, f->s, 0, f->a);
            //f->x+=f->vx *_tms.dt*4.f * G->get_time_mul();
            //f->y+=f->vy *_tms.dt*4.f * G->get_time_mul();
            f->life-=_tms.dt*5.f;
        }
    }

    if (this->thrustmul > 0.f) {
        b2Vec2 p = this->local_to_world(b2Vec2(0.f,0.f), 0.f);
        float s = 1.8f+(rand()%100)/100.f * .2f;
        spritebuffer::add(p.x, p.y, this->get_layer()*LAYER_DEPTH+this->z_offset,
                1.f, 1.f, 1.f, .25f,//.25f+(rand()%100)/100.f * .5f,
                s,s, 2);

        if (this->done) {
            this->thrustmul *= .95f;
        }
    }
}

void
flame_effect::set_thrustmul(float thrustmul)
{
    this->thrustmul = thrustmul;
}

void
flame_effect::set_z_offset(float z_offset)
{
    this->z_offset = z_offset;
}
