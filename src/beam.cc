#include "beam.hh"
#include "material.hh"
#include "model.hh"
#include "game.hh"

class beam_ray_cb : public b2RayCastCallback
{
  public:
    entity *result;
    beam *ignore;
    b2Vec2 result_point;
    uint8_t result_frame;
    b2Fixture *result_fx;
    b2Vec2 vec;
    int dir;

    beam_ray_cb(beam *ignore)
    {
        this->ignore = ignore;
        result = 0;
    }

    float32 ReportFixture(b2Fixture *f, const b2Vec2 &pt, const b2Vec2 &nor, float32 fraction)
    {
        if (f->IsSensor()) {
            return -1.f;
        }

        entity *r = (entity*)f->GetUserData();

        float ret = 1.f;

        if (r && r != this->ignore) {
            /* TODO: verify type of entity */

            connection *c = 0;

            if ((f->GetFilterData().categoryBits & ignore->fx->GetFilterData().categoryBits) != 0) {
                c = (this->dir == 0 ? &ignore->c[0] : &ignore->c[1]);
                if (c->pending) {
                    this->result = r;
                    this->result_fx = f;
                    this->result_frame = VOID_TO_UINT8(f->GetBody()->GetUserData());
                    this->result_point = pt;
                }
                ret = fraction;
            } else if (r->get_layer() == ignore->get_layer()+1
                    /* && (r->group == 0 ||
                     r->group != ignore->group
                     )*/) { /* XXX */

                if (r->type == ENTITY_PLANK) {
                    b2Vec2 p = pt + vec;

                    if (ignore->fx->TestPoint(p)) {
                        c = G->get_tmp_conn();
                        c->type = CONN_GROUP;
                        c->typeselect = 1;
                        c->p = pt + vec;
                        c->e = ignore;
                        c->o = r;
                        c->o_data = r->get_fixture_connection_data(f);

                        if (W->level.flag_active(LVL_NAIL_CONNS)) {
                            c->render_type = CONN_RENDER_SMALL;
                        } else {
                            c->render_type = CONN_RENDER_DEFAULT;
                        }

                        c->f[1] = VOID_TO_UINT8(f->GetBody()->GetUserData());

                        if (!G->add_pair(ignore, r, c)) {
                            G->return_tmp_conn(c);
                        }
                    }
                }
            }
        }

        return ret;
    }
};

beam::beam(int btype)
{
    this->do_update_fixture = false;
    this->btype = btype;
    this->type = ENTITY_PLANK;
    this->num_sliders = 1;
    this->menu_scale = .25f;

    this->set_flag(ENTITY_IS_MOVEABLE,  true);
    this->set_flag(ENTITY_DO_TICK,      true);
    this->set_flag(ENTITY_IS_BEAM,      true);

    this->set_num_properties(1); /* 1 property for the size */
    this->set_mesh(mesh_factory::get_mesh(MODEL_THINPLANK4));
    this->set_material(&m_wood);

    this->set_property(0, (uint32_t)3);

    this->c[0].init_owned(0, this);
    this->c[0].type = CONN_GROUP;
    this->c[0].angle = M_PI;
    this->c[1].init_owned(1, this);
    this->c[1].angle = 0.f;
    this->c[1].type = CONN_GROUP;

    tmat4_load_identity(this->M);
    tmat3_load_identity(this->N);

    this->layer_mask = 6;

    this->update_fixture();
}

connection*
beam::load_connection(connection &conn)
{
    this->c[conn.o_index] = conn;
    this->c[0].angle = M_PI;
    this->c[1].angle = 0.f;
    return &this->c[conn.o_index];
}

void
beam::on_load(bool created)
{
    this->update_fixture();
}

void
beam::update_fixture()
{
    uint32_t size = this->properties[0].v.i;
    if (size > 3) size = 3;
    this->properties[0].v.i = (uint32_t)size;

    this->set_property(0, (uint32_t)size);

    this->set_mesh(mesh_factory::models[MODEL_THINPLANK1+size].mesh);

    this->set_as_rect(((float)size+1.f)/2.f, .125f);

    if (this->body) {
        this->recreate_shape();
    }
}

void
beam::tick()
{
    if (this->do_update_fixture) {
        this->update_fixture();
    }
}

void
beam::on_slider_change(int s, float value)
{
    uint32_t size = (uint32_t)roundf(value * 3.f);
    G->animate_disconnect(this);
    this->disconnect_all();
    this->set_property(0, size);

    this->update_fixture();
    G->show_numfeed(1.f+size);
}

void
beam::find_pairs()
{
    beam_ray_cb handler(this);

    for (int x=0; x<2; x++) {
        b2Vec2 dir[2];
        float sign = (x == 0 ? 1.f : -1.f);
        dir[0] = b2Vec2(-0.25f*sign, 0.f);
        dir[1] = b2Vec2((this->width + .25f)*sign, 0.f);

        b2Vec2 p1 = this->local_to_world(dir[0], 0);
        b2Vec2 p2 = this->local_to_world(dir[1], 0);

        handler.result = 0;
        handler.dir = x;
        handler.vec = p2 - this->get_position();
        handler.vec *= 1.f/handler.vec.Length();
        handler.vec *= .15f;

        W->raycast(&handler, p1, p2);

        if (handler.result && this->c[x].pending) {
            this->c[x].type = CONN_GROUP;
            this->c[x].o = handler.result;
            this->c[x].p = handler.result_point;
            this->c[x].f[1] = handler.result_frame;
            this->c[x].o_data = handler.result->get_fixture_connection_data(handler.result_fx);

            if (W->level.flag_active(LVL_NAIL_CONNS)) {
                this->c[x].render_type = CONN_RENDER_NAIL;
            } else {
                this->c[x].render_type = CONN_RENDER_DEFAULT;
            }
            G->add_pair(this, handler.result, &this->c[x]);
        }
    }
}
