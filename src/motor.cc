#include "motor.hh"

#include "game.hh"
#include "world.hh"
#include "model.hh"
#include "material.hh"
#include "gui.hh"

#define SPEED 1.25f
#define TORQUE 40.f

motor::motor(int mtype)
{
    this->mtype = mtype;

    this->layer_mask = 7;

    tmat4_load_identity(this->M);
    tmat3_load_identity(this->N);

    this->set_num_properties(4);
    this->properties[0].v.f = .5f;   /* Speed/force tradeoff */
    this->properties[0].type = P_FLT;
    this->properties[1].v.f = 0;     /* Reference angle */
    this->properties[1].type = P_FLT;
    this->properties[2].v.i = 0;     /* Motor dir */
    this->properties[2].type = P_INT;

    this->properties[3].type = P_FLT; /* speed cap */
    this->properties[3].v.f = .5f;

    this->num_s_out = 0;

    this->do_solve_electronics = true;
    this->set_mesh(mesh_factory::get_mesh(MODEL_SIMPLEMOTOR));
    this->set_material(&m_misc);

    this->num_s_in = 2;

    this->s_in[0].lpos = b2Vec2(-0.125f,.25f);
    this->s_in[0].ctype = CABLE_BLACK;
    this->s_in[0].angle = M_PI/2.f;

    this->s_in[1].lpos = b2Vec2(0.125f,.25f);
    this->s_in[1].ctype = CABLE_RED;
    this->s_in[1].angle = M_PI/2.f;
    this->s_in[1].tag = SOCK_TAG_SPEED;

    this->num_sliders = 1;

    this->c.init_owned(0, this);

    this->c_side[0].init_owned(1, this);
    this->c_side[0].type = CONN_GROUP;
    this->c_side[1].init_owned(2, this);
    this->c_side[1].type = CONN_GROUP;
    this->c_side[2].init_owned(3, this);
    this->c_side[2].type = CONN_GROUP;
    this->c_side[3].init_owned(4, this);
    this->c_side[3].type = CONN_GROUP;

    this->set_as_rect(.375f, .375f);
    float qw = this->width/2.f + 0.15f;
    float qh = this->height/2.f + 0.15f;
    this->query_sides[0].Set(0.f,  qh); /* up */
    this->query_sides[1].Set(-qw, 0.f); /* left */
    this->query_sides[2].Set(0.f, -qh); /* down */
    this->query_sides[3].Set( qw, 0.f); /* right */
}

struct tms_sprite*
motor::get_axis_rot_sprite()
{
    return gui_spritesheet::get_sprite(S_MOTOR_AXISROT);
}

const char*
motor::get_axis_rot_tooltip()
{
    if (this->flag_active(ENTITY_AXIS_ROT)) {
        return "Unflatten";
    } else {
        return "Flatten";
    }
}

void
motor::on_load(bool created)
{
    if (this->mtype != MOTOR_TYPE_SIMPLE) {
        this->set_mesh(mesh_factory::get_mesh(this->flag_active(ENTITY_AXIS_ROT) ? MODEL_FLATMOTOR : MODEL_DMOTOR));
        this->layer_mask = this->flag_active(ENTITY_AXIS_ROT) ? 8 : 7;
        this->recreate_shape();
    }
}

void
motor::toggle_axis_rot()
{
    this->set_flag(ENTITY_AXIS_ROT, !this->flag_active(ENTITY_AXIS_ROT));
    this->on_load(false);
}

bool
motor::ReportFixture(b2Fixture *f)
{
    entity *e = static_cast<entity*>(f->GetUserData());

    if (!f->IsSensor() && e && e != this && f->TestPoint(this->q_point)
        && e->allow_connections() && e->allow_connection(this, VOID_TO_UINT8(f->GetBody()->GetUserData()), this->q_point)) {

        if (((this->flag_active(ENTITY_AXIS_ROT) && (e->get_layer() == this->get_layer() || e->get_layer() == this->get_layer()+1)
                        && this->sublayer_dist(e) < 3)
            || (!this->flag_active(ENTITY_AXIS_ROT) && abs(e->get_layer() - this->get_layer()) == 1))
            ) {
            this->q_frame = VOID_TO_UINT8(f->GetBody()->GetUserData());
            this->q_fx = f;
            this->q_result = e;
            return false;
        }
    }

    return true;
}

