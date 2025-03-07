#include "activator.hh"
#include "world.hh"
#include "game.hh"

bool
activator::activator_touched(b2Fixture *other)
{
    if (other->IsSensor()) {
        return false;
    }

    entity *e = static_cast<entity*>(other->GetUserData());

    return false;

}

bool
activator::activator_untouched(b2Fixture *other)
{
    if (other->IsSensor()) return false;

    entity *e = static_cast<entity*>(other->GetUserData());

    return false;

}
