#include "material.hh"
#include "ball.hh"
#include "world.hh"
#include "model.hh"

ball::ball()
{
    this->width = .5f;
    this->set_flag(ENTITY_ALLOW_ROTATION, false);
    this->set_flag(ENTITY_ALLOW_CONNECTIONS, false);

    this->type = ENTITY_BALL;

    this->btype = type;

    this->set_mesh(mesh_factory::get_mesh(MODEL_SPHERE));

    this->set_material(&m_wood);

    this->layer_mask = 6;

    tmat4_load_identity(this->M);
    tmat3_load_identity(this->N);
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

