#include "material.hh"
#include "ball.hh"
#include "world.hh"
#include "model.hh"
#include "game.hh"


ball::ball(int type)
{
    this->width = .5f;
    this->set_flag(ENTITY_ALLOW_ROTATION, false);
    this->set_flag(ENTITY_ALLOW_CONNECTIONS, false);

    this->type = ENTITY_BALL;

    this->btype = type;

    this->saved_layer = 0;
    this->layer_new = 0;
    this->layer_old = 0;
    this->layer_blend = 1.f;

    this->set_mesh(mesh_factory::get_mesh(MODEL_SPHERE));

    this->set_material(&m_wood);

    this->layer_mask = 6;

    tmat4_load_identity(this->M);
    tmat3_load_identity(this->N);
}

void
ball::on_load(bool created)
{
    this->saved_layer = this->get_layer();
    this->layer_new = this->get_layer();
    this->layer_old = this->get_layer();
    this->layer_blend = 1.f;
}

void
ball::setup()
{
    this->saved_layer = this->get_layer();

    this->layer_new = this->get_layer();
    this->layer_old = this->get_layer();
    this->layer_blend = 1.f;

    this->initialize_interactive();
}

void
ball::on_pause()
{
    this->set_layer(this->saved_layer);
    this->layer_new = this->get_layer();
    this->layer_old = this->get_layer();
    this->layer_blend = 1.f;
}

void
ball::construct()
{
    this->saved_layer = this->get_layer();
}

void
ball::set_layer(int l)
{
    entity::set_layer(l);

    if (W->is_paused()) {
        this->layer_new = this->get_layer();
        this->layer_old = this->get_layer();
        this->layer_blend = 1.f;
    }
}

void
ball::add_to_world()
{
    if (W->is_paused())
        this->create_circle(this->get_dynamic_type(), .26f, this->material);
    else
        this->create_circle(this->get_dynamic_type(), .25f, this->material);

    ((struct tms_entity*)this)->update = tms::_oopassthrough_entity_update;
}

