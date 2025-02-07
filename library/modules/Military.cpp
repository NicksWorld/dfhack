#include <string>
#include <vector>
#include <array>
#include "MiscUtils.h"
#include "modules/Military.h"
#include "modules/Translation.h"
#include "modules/Units.h"
#include "df/building.h"
#include "df/building_civzonest.h"
#include "df/histfig_entity_link_former_positionst.h"
#include "df/histfig_entity_link_former_squadst.h"
#include "df/histfig_entity_link_positionst.h"
#include "df/histfig_entity_link_squadst.h"
#include "df/historical_figure.h"
#include "df/historical_entity.h"
#include "df/entity_position.h"
#include "df/entity_position_assignment.h"
#include "df/plotinfost.h"
#include "df/squad.h"
#include "df/squad_position.h"
#include "df/squad_schedule_order.h"
#include "df/squad_order.h"
#include "df/squad_order_trainst.h"
#include "df/unit.h"
#include "df/world.h"

using namespace DFHack;
using namespace df::enums;
using df::global::world;
using df::global::plotinfo;

std::string Military::getSquadName(int32_t squad_id)
{
    df::squad *squad = df::squad::find(squad_id);
    if (!squad)
        return "";
    if (squad->alias.size() > 0)
        return squad->alias;
    return Translation::translateName(&squad->name, true);
}

//only works for making squads for fort mode player controlled dwarf squads
//could be extended straightforwardly by passing in entity
df::squad* Military::makeSquad(int32_t assignment_id)
{
    if (df::global::squad_next_id == nullptr || df::global::plotinfo == nullptr)
        return nullptr;

    df::language_name name;
    name.type = df::language_name_type::Squad;

    for (int i=0; i < 7; i++)
    {
        name.words[i] = -1;
        name.parts_of_speech[i] = df::part_of_speech::Noun;
    }

    df::historical_entity* fort = df::historical_entity::find(df::global::plotinfo->group_id);

    if (fort == nullptr)
        return nullptr;

    df::entity_position_assignment* found_assignment = nullptr;

    for (auto* assignment : fort->positions.assignments)
    {
        if (assignment->id == assignment_id)
        {
            found_assignment = assignment;
            break;
        }
    }

    if (found_assignment == nullptr)
        return nullptr;

    //this function does not attempt to delete or replace squads for assignments
    if (found_assignment->squad_id != -1)
        return nullptr;

    df::entity_position* corresponding_position = nullptr;

    for (auto* position : fort->positions.own)
    {
        if (position->id == found_assignment->position_id)
        {
            corresponding_position = position;
            break;
        }
    }

    if (corresponding_position == nullptr)
        return nullptr;

    df::squad* result = new df::squad();
    result->id = *df::global::squad_next_id;
    result->uniform_priority = result->id + 1; //no idea why, but seems to hold
    result->supplies.carry_food = 2;
    result->supplies.carry_water = df::squad::T_supplies::Water;
    result->entity_id = df::global::plotinfo->group_id;
    result->leader_position = corresponding_position->id;
    result->leader_assignment = found_assignment->id;
    result->name = name;

    int16_t squad_size = corresponding_position->squad_size;

    for (int i=0; i < squad_size; i++)
    {
        //construct for squad_position seems to set all the attributes correctly
        df::squad_position* pos = new df::squad_position();

        result->positions.push_back(pos);
    }

    const auto& routines = df::global::plotinfo->alerts.routines;

    for (const auto& routine : routines)
    {
        df::squad_schedule_entry* asched = (df::squad_schedule_entry*)malloc(sizeof(df::squad_schedule_entry) * 12);

        for(int kk=0; kk < 12; kk++)
        {
            new (&asched[kk]) df::squad_schedule_entry;

            for(int jj=0; jj < squad_size; jj++)
            {
                int32_t* order_assignments = new int32_t();
                *order_assignments = -1;

                asched[kk].order_assignments.push_back(order_assignments);
            }
        }

        auto insert_training_order = [asched, squad_size](int month)
        {
            df::squad_schedule_order* order = new df::squad_schedule_order();
            order->min_count = squad_size;
            //assumed
            order->positions.resize(squad_size);

            df::squad_order* s_order = df::allocate<df::squad_order_trainst>();

            s_order->year = *df::global::cur_year;
            s_order->year_tick = *df::global::cur_year_tick;

            order->order = s_order;

            asched[month].orders.push_back(order);
            //wear uniform while training
            asched[month].uniform_mode = df::squad_civilian_uniform_type::Regular;
        };

        //Dwarf fortress does do this via a series of string comparisons
        //Off duty: No orders, Sleep/room at will. Equip/orders only
        if (routine->name == "Off duty")
        {
            for (int i=0; i < 12; i++)
            {
                asched[i].sleep_mode = df::squad_sleep_option_type::AnywhereAtWill;
                asched[i].uniform_mode = df::squad_civilian_uniform_type::Civilian;
            }
        }
        //Staggered Training: Training orders at months 3 4 5 9 10 11, *or* 0 1 2 6 7 8, sleep/room at will. Equip/orders only, except train months which are equip/always
        else if (routine->name == "Staggered training")
        {
            //this is semi randomised for different squads
            //appears to be something like squad.id & 1, it isn't smart
            //if you alternate squad creation, its 'correctly' staggered
            //but it'll also happily not stagger them if you eg delete a squad and make another
            std::array<int, 6> indices;

            if ((*df::global::squad_next_id) & 1)
            {
                indices = {3, 4, 5, 9, 10, 11};
            }
            else
            {
                indices = {0, 1, 2, 6, 7, 8};
            }

            for (int index : indices)
            {
                insert_training_order(index);
                //still sleep in room at will even when training
                asched[index].sleep_mode = df::squad_sleep_option_type::AnywhereAtWill;
            }
        }
        //see above, but with all indices
        else if (routine->name == "Constant training")
        {
            for (int i=0; i < 12; i++)
            {
                insert_training_order(i);
                //still sleep in room at will even when training
                asched[i].sleep_mode = df::squad_sleep_option_type::AnywhereAtWill;
            }
        }
        else if (routine->name == "Ready")
        {
            for (int i=0; i < 12; i++)
            {
                asched[i].sleep_mode = df::squad_sleep_option_type::InBarracksAtNeed;
                asched[i].uniform_mode = df::squad_civilian_uniform_type::Regular;
            }
        }
        else
        {
            for (int i=0; i < 12; i++)
            {
                asched[i].sleep_mode = df::squad_sleep_option_type::AnywhereAtWill;
                asched[i].uniform_mode = df::squad_civilian_uniform_type::Regular;
            }
        }

        result->schedule.push_back(reinterpret_cast<df::squad::T_schedule*>(asched));
    }

    //Modify necessary world state
    (*df::global::squad_next_id)++;
    fort->squads.push_back(result->id);
    df::global::world->squads.all.push_back(result);
    found_assignment->squad_id = result->id;

    return result;
}

