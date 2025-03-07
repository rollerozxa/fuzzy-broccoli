#include "iomiscgate.hh"
#include "model.hh"
#include "material.hh"

i1o2gate::i1o2gate()
{
    this->set_mesh(mesh_factory::get_mesh(MODEL_I1O2));
    this->set_material(&m_iomisc);

    this->num_s_in = 1;
    this->num_s_out = 2;

    this->s_in[0].lpos = b2Vec2(.0f, -.125f);
    this->s_out[0].lpos = b2Vec2(.125f, .125f);
    this->s_out[1].lpos = b2Vec2(-.125f, .125f);

    this->set_as_rect(.275f, .275f);
}
