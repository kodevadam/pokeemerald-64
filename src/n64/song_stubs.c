/*
 * src/n64/song_stubs.c
 *
 * N64 port — stub definitions for all song/SE/phono symbols.
 * All music is no-op on N64; these are valid SongHeader structs
 * so the song table links correctly.
 */

#include "global.h"
#include "m4a.h"

/* Single shared dummy song part (FINE command = 0xB1) */
static u8 sStubSongPart[1] = { 0xB1 };

const struct SongHeader mus_abandoned_ship = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_abnormal_weather = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_aqua_magma_hideout = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_awaken_legend = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_b_arena = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_b_dome = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_b_dome_lobby = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_b_factory = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_b_frontier = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_b_palace = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_b_pike = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_b_pyramid = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_b_pyramid_top = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_b_tower = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_b_tower_rs = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_birch_lab = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_c_comm_center = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_c_vs_legend_beast = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_cable_car = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_caught = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_cave_of_origin = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_contest = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_contest_lobby = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_contest_results = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_contest_winner = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_credits = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_cycling = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_dewford = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_encounter_aqua = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_encounter_brendan = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_encounter_champion = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_encounter_cool = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_encounter_elite_four = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_encounter_female = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_encounter_girl = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_encounter_hiker = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_encounter_intense = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_encounter_interviewer = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_encounter_magma = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_encounter_male = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_encounter_may = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_encounter_rich = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_encounter_suspicious = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_encounter_swimmer = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_encounter_twins = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_end = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_ever_grande = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_evolution = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_evolution_intro = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_evolved = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_fallarbor = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_follow_me = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_fortree = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_game_corner = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_gsc_pewter = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_gsc_route38 = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_gym = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_hall_of_fame = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_hall_of_fame_room = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_heal = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_help = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_intro = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_intro_battle = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_level_up = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_lilycove = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_lilycove_museum = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_link_contest_p1 = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_link_contest_p2 = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_link_contest_p3 = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_link_contest_p4 = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_littleroot = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_littleroot_test = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_move_deleted = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_mt_chimney = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_mt_pyre = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_mt_pyre_exterior = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_obtain_b_points = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_obtain_badge = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_obtain_berry = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_obtain_item = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_obtain_symbol = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_obtain_tmhm = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_oceanic_museum = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_oldale = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_petalburg = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_petalburg_woods = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_poke_center = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_poke_mart = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rayquaza_appears = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_register_match_call = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_berry_pick = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_caught = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_caught_intro = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_celadon = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_cinnabar = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_credits = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_cycling = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_dex_rating = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_encounter_boy = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_encounter_deoxys = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_encounter_girl = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_encounter_gym_leader = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_encounter_rival = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_encounter_rocket = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_follow_me = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_fuchsia = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_game_corner = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_game_freak = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_gym = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_hall_of_fame = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_heal = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_intro_fight = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_jigglypuff = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_lavender = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_mt_moon = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_mystery_gift = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_net_center = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_new_game_exit = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_new_game_instruct = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_new_game_intro = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_oak = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_oak_lab = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_obtain_key_item = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_pallet = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_pewter = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_photo = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_poke_center = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_poke_flute = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_poke_jump = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_poke_mansion = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_poke_tower = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_rival_exit = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_rocket_hideout = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_route1 = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_route11 = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_route24 = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_route3 = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_sevii_123 = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_sevii_45 = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_sevii_67 = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_sevii_cave = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_sevii_dungeon = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_sevii_route = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_silph = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_slow_pallet = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_ss_anne = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_surf = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_teachy_tv_menu = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_teachy_tv_show = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_title = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_trainer_tower = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_union_room = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_vermillion = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_victory_gym_leader = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_victory_road = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_victory_trainer = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_victory_wild = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_viridian_forest = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_vs_champion = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_vs_deoxys = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_vs_gym_leader = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_vs_legend = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_vs_mewtwo = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_vs_trainer = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rg_vs_wild = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_roulette = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_route101 = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_route104 = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_route110 = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_route111 = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_route113 = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_route119 = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_route120 = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_route122 = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_rustboro = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_safari_zone = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_sailing = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_school = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_sealed_chamber = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_slateport = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_slots_jackpot = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_slots_win = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_sootopolis = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_surf = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_title = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_too_bad = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_trick_house = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_underwater = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_verdanturf = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_victory_aqua_magma = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_victory_gym_leader = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_victory_league = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_victory_road = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_victory_trainer = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_victory_wild = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_vs_aqua_magma = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_vs_aqua_magma_leader = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_vs_champion = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_vs_elite_four = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_vs_frontier_brain = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_vs_gym_leader = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_vs_kyogre_groudon = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_vs_mew = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_vs_rayquaza = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_vs_regi = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_vs_rival = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_vs_trainer = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_vs_wild = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader mus_weather_groudon = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader ph_choice_blend = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader ph_choice_held = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader ph_choice_solo = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader ph_cloth_blend = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader ph_cloth_held = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader ph_cloth_solo = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader ph_cure_blend = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader ph_cure_held = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader ph_cure_solo = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader ph_dress_blend = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader ph_dress_held = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader ph_dress_solo = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader ph_face_blend = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader ph_face_held = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader ph_face_solo = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader ph_fleece_blend = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader ph_fleece_held = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader ph_fleece_solo = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader ph_foot_blend = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader ph_foot_held = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader ph_foot_solo = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader ph_goat_blend = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader ph_goat_held = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader ph_goat_solo = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader ph_goose_blend = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader ph_goose_held = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader ph_goose_solo = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader ph_kit_blend = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader ph_kit_held = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader ph_kit_solo = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader ph_lot_blend = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader ph_lot_held = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader ph_lot_solo = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader ph_mouth_blend = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader ph_mouth_held = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader ph_mouth_solo = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader ph_nurse_blend = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader ph_nurse_held = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader ph_nurse_solo = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader ph_price_blend = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader ph_price_held = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader ph_price_solo = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader ph_strut_blend = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader ph_strut_held = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader ph_strut_solo = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader ph_thought_blend = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader ph_thought_held = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader ph_thought_solo = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader ph_trap_blend = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader ph_trap_held = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader ph_trap_solo = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_a = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_applause = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_arena_timeup1 = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_arena_timeup2 = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_ball = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_ball_bounce_1 = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_ball_bounce_2 = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_ball_bounce_3 = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_ball_bounce_4 = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_ball_open = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_ball_throw = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_ball_trade = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_ball_tray_ball = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_ball_tray_enter = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_ball_tray_exit = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_balloon_blue = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_balloon_red = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_balloon_yellow = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_bang = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_berry_blender = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_bike_bell = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_bike_hop = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_boo = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_breakable_door = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_bridge_walk = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_card = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_click = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_contest_condition_lose = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_contest_curtain_fall = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_contest_curtain_rise = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_contest_heart = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_contest_icon_change = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_contest_icon_clear = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_contest_mons_turn = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_contest_place = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_dex_page = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_dex_scroll = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_dex_search = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_ding_dong = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_door = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_downpour = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_downpour_stop = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_e = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_effective = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_egg_hatch = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_elevator = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_escalator = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_exit = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_exp = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_exp_max = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_failure = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_faint = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_fall = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_field_poison = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_flee = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_fu_zaku = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_glass_flute = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_i = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_ice_break = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_ice_crack = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_ice_stairs = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_intro_blast = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_itemfinder = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_lavaridge_fall_warp = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_ledge = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_low_health = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_absorb = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_absorb_2 = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_acid_armor = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_attract = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_attract2 = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_barrier = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_baton_pass = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_belly_drum = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_bind = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_bite = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_blizzard = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_blizzard2 = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_bonemerang = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_brick_break = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_bubble = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_bubble2 = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_bubble3 = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_bubble_beam = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_bubble_beam2 = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_charge = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_charm = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_comet_punch = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_confuse_ray = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_cosmic_power = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_crabhammer = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_cut = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_detect = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_dig = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_dive = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_dizzy_punch = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_double_slap = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_double_team = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_dragon_rage = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_earthquake = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_ember = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_encore = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_encore2 = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_explosion = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_faint_attack = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_fire_punch = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_flame_wheel = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_flame_wheel2 = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_flamethrower = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_flatter = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_fly = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_giga_drain = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_grasswhistle = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_gust = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_gust2 = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_hail = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_harden = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_haze = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_headbutt = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_heal_bell = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_heat_wave = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_horn_attack = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_hydro_pump = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_hyper_beam = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_hyper_beam2 = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_icy_wind = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_jump_kick = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_leer = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_lick = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_lock_on = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_mega_kick = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_mega_kick2 = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_metronome = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_milk_drink = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_minimize = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_mist = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_moonlight = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_morning_sun = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_nightmare = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_pay_day = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_perish_song = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_petal_dance = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_poison_powder = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_psybeam = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_psybeam2 = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_rain_dance = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_razor_wind = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_razor_wind2 = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_reflect = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_reversal = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_rock_throw = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_sacred_fire = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_sacred_fire2 = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_sand_attack = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_sand_tomb = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_sandstorm = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_scratch = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_screech = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_self_destruct = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_sing = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_sketch = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_sky_uppercut = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_snore = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_solar_beam = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_spit_up = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_stat_decrease = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_stat_increase = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_strength = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_string_shot = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_string_shot2 = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_supersonic = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_surf = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_swagger = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_swagger2 = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_sweet_scent = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_swift = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_swords_dance = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_tail_whip = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_take_down = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_teeter_dance = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_teleport = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_thunder_wave = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_thunderbolt = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_thunderbolt2 = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_toxic = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_tri_attack = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_tri_attack2 = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_twister = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_uproar = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_vicegrip = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_vital_throw = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_vital_throw2 = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_waterfall = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_whirlpool = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_wing_attack = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_m_yawn = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_mud_ball = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_mugshot = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_n = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_not_effective = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_note_a = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_note_b = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_note_c = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_note_c_high = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_note_d = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_note_e = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_note_f = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_note_g = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_o = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_orb = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_pc_login = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_pc_off = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_pc_on = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_pike_curtain_close = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_pike_curtain_open = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_pin = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_pokenav_call = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_pokenav_hang_up = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_pokenav_off = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_pokenav_on = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_puddle = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_rain = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_rain_stop = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_repel = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_rg_bag_cursor = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_rg_bag_pocket = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_rg_ball_click = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_rg_card_flip = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_rg_card_flipping = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_rg_card_open = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_rg_deoxys_move = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_rg_door = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_rg_help_close = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_rg_help_error = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_rg_help_open = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_rg_poke_jump_failure = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_rg_poke_jump_success = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_rg_shop = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_rg_ss_anne_horn = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_rotating_gate = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_roulette_ball = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_roulette_ball2 = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_save = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_select = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_shiny = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_ship = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_shop = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_sliding_door = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_success = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_sudowoodo_shake = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_super_effective = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_switch = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_taillow_wing_flap = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_thunder = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_thunder2 = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_thunderstorm = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_thunderstorm_stop = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_truck_door = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_truck_move = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_truck_stop = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_truck_unload = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_u = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_unlock = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_use_item = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_vend = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_wall_hit = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_warp_in = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_warp_out = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};

const struct SongHeader se_win_open = {
    .trackCount = 1, .blockCount = 0, .priority = 0, .reverb = 0,
    .tone = NULL, .part = { sStubSongPart }
};
