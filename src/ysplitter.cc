#include "ysplitter.hh"
#include "game.hh"

edevice*
ysplitter::solve_electronics()
{
    if (!this->s_in[0].is_ready()) {
        return this->s_in[0].get_connected_edevice();
    }

    float v = this->s_in[0].get_value();

    this->s_out[0].write(v);
    this->s_out[1].write(v);

    return 0;
}