void Military::updateRoomAssignments(int32_t squad_id, int32_t civzone_id, df::squad_use_flags flags)
{
    df::squad* squad = df::squad::find(squad_id);
    df::building* bzone = df::building::find(civzone_id);

    df::building_civzonest* zone = strict_virtual_cast<df::building_civzonest>(bzone);

    if (squad == nullptr || zone == nullptr)
        return;

    df::squad::T_rooms* room_from_squad = nullptr;
    df::building_civzonest::T_squad_room_info* room_from_building = nullptr;

    for (auto room : squad->rooms)
    {
        if (room->building_id == civzone_id)
        {
            room_from_squad = room;
            break;
        }
    }

    for (auto room : zone->squad_room_info)
    {
        if (room->squad_id == squad_id)
        {
            room_from_building = room;
            break;
        }
    }

    if (flags.whole == 0 && room_from_squad == nullptr && room_from_building == nullptr)
        return;

    //if we're setting 0 flags, and there's no room already, don't set a room
    bool avoiding_squad_roundtrip = flags.whole == 0 && room_from_squad == nullptr;

    if (!avoiding_squad_roundtrip && room_from_squad == nullptr)
    {
        room_from_squad = new df::squad::T_rooms();
        room_from_squad->building_id = civzone_id;

        insert_into_vector(squad->rooms, &df::squad::T_rooms::building_id, room_from_squad);
    }

    if (room_from_building == nullptr)
    {
        room_from_building = new df::building_civzonest::T_squad_room_info();
        room_from_building->squad_id = squad_id;

        insert_into_vector(zone->squad_room_info, &df::building_civzonest::T_squad_room_info::squad_id, room_from_building);
    }

    if (room_from_squad)
        room_from_squad->mode = flags;

    room_from_building->mode = flags;

    if (flags.whole == 0 && !avoiding_squad_roundtrip)
    {
        for (int i=0; i < (int)squad->rooms.size(); i++)
        {
            if (squad->rooms[i]->building_id == civzone_id)
            {
                delete squad->rooms[i];
                squad->rooms.erase(squad->rooms.begin() + i);
                i--;
            }
        }
    }
}

