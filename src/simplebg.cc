#include "simplebg.hh"
#include "model.hh"
#include "material.hh"

simplebg::simplebg()
{
    this->set_mesh(static_cast<tms::mesh*>(const_cast<tms_mesh*>(tms_meshfactory_get_square())));
    this->set_material(&m_bg);

    this->set_flag(ENTITY_DO_UPDATE, false);

    tmat4_load_identity(this->M);
    tmat4_translate(this->M, 0, 0, -1.f);
    tmat4_scale(this->M, 200, 200, .0f);
    this->prio = 0;

    tmat3_load_identity(this->N);
}

bool
simplebg::set_level_size(uint16_t left, uint16_t right, uint16_t down, uint16_t up)
{
    float border_extra_span = 20.f;

    float w = (float)left+(float)right;
    float h = (float)down+(float)up;

    this->set_material(&m_bg);

    if (w < 5.f || h < 5.f) {
        tms_infof("invalid size %f %f %u %u %u %u", w, h, left, right, down, up);

        return false;
    }

    float px = (float)right / 2.f - (float)left/2.f;
    float py = (float)up / 2.f - (float)down/2.f;

    b2Vec2 pos[4] = {
        b2Vec2(right+border_extra_span/2.f, py),
        b2Vec2(px, up+border_extra_span/2.f),
        b2Vec2(-left-border_extra_span/2.f, py),
        b2Vec2(px, -down-border_extra_span/2.f)
    };

    b2Vec2 size[4] = {
        b2Vec2(border_extra_span+1.f, h+border_extra_span*2.f),
        b2Vec2(w, border_extra_span+1.f),
        b2Vec2(border_extra_span+1.f, h+border_extra_span*2.f),
        b2Vec2(w, border_extra_span+1.f),
    };

    tmat4_load_identity(this->M);
    tmat4_translate(this->M, px, py, -.5f);
    tmat4_scale(this->M, w, h, 1.f);

    tmat3_load_identity(this->N);

    return true;
}


void
simplebg::set_color(tvec4 c)
{
    this->set_uniform("~color", TVEC4_INLINE(c));
}
