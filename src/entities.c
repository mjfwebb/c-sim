#include "headers.h"

char entity_names[][32] = {
    "pushqrdx",
    "Athano",
    "AshenHobs",
    "adrian_learns",
    "RVerite",
    "Orshy",
    "ruggs888",
    "Xent12",
    "nuke_bird",
    "kasper_573",
    "SturdyPose",
    "coffee_lava",
    "goudacheeseburgers",
    "ikiwixz",
    "NixAurvandil",
    "smilingbig",
    "tk_dev",
    "realSuperku",
    "Hoby2000",
    "CuteMath",
    "forodor",
    "Azenris",
    "collector_of_stuff",
    "EvanMMO",
    "thechaosbean",
    "Lutf1sk",
    "BauBas9883",
    "physbuzz",
    "rizoma0x00",
    "Tkap1",
    "GavinsAwfulStream",
    "Resist_0",
    "b1k4sh",
    "nhancodes",
    "qcircuit1",
    "fruloo",
    "programmer_jeff",
    "BluePinStudio",
    "Pierito95RsNg",
    "jumpylionnn",
    "Aruseus",
    "lastmiles",
    "soulfoam",
    "AQtun81",
    "jess_forrealz",
    "RAFi18",
    "Delvoid",
    "Lolboy_30",
    "VevenVelour",
    "Kisamius",
    "tobias_bms",
    "spectral_ray1",
    "Toasty",  // AKA CarbonCollins
    "Roilisi",
    "MickyMaven",
    "Katsuida",
    "YogiEisbar",
    "WaryOfDairy",
    "BauBas9883",
    "Kataemoi",
    "AgentulSRI",
    "Pushtoy",
    "Neron0010",
    "exodus_uk",
    "Coopert1n0",
    "mantra4aa",
    "Keikzz",
    "sreetunks",
    "noisycat3",
    "ca2software",
    "GyrosGeier",
    "GloriousSir",
    "kuviman",
    "nigelwithrow",
    "pgorley",
    "Kasie_SoftThorn",
    "tapir2342",
    "Protonmat",
    "davexmachina_",
    "seek1337",
    "godmode0071",
    "cakez77",
    "TravisVroman",
    "Deharma",
    "Rogue_Wolf_Dev",
    "Tuhkamjuhkam",
    "lolDayzo",
    "retromaximusplays",
    "nickely",
    "MaGetzUb",
    "capuche_man",
    "MrElmida",
    "Zanarias",
    "dasraizer",
    "Riazey",
    "Phil_Massicotte",
    "whaatsuuup",
    "BlaximusIV",
    "homerjay48",
    "Woozx",
    "Przemko9856",
    "whitent_",
    "dandymcgee"
};

void set_random_entity_direction(int entity_id, float velocity) {
  float angle = (float)(random_int_between(0, 360) * ATH_PI / 180);

  game_context.speed[entity_id] = (SpeedComponent){
      .current_direction.x = cosf(angle),
      .current_direction.y = sinf(angle),
      .current_velocity = velocity,
  };
}

void create_entity(float entity_width, int texture_id, int health_current, int health_max, char* name, Species species, Vec2 position) {
  game_context.texture[game_context.entity_count] = (TextureComponent){.texture_id = texture_id, .size = {.x = entity_width}};

  float scale = entity_width / render_context.texture_atlas.size[texture_id].x;
  game_context.texture[game_context.entity_count].size.y = (float)(render_context.texture_atlas.size[texture_id].y * scale);

  game_context.health_current[game_context.entity_count] = health_current;
  game_context.health_max[game_context.entity_count] = health_max;

  strcpy(game_context.name[game_context.entity_count], name);  // FIXME: Use the safe version strcpy_s. PRs welcome

  game_context.selected[game_context.entity_count] = false;
  game_context.hovered[game_context.entity_count] = false;

  game_context.realm[game_context.entity_count] = 0;
  game_context.experience[game_context.entity_count] = 0;
  game_context.species[game_context.entity_count] = species;

  game_context.decision_countdown[game_context.entity_count] = random_int_between(0, TICKS_TO_NEXT_DECISION);
  game_context.action_countdown[game_context.entity_count] = random_int_between(0, TICKS_TO_NEXT_ACTION);
  game_context.decision[game_context.entity_count] = Decisions__Wait;

  game_context.position[game_context.entity_count] = (Position){
      .current = position,
      .target = position,
      .spring_x =
          {
              .target = position.x,
              .current = position.x,
              .velocity = 0.0f,
              .acceleration = 0.5f,
              .friction = 0.1f,
          },
      .spring_y =
          {
              .target = position.y,
              .current = position.y,
              .velocity = 0.0f,
              .acceleration = 0.5f,
              .friction = 0.1f,
          },
  };
}

void create_tree(void) {
  Vec2 position = {.x = (float)random_int_between(-400, 400) * 100, .y = (float)random_int_between(-400, 400) * 100};
  create_entity(500.0f, random_int_between(GFX_TEXTURE_TREE_1, GFX_TEXTURE_TREE_6), 1000, 1000, "tree", Species__Tree, position);

  game_context.entity_count++;
}

void create_rock(void) {
  Vec2 position = {.x = (float)random_int_between(-400, 400) * 100, .y = (float)random_int_between(-400, 400) * 100};
  create_entity(100.0f, GFX_TEXTURE_ROCK, 1000, 1000, "rock", Species__Rock, position);

  game_context.entity_count++;
}

void create_human(char* name) {
  Vec2 position = {.x = (float)random_int_between(-1000, 1000), .y = (float)random_int_between(-1000, 1000)};
  create_entity(100.0f, random_int_between(0, 7), 100, 100, name, Species__Human, position);

  set_random_entity_direction(game_context.entity_count, BASE_VELOCITY);

  int random_amount_of_personalities = random_int_between(5, 10);
  for (int i = 0; i < random_amount_of_personalities; i++) {
    int personality = random_int_between(0, Personality_Count);
    game_context.personalities[game_context.entity_count][personality] = random_int_between(0, 100);
  }

  game_context.entity_count++;
}

void create_entities(void) {
  for (int name_index = 0; name_index < array_count(entity_names); name_index++) {
    create_human(entity_names[name_index]);
  }

  for (int i = 0; i < 9999; i++) {
    create_tree();
  }

  for (int i = 0; i < 9999; i++) {
    create_rock();
  }
}