static bool remove_soldier_entity_link(df::historical_figure* hf, df::squad* squad)
{
    int32_t start_year = -1;
    for (size_t i = 0; i < hf->entity_links.size(); i++)
    {
        auto link = strict_virtual_cast<df::histfig_entity_link_squadst>(hf->entity_links[i]);
        if (link == nullptr || link->squad_id != squad->id)
            continue;
        
        hf->entity_links.erase(hf->entity_links.begin() + i);
        start_year = link->start_year;

        delete link;
        break;
    }
    
    if (start_year == -1) 
        return false;

    auto former_squad = df::allocate<df::histfig_entity_link_former_squadst>();
    former_squad->squad_id = squad->id;
    former_squad->entity_id = squad->entity_id;
    former_squad->start_year = start_year;
    former_squad->end_year = *df::global::cur_year;
    former_squad->link_strength = 100;

    hf->entity_links.push_back(former_squad);
    return true;
}

static bool remove_officer_entity_link(df::historical_figure* hf, df::squad* squad)
{
    std::vector<Units::NoblePosition> nps;
    if (! Units::getNoblePositions(&nps, hf))
        return false;

    int32_t assignment_id = -1;
    for (auto& np : nps)
    {
        if (np.entity->id != squad->entity_id || np.assignment->squad_id != squad->id) 
            continue;
        
        np.assignment->histfig = -1;
        np.assignment->histfig2 = -1;

        assignment_id = np.assignment->id;
        break;
    }
    
    if (assignment_id == -1)
        return false;

    int32_t start_year = -1;
    for (size_t i = 0; i < hf->entity_links.size(); i++)
    {
        auto link = strict_virtual_cast<df::histfig_entity_link_positionst>(hf->entity_links[i]);
        if (link == nullptr) continue;
        if (link->assignment_id != assignment_id && link->entity_id != squad->entity_id) continue;

        hf->entity_links.erase(hf->entity_links.begin() + i);
        start_year = link->start_year;
        
        delete link;
        break;
    }
    
    if (start_year == -1)
        return false;

    auto former_pos = df::allocate<df::histfig_entity_link_former_positionst>();
    former_pos->assignment_id = assignment_id;
    former_pos->entity_id = squad->entity_id;
    former_pos->start_year = start_year;
    former_pos->end_year = *df::global::cur_year;
    former_pos->link_strength = 100;

    hf->entity_links.push_back(former_pos);
    return true;
}

bool Military::removeFromSquad(int32_t unit_id)
{
    df::unit *unit = df::unit::find(unit_id);
    if (unit == nullptr)
        return false;
    if (unit->military.squad_id == -1 || unit->military.squad_position == -1)
        return false;

    int32_t squad_id = unit->military.squad_id;
    df::squad* squad = df::squad::find(squad_id);
    if (squad == nullptr)
        return false;

    // remove from squad information
    int32_t squad_pos = unit->military.squad_position;
    df::squad_position* pos = squad->positions.at(squad_pos);
    pos->occupant = -1;

    // remove from unit information
    unit->military.squad_id = -1;
    unit->military.squad_position = -1;

    df::historical_figure* hf = df::historical_figure::find(unit->hist_figure_id);
    if (hf == nullptr)
        return false;

    return squad_pos == 0 // is unit a commander?
        ? remove_officer_entity_link(hf, squad)
        : remove_soldier_entity_link(hf, squad);
}