bool
motor::allow_connection(entity *asker, uint8_t fr, b2Vec2 pt)
{
    /* dont allow cylinders to connect to us, WE connect to THEM */
    if (asker->type == ENTITY_WHEEL) return false;
    return true;
}

void
motor::find_pairs()
{
    if (this->c.pending) {
        b2Vec2 p = this->get_position();
        this->q_result = 0;
        this->q_point = p;
        b2AABB aabb;
        aabb.lowerBound.Set(p.x - .05f, p.y - .05f);
        aabb.upperBound.Set(p.x + .05f, p.y + .05f);
        W->b2->QueryAABB(this, aabb);

        if (this->q_result != 0) {
            this->c.o = this->q_result;
            this->c.p = this->q_point;
            this->c.f[0] = 0;
            this->c.f[1] = this->q_frame;
            this->c.o_data = this->q_result->get_fixture_connection_data(this->q_fx);
            G->add_pair(this, this->q_result, &this->c);
        }
    }

    this->sidecheck4(this->c_side);
}

void
motor::connection_create_joint(connection *c)
{
    b2RevoluteJointDef rjd;
    rjd.collideConnected = true;
    rjd.maxMotorTorque = 0.f;
    rjd.motorSpeed = 0.f;
    rjd.enableMotor = true;
    rjd.localAnchorA = this->local_to_body(b2Vec2(0.f, 0.f), 0);
    /*rjd.referenceAngle = (c->o->get_body(c->f[1])->GetAngle()- this->get_body(0)->GetAngle())
        - this->properties[1].v.f;*/
    rjd.referenceAngle = c->o->get_body(c->f[1])->GetAngle() - c->e->get_body(c->f[0])->GetAngle()
    - (W->level.version >= LEVEL_VERSION_1_5 ? (c->o->get_angle(c->f[1]) - c->e->get_angle(c->f[0]))+c->relative_angle : 0.f)
     - this->properties[1].v.f ;

    if (c->o->type == ENTITY_PLANK && !c->o->gr) {
        rjd.localAnchorB = c->o->world_to_body(this->get_position(), c->f[1]);
        rjd.localAnchorB.y = 0.f;
    } else if (c->o->type == ENTITY_WHEEL) {
        /* get the centroid of the object */
        rjd.localAnchorB = c->o->local_to_body(b2Vec2(0.f,0.f), c->f[1]);
    } else {
        rjd.localAnchorB = c->o->local_to_body(c->p_s, c->f[1]);
    }

    rjd.bodyA = this->get_body(0);
    rjd.bodyB = c->o->get_body(c->f[1]);

    if (rjd.bodyA != rjd.bodyB) {
        c->j = W->b2->CreateJoint(&rjd);
        static_cast<b2RevoluteJoint*>(c->j)->EnableMotor(true);
    }
}

connection *
motor::load_connection(connection &conn)
{
    if (conn.o_index == 0) {
        this->c = conn;
        return &this->c;
    } else if (conn.o_index >= 1 && conn.o_index <= 4) {
        this->c_side[conn.o_index - 1] = conn;
        return &this->c_side[conn.o_index - 1];
    }

    return 0;
}

/* only for simple motor */
edevice*
motor::solve_electronics()
{
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();
    if (!this->s_in[1].is_ready())
        return this->s_in[1].get_connected_edevice();

    float torque = this->s_in[0].get_value();
    float speed = torque * (this->s_in[1].p ? this->s_in[1].get_value() : 1.f);

    speed *= SPEED;
    torque *= TORQUE;

    speed *= 1.f-this->properties[0].v.f;
    torque *= this->properties[0].v.f;

    speed *= (this->properties[2].v.i ? 1.f : -1.f);

    b2RevoluteJoint *j = static_cast<b2RevoluteJoint*>(this->c.j);

    if (j) {
        float s = j->GetJointSpeed();

        if (W->level.version >= LEVEL_VERSION_1_5) {
            if (s/speed > 1.f) {
                torque = W->level.joint_friction;
            }
        } else {
            if (speed == 0.f) {
                torque = 0.f;
            } else {
                if (s/speed > 1.f) {
                    torque = .02f; /* friction */
                }
            }
        }

        j->SetMotorSpeed(speed);
        j->SetMaxMotorTorque(torque);
    }
    return 0;
}

void
motor::on_slider_change(int s, float value)
{
    this->properties[0].v.f = value;
    G->show_numfeed(value - 0.5f);

}
