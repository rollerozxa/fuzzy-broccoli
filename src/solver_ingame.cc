#include "solver_ingame.hh"
#include "entity.hh"
#include "game.hh"
#include "explosive.hh"
#include "button.hh"
#include "soundmanager.hh"

#include <tms/bindings/cpp/cpp.hh>

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

bool
enable_emitted_contact(entity *a, entity *b)
{
    /* TODO: Modify the step count if a time multiplier is active. */
    if (a->emitted_by) {
        int emit_delay = EMIT_DELAY_COLLIDE_EMITTER/(G->get_time_mul()+0.0001f);
        if (W->step_count - a->emit_step > emit_delay) {
            //a->emitted_by = 0;
            return true;
        }

        if (a->emitted_by != b->id) {
            //a->emitted_by = 0;
            return true;
        } else
            return false;
    }

    if (b->emitted_by) {
        int emit_delay = EMIT_DELAY_COLLIDE_EMITTER/(G->get_time_mul()+0.0001f);
        if (W->step_count - b->emit_step > emit_delay) {
            //b->emitted_by = 0;
            return true;
        }

        if (b->emitted_by != a->id) {
            //b->emitted_by = 0;
            return true;
        } else
            return false;
    }

    return true;
}

void solver_ingame::BeginContact(b2Contact *contact)
{
    contact->first_contact = true;
    contact->rel_speed = 0.f;

    b2Fixture *a = contact->GetFixtureA();
    b2Fixture *b = contact->GetFixtureB();

    entity *ea = static_cast<entity*>(a->GetUserData());
    entity *eb = static_cast<entity*>(b->GetUserData());

    if (a->IsSensor()) {
        if (ea) {
            G->lock();
            ea->on_touch(a, b);
            G->unlock();
        }
    } else {

    }
    if (b->IsSensor() && eb) {
        G->lock();
        eb->on_touch(b, a);
        G->unlock();
    }

    b2WorldManifold wm;
    contact->GetWorldManifold(&wm);
    b2Vec2 vel1 = a->GetBody()->GetLinearVelocityFromWorldPoint( wm.points[0] );
    b2Vec2 vel2 = b->GetBody()->GetLinearVelocityFromWorldPoint( wm.points[0] );
    b2Vec2 rvel(vel1 - vel2);

    float i;

    if (rvel.x < FLT_EPSILON && rvel.y < FLT_EPSILON) {
        i = 0.f;
    } else {
        i = rvel.Length();
    }
    contact->rel_speed = i;

    if (ea && eb) {
        if (ea->flag_active(ENTITY_IS_EXPLOSIVE) && eb->flag_active(ENTITY_TRIGGER_EXPLOSIVES)) {
            ((explosive*)ea)->triggered = true;
        }
        if (eb->flag_active(ENTITY_IS_EXPLOSIVE) && ea->flag_active(ENTITY_TRIGGER_EXPLOSIVES)) {
            ((explosive*)eb)->triggered = true;
        }
    }

    /* dont play any sound when an interactive object being dragged hits the player */
    /*
    if (ea && ea->interactive && eb && eb->id == 0xffffffff && ea->in_dragfield && G->interacting_with(ea)) {
        contact->SetEnabled(false);
        return;
    }
    if (eb && eb->interactive && ea && ea->id == 0xffffffff && eb->in_dragfield && G->interacting_with(eb)) {
        contact->SetEnabled(false);
        return;
    }
    */

    if (a->IsSensor() || b->IsSensor())
        return;

    if (ea && eb && !enable_emitted_contact(ea,eb)) {
        contact->SetEnabled(false);
        return;
    }

    i *= .25f;

    if (i > SM_IMPACT_THRESHOLD) {
        if ((ea || eb) && contact->IsEnabled()) {
            m *material_a = 0;
            m *material_b = 0;

            if (ea) {
                material_a = ea->get_material();
            }
            if (eb) {
                material_b = eb->get_material();
            }

            if (!material_a)
                material_a = &m_wood;
            if (!material_b)
                material_b = &m_wood;

            /*
            if (material_a->type == TYPE_METAL || material_a->type == TYPE_SHEET_METAL || material_a->type == TYPE_METAL2 ||
                material_b->type == TYPE_METAL || material_b->type == TYPE_SHEET_METAL || material_b->type == TYPE_METAL2) {
                if (i < (SM_IMPACT_THRESHOLD * 2.f)) {
                    return;
                }
            }
            */

            //tms_debugf("impulse %f %f %f", i, rvel.x, rvel.y);
            float impulse_modifier = 15.f;
            float volume = i / impulse_modifier;

            uint8_t combined_type = material_a->type | material_b->type;

            switch (combined_type) {
                case C_WOOD2: case C_WOOD_WOOD2: case C_METAL_WOOD2:
                case C_PLASTIC_WOOD2: case C_METAL2_WOOD2:
                    G->play_sound(SND_WOOD_HOLLOWWOOD, wm.points[0].x, wm.points[0].y, rand(), volume);
                    break;

                case C_WOOD:
                    G->play_sound(SND_WOOD_WOOD, wm.points[0].x, wm.points[0].y, rand(), volume*2.f);
                    break;

                case C_WOOD_METAL2:
                case C_WOOD_METAL:
                    G->play_sound(SND_WOOD_METAL, wm.points[0].x, wm.points[0].y, rand(), volume);
                    break;
                case C_WOOD_PLASTIC:
                    //tms_infof("TODO: wood on plastic");
                    break;

                case C_METAL_METAL2:
                case C_METAL2:
                    G->play_sound(SND_METAL_METAL2, wm.points[0].x, wm.points[0].y, rand(), volume);
                    break;

                case C_METAL_PLASTIC:
                    //tms_infof("TODO: metal on plastic");
                    break;

                case C_METAL:
                    G->play_sound(SND_METAL_METAL, wm.points[0].x, wm.points[0].y, rand(), volume);
                    break;

                case C_SHEET_METAL_WOOD2:
                case C_SHEET_METAL:
                case C_SHEET_METAL_METAL2:
                case C_WOOD_SHEET_METAL:
                case C_METAL_SHEET_METAL:
                case C_SHEET_METAL_PLASTIC:
                    G->play_sound(SND_SHEET_METAL, wm.points[0].x, wm.points[0].y, rand(), volume);
                    break;

                case C_PLASTIC:
                    //tms_infof("TODO: plastic on plastic");
                    break;

                case C_RUBBER:
                case C_WOOD_RUBBER:
                case C_METAL_RUBBER:
                case C_SHEET_METAL_RUBBER:
                case C_PLASTIC_RUBBER:
                case C_RUBBER_WOOD2:
                case C_RUBBER_METAL2:
                    G->play_sound(SND_RUBBER, wm.points[0].x, wm.points[0].y, 0, volume*.5f);
                    break;

                case C_STONE:
                    G->play_sound(SND_STONE_STONE, wm.points[0].x, wm.points[0].y, rand(), volume*1.5f);
                    break;

                default:
                    //tms_errorf("Undefined audio collision: %d", combined_type);
                    break;
            }

            /* XXX: We're currently just assuming we should use the first point,
             * do we have a reason to change this? */
        }
    }
}

