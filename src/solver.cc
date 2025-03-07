#include "solver.hh"
#include "entity.hh"
#include <tms/bindings/cpp/cpp.hh>

#define NUM_HANDLERS 13

static void (*presolve_handler[13][13])(b2Contact *contact, entity *a, entity *b, int rev, const b2Manifold *man) =
{
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};

static void (*begin_handler[13][13])(b2Contact *contact, entity *a, entity *b, int rev) =
{
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};

static void (*end_handler[13][13])(b2Contact *contact, entity *a, entity *b, int rev) =
{
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};

void solver::BeginContact(b2Contact *contact)
{
    b2Fixture *a = contact->GetFixtureA();
    b2Fixture *b = contact->GetFixtureB();

    if (a->IsSensor()) {
        if (b->IsSensor()) {
            entity *ea = static_cast<entity*>(a->GetUserData());
            if (ea) {
                ea->on_paused_touch(a, b);
            }
            entity *eb = static_cast<entity*>(b->GetUserData());
            if (eb) {
                eb->on_paused_touch(b, a);
            }
            return;
        }

        entity *ea = static_cast<entity*>(a->GetUserData());
        if (ea) {
            ea->on_paused_touch(a, b);
        }

        return;
    } else if (b->IsSensor()) {
        if (a->IsSensor()) {
            entity *ea = static_cast<entity*>(a->GetUserData());
            if (ea) {
                ea->on_paused_touch(a, b);
            }
            entity *eb = static_cast<entity*>(b->GetUserData());
            if (eb) {
                eb->on_paused_touch(b, a);
            }
            return;
        }

        entity *ea = static_cast<entity*>(b->GetUserData());
        if (ea)
            ea->on_paused_touch(b, a);

        return;
    }

    entity *ea, *eb;

    if ((ea = static_cast<entity*>(a->GetUserData())) && (eb = static_cast<entity*>(b->GetUserData())) ){
        int rev = 0;
        if (ea->type > eb->type) {
            entity *tmp = ea;
            ea = eb;
            eb = tmp;
            rev = 1;
        }

        if (ea->type < 13 && eb->type < 13 && begin_handler[ea->type][eb->type]) {
            begin_handler[ea->type][eb->type](contact, ea, eb, rev);
        }
    }
}

void solver::EndContact(b2Contact *contact)
{
    b2Fixture *a = contact->GetFixtureA();
    b2Fixture *b = contact->GetFixtureB();

    if (a->IsSensor()) {
        if (b->IsSensor())
            return;

        entity *ea = static_cast<entity*>(a->GetUserData());
        if (ea) {
            ea->on_paused_untouch(a, b);
        }

        return;
    } else if (b->IsSensor()) {
        entity *ea = static_cast<entity*>(b->GetUserData());
        if (ea)
            ea->on_paused_untouch(b, a);

        return;
    }

    entity *ea, *eb;

    if ((ea = static_cast<entity*>(a->GetUserData())) && (eb = static_cast<entity*>(b->GetUserData())) ){
        int rev = 0;
        if (ea->type > eb->type) {
            entity *tmp = ea;
            ea = eb;
            eb = tmp;
            rev = 1;
        }

        if (end_handler[ea->type][eb->type]) {
            end_handler[ea->type][eb->type](contact, ea, eb, rev);
        }
    }
}

void solver::PreSolve(b2Contact *contact, const b2Manifold *manifold)
{
    b2Fixture *a = contact->GetFixtureA();
    b2Fixture *b = contact->GetFixtureB();

    entity *ea, *eb;

    if ((ea = static_cast<entity*>(a->GetUserData())) && (eb = static_cast<entity*>(b->GetUserData())) ){
        int rev = 0;
        if (ea->type > eb->type) {
            entity *tmp = ea;
            ea = eb;
            eb = tmp;
            rev = 1;
        }

        //if (ea && eb && ea->body && eb->body && ea->type < NUM_HANDLERS && eb->type < NUM_HANDLERS && ea->type > 0 && eb->type > 0) {
            if (presolve_handler[ea->type][eb->type]) {
                presolve_handler[ea->type][eb->type](contact, ea, eb, rev, manifold);
            }
        //}
    }
}

void solver::PostSolve(b2Contact *contact, const b2ContactImpulse *impulse)
{

}