void solver_ingame::EndContact(b2Contact *contact)
{
    contact->first_contact = false;

    b2Fixture *a = contact->GetFixtureA();
    b2Fixture *b = contact->GetFixtureB();

    entity *ea = static_cast<entity*>(a->GetUserData());
    entity *eb = static_cast<entity*>(b->GetUserData());

    if (a->IsSensor() && ea) {
        G->lock();
        ea->on_untouch(a, b);
        G->unlock();
    }
    if (b->IsSensor() && eb) {
        G->lock();
        eb->on_untouch(b, a);
        G->unlock();
    }
}

void solver_ingame::PreSolve(b2Contact *contact, const b2Manifold *manifold)
{
    b2Fixture *a = contact->GetFixtureA();
    b2Fixture *b = contact->GetFixtureB();

    b2WorldManifold wm;
    contact->GetWorldManifold(&wm);

    entity *ea, *eb;

    ea = static_cast<entity*>(a->GetUserData());
    eb = static_cast<entity*>(b->GetUserData());

    if (ea && eb) {

        if (!a->IsSensor() && !b->IsSensor()) {
            if (!enable_emitted_contact(ea, eb)) {
                contact->SetEnabled(false);
                return;
            }
        }

        int rev = 0;
        if (ea->type > eb->type) {
            entity *tmp = ea;
            ea = eb;
            eb = tmp;
            rev = 1;
        }

        if (ea->type < 13 && eb->type < 13) {
            if (presolve_handler[ea->type][eb->type]) {
                presolve_handler[ea->type][eb->type](contact, ea, eb, rev, manifold);
            }
        }
    } else {

    }
}

#define BUTTON_THRESHOLD .75f

void solver_ingame::PostSolve(b2Contact *contact, const b2ContactImpulse *impulse)
{
    b2Fixture *a = contact->GetFixtureA();
    b2Fixture *b = contact->GetFixtureB();

    b2WorldManifold wm;
    contact->GetWorldManifold(&wm);

    entity *ea, *eb;
    ea = static_cast<entity*>(a->GetUserData());
    eb = static_cast<entity*>(b->GetUserData());

    float bullet_modifier = 5.f;

    if (ea) {
        if (ea->g_id == O_BUTTON) {
            button *bt = static_cast<button*>(ea);

            if (bt->switch_fx == a || bt->body->GetLocalVector(wm.normal).y > 0.8f) {
                float i = 0.f;
                for (int x = 0; x < impulse->count; ++x) {
                    i += impulse->normalImpulses[x];
                }

                if (i > BUTTON_THRESHOLD) {
                    bt->press();
                }
            }
        } else if (ea->g_id == O_LAND_MINE) {
            float i = 0.f;
            for (int x = 0; x < impulse->count; ++x)
                i += impulse->normalImpulses[x];

            if (i >= ((explosive*)ea)->properties[0].v.f) {
                ((explosive*)ea)->triggered = true;
            }
        }
    }
    if (eb) {

        if (eb->g_id == O_BUTTON) {
            button *bt = static_cast<button*>(eb);

            b2WorldManifold wm;
            contact->GetWorldManifold(&wm);
            if (bt->switch_fx == b || bt->body->GetLocalVector(wm.normal).y > 0.8f) {
                float i = 0.f;
                for (int x = 0; x < impulse->count; ++x) {
                    i += impulse->normalImpulses[x];
                }

                if (i > BUTTON_THRESHOLD) {
                    bt->press();
                }
            }
        } else if (eb->g_id == O_LAND_MINE) {
            float i = 0.f;
            for (int x = 0; x < impulse->count; ++x)
                i += impulse->normalImpulses[x];

            if (i >= ((explosive*)eb)->properties[0].v.f) {
                ((explosive*)eb)->triggered = true;
            }
        }
    }
}

