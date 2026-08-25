#ifndef YGOENV_EDOPro_EDOPro_H_
#define YGOENV_EDOPro_EDOPro_H_

// clang-format off
#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <numeric>
#include <stdexcept>
#include <string>
#include <cstring>
#include <fstream>
#include <stack>
#include <shared_mutex>
#include <iostream>

#include <fmt/core.h>
#include <fmt/ranges.h>
#include <SQLiteCpp/SQLiteCpp.h>
#include <SQLiteCpp/VariadicBind.h>
#include <ankerl/unordered_dense.h>

#include "ygoenv/core/async_envpool.h"
#include "ygoenv/core/env.h"

#include "edopro-core/common.h"
#include "edopro-core/card.h"
#include "edopro-core/ocgapi.h"

// clang-format on

namespace edopro {

inline std::vector<std::vector<int>> combinations(int n, int r) {
  std::vector<std::vector<int>> combs;
  std::vector<bool> m(n);
  std::fill(m.begin(), m.begin() + r, true);

  do {
    std::vector<int> cs;
    cs.reserve(r);
    for (int i = 0; i < n; ++i) {
      if (m[i]) {
        cs.push_back(i);
      }
    }
    combs.push_back(cs);
  } while (std::prev_permutation(m.begin(), m.end()));

  return combs;
}

inline bool sum_to(const std::vector<int> &w, const std::vector<int> ind, int i,
                   int r) {
  if (r <= 0) {
    return false;
  }
  int n = ind.size();
  if (i == n - 1) {
    return r == 1 || (w[ind[i]] == r);
  }
  return sum_to(w, ind, i + 1, r - 1) || sum_to(w, ind, i + 1, r - w[ind[i]]);
}

inline bool sum_to(const std::vector<int> &w, const std::vector<int> ind,
                   int r) {
  return sum_to(w, ind, 0, r);
}

inline std::vector<std::vector<int>>
combinations_with_weight(const std::vector<int> &weights, int r) {
  int n = weights.size();
  std::vector<std::vector<int>> results;

  for (int k = 1; k <= n; k++) {
    std::vector<std::vector<int>> combs = combinations(n, k);
    for (const auto &comb : combs) {
      if (sum_to(weights, comb, r)) {
        results.push_back(comb);
      }
    }
  }
  return results;
}

inline bool sum_to2(const std::vector<std::vector<int>> &w,
                    const std::vector<int> ind, int i, int r,
                    bool max = false) {
  if (r <= 0) {
    if (max) {
      return true;
    } else {
      return false;
    }
  }
  int n = ind.size();
  const auto &w_ = w[ind[i]];
  if (i == n - 1) {
    if (w_.size() == 1) {
      if (max) {
        return w_[0] >= r;
      } else {
        return w_[0] == r;
      }
    } else {
      if (max) {
        return w_[0] >= r || w_[1] >= r;
      } else {
        return w_[0] == r || w_[1] == r;
      }
    }
  }
  if (w_.size() == 1) {
    return sum_to2(w, ind, i + 1, r - w_[0], max);
  } else {
    return sum_to2(w, ind, i + 1, r - w_[0], max) ||
           sum_to2(w, ind, i + 1, r - w_[1], max);
  }
}

inline bool sum_to2(const std::vector<std::vector<int>> &w,
                    const std::vector<int> ind, int r, bool max = false) {
  return sum_to2(w, ind, 0, r, max);
}

inline std::vector<std::vector<int>>
combinations_with_weight2(const std::vector<std::vector<int>> &weights,
                          int r, bool max = false) {
  int n = weights.size();
  std::vector<std::vector<int>> results;

  for (int k = 1; k <= n; k++) {
    std::vector<std::vector<int>> combs = combinations(n, k);
    for (const auto &comb : combs) {
      if (sum_to2(weights, comb, r, max)) {
        results.push_back(comb);
      }
    }
  }
  return results;
}

static std::string msg_to_string(int msg) {
  switch (msg) {
  case MSG_RETRY:
    return "retry";
  case MSG_HINT:
    return "hint";
  case MSG_WIN:
    return "win";
  case MSG_SELECT_BATTLECMD:
    return "select_battlecmd";
  case MSG_SELECT_IDLECMD:
    return "select_idlecmd";
  case MSG_SELECT_EFFECTYN:
    return "select_effectyn";
  case MSG_SELECT_YESNO:
    return "select_yesno";
  case MSG_SELECT_OPTION:
    return "select_option";
  case MSG_SELECT_CARD:
    return "select_card";
  case MSG_SELECT_CHAIN:
    return "select_chain";
  case MSG_SELECT_PLACE:
    return "select_place";
  case MSG_SELECT_POSITION:
    return "select_position";
  case MSG_SELECT_TRIBUTE:
    return "select_tribute";
  case MSG_SELECT_COUNTER:
    return "select_counter";
  case MSG_SELECT_SUM:
    return "select_sum";
  case MSG_SELECT_DISFIELD:
    return "select_disfield";
  case MSG_SORT_CARD:
    return "sort_card";
  case MSG_SELECT_UNSELECT_CARD:
    return "select_unselect_card";
  case MSG_CONFIRM_DECKTOP:
    return "confirm_decktop";
  case MSG_CONFIRM_CARDS:
    return "confirm_cards";
  case MSG_SHUFFLE_DECK:
    return "shuffle_deck";
  case MSG_SHUFFLE_HAND:
    return "shuffle_hand";
  case MSG_SWAP_GRAVE_DECK:
    return "swap_grave_deck";
  case MSG_SHUFFLE_SET_CARD:
    return "shuffle_set_card";
  case MSG_REVERSE_DECK:
    return "reverse_deck";
  case MSG_DECK_TOP:
    return "deck_top";
  case MSG_SHUFFLE_EXTRA:
    return "shuffle_extra";
  case MSG_NEW_TURN:
    return "new_turn";
  case MSG_NEW_PHASE:
    return "new_phase";
  case MSG_CONFIRM_EXTRATOP:
    return "confirm_extratop";
  case MSG_MOVE:
    return "move";
  case MSG_POS_CHANGE:
    return "pos_change";
  case MSG_SET:
    return "set";
  case MSG_SWAP:
    return "swap";
  case MSG_FIELD_DISABLED:
    return "field_disabled";
  case MSG_SUMMONING:
    return "summoning";
  case MSG_SUMMONED:
    return "summoned";
  case MSG_SPSUMMONING:
    return "spsummoning";
  case MSG_SPSUMMONED:
    return "spsummoned";
  case MSG_FLIPSUMMONING:
    return "flipsummoning";
  case MSG_FLIPSUMMONED:
    return "flipsummoned";
  case MSG_CHAINING:
    return "chaining";
  case MSG_CHAINED:
    return "chained";
  case MSG_CHAIN_SOLVING:
    return "chain_solving";
  case MSG_CHAIN_SOLVED:
    return "chain_solved";
  case MSG_CHAIN_END:
    return "chain_end";
  case MSG_CHAIN_NEGATED:
    return "chain_negated";
  case MSG_CHAIN_DISABLED:
    return "chain_disabled";
  case MSG_RANDOM_SELECTED:
    return "random_selected";
  case MSG_BECOME_TARGET:
    return "become_target";
  case MSG_DRAW:
    return "draw";
  case MSG_DAMAGE:
    return "damage";
  case MSG_RECOVER:
    return "recover";
  case MSG_EQUIP:
    return "equip";
  case MSG_LPUPDATE:
    return "lpupdate";
  case MSG_CARD_TARGET:
    return "card_target";
  case MSG_CANCEL_TARGET:
    return "cancel_target";
  case MSG_PAY_LPCOST:
    return "pay_lpcost";
  case MSG_ADD_COUNTER:
    return "add_counter";
  case MSG_REMOVE_COUNTER:
    return "remove_counter";
  case MSG_ATTACK:
    return "attack";
  case MSG_BATTLE:
    return "battle";
  case MSG_ATTACK_DISABLED:
    return "attack_disabled";
  case MSG_DAMAGE_STEP_START:
    return "damage_step_start";
  case MSG_DAMAGE_STEP_END:
    return "damage_step_end";
  case MSG_MISSED_EFFECT:
    return "missed_effect";
  case MSG_TOSS_COIN:
    return "toss_coin";
  case MSG_TOSS_DICE:
    return "toss_dice";
  case MSG_ROCK_PAPER_SCISSORS:
    return "rock_paper_scissors";
  case MSG_HAND_RES:
    return "hand_res";
  case MSG_ANNOUNCE_RACE:
    return "announce_race";
  case MSG_ANNOUNCE_ATTRIB:
    return "announce_attrib";
  case MSG_ANNOUNCE_CARD:
    return "announce_card";
  case MSG_ANNOUNCE_NUMBER:
    return "announce_number";
  case MSG_CARD_HINT:
    return "card_hint";
  case MSG_TAG_SWAP:
    return "tag_swap";
  case MSG_RELOAD_FIELD:
    return "reload_field";
  case MSG_AI_NAME:
    return "ai_name";
  case MSG_SHOW_HINT:
    return "show_hint";
  case MSG_PLAYER_HINT:
    return "player_hint";
  case MSG_MATCH_KILL:
    return "match_kill";
  case MSG_CUSTOM_MSG:
    return "custom_msg";
  default:
    return "unknown_msg";
  }
}

// system string
static const ankerl::unordered_dense::map<int, std::string> system_strings = {
    {30, "Replay rules apply. Continue this attack?"},
    {31, "Attack directly with this monster?"},
    {96, "Use the effect of [%ls] to avoid destruction?"},
    {221, "On [%ls], Activate Trigger Effect of [%ls]?"},
    {1190, "Add to hand"},
    {1192, "Banish"},
    {1621, "Attack Negated"},
    {1622, "[%ls] Missed timing"}
};

static std::string get_system_string(uint32_t desc) {
  auto it = system_strings.find(desc);
  if (it != system_strings.end()) {
    return it->second;
  }
  return "system string " + std::to_string(desc);
}

static std::string ltrim(std::string s) {
  s.erase(s.begin(),
          std::find_if(s.begin(), s.end(),
                       std::not1(std::ptr_fun<int, int>(std::isspace))));
  return s;
}

inline std::vector<std::string> flag_to_usable_cardspecs(uint32_t flag,
                                                         bool reverse = false) {
  std::string zone_names[4] = {"m", "s", "om", "os"};
  std::vector<std::string> specs;
  for (int j = 0; j < 4; j++) {
    uint32_t value = (flag >> (j * 8)) & 0xff;
    for (int i = 0; i < 8; i++) {
      bool avail = (value & (1 << i)) == 0;
      if (reverse) {
        avail = !avail;
      }
      if (avail) {
        specs.push_back(zone_names[j] + std::to_string(i + 1));
      }
    }
  }
  return specs;
}

inline std::string ls_to_spec(uint8_t loc, uint8_t seq, uint8_t pos) {
  std::string spec;
  if (loc & LOCATION_HAND) {
    spec += "h";
  } else if (loc & LOCATION_MZONE) {
    spec += "m";
  } else if (loc & LOCATION_SZONE) {
    spec += "s";
  } else if (loc & LOCATION_GRAVE) {
    spec += "g";
  } else if (loc & LOCATION_REMOVED) {
    spec += "r";
  } else if (loc & LOCATION_EXTRA) {
    spec += "x";
  }
  spec += std::to_string(seq + 1);
  if (loc & LOCATION_OVERLAY) {
    spec.push_back('a' + pos);
  }
  return spec;
}

inline std::string ls_to_spec(uint8_t loc, uint8_t seq, uint8_t pos,
                              bool opponent) {
  std::string spec = ls_to_spec(loc, seq, pos);
  if (opponent) {
    spec.insert(0, 1, 'o');
  }
  return spec;
}

inline std::tuple<uint8_t, uint8_t, uint8_t>
spec_to_ls(const std::string spec) {
  uint8_t loc;
  uint8_t seq;
  uint8_t pos = 0;
  int offset = 1;
  if (spec[0] == 'h') {
    loc = LOCATION_HAND;
  } else if (spec[0] == 'm') {
    loc = LOCATION_MZONE;
  } else if (spec[0] == 's') {
    loc = LOCATION_SZONE;
  } else if (spec[0] == 'g') {
    loc = LOCATION_GRAVE;
  } else if (spec[0] == 'r') {
    loc = LOCATION_REMOVED;
  } else if (spec[0] == 'x') {
    loc = LOCATION_EXTRA;
  } else if (std::isdigit(spec[0])) {
    loc = LOCATION_DECK;
    offset = 0;
  } else {
    throw std::runtime_error("Invalid location");
  }
  int end = offset;
  while (end < spec.size() && std::isdigit(spec[end])) {
    end++;
  }
  seq = std::stoi(spec.substr(offset, end - offset)) - 1;
  if (end < spec.size()) {
    pos = spec[end] - 'a';
  }
  return {loc, seq, pos};
}

inline uint32_t ls_to_spec_code(uint8_t loc, uint8_t seq, uint8_t pos,
                                bool opponent) {
  uint32_t c = opponent ? 1 : 0;
  c |= (loc << 8);
  c |= (seq << 16);
  c |= (pos << 24);
  return c;
}

inline uint32_t spec_to_code(const std::string &spec) {
  int offset = 0;
  bool opponent = false;
  if (spec[0] == 'o') {
    opponent = true;
    offset++;
  }
  auto [loc, seq, pos] = spec_to_ls(spec.substr(offset));
  return ls_to_spec_code(loc, seq, pos, opponent);
}

inline std::string code_to_spec(uint32_t spec_code) {
  uint8_t loc = (spec_code >> 8) & 0xff;
  uint8_t seq = (spec_code >> 16) & 0xff;
  uint8_t pos = (spec_code >> 24) & 0xff;
  bool opponent = (spec_code & 0xff) == 1;
  return ls_to_spec(loc, seq, pos, opponent);
}

static std::tuple<std::vector<uint32_t>, std::vector<uint32_t>, std::vector<uint32_t>> read_decks(const std::string &fp) {
  std::ifstream file(fp);
  std::string line;
  std::vector<uint32_t> main_deck, extra_deck, side_deck;
  bool found_extra = false;

  if (file.is_open()) {
    // Read the main deck
    while (std::getline(file, line)) {
      if (line.find("side") != std::string::npos) {
        break;
      }
      if (line.find("extra") != std::string::npos) {
        found_extra = true;
        break;
      }
      // Check if line contains only digits
      if (!line.empty() && std::all_of(line.begin(), line.end(), ::isdigit)) {
        main_deck.push_back(std::stoul(line));
      }
    }

    // Read the extra deck
    if (found_extra) {
      while (std::getline(file, line)) {
        if (line.find("side") != std::string::npos) {
          break;
        }
        // Check if line contains only digits
        if (!line.empty() && std::all_of(line.begin(), line.end(), ::isdigit)) {
          extra_deck.push_back(std::stoul(line));
        }
      }
    }

    // Read the side deck
    while (std::getline(file, line)) {
      // Check if line contains only digits
      if (!line.empty() && std::all_of(line.begin(), line.end(), ::isdigit)) {
        side_deck.push_back(std::stoul(line));
      }
    }

    file.close();
  } else {
    throw std::runtime_error(fmt::format("Unable to open deck file: {}", fp));
  }

  return std::make_tuple(main_deck, extra_deck, side_deck);
}

template <class K = uint8_t>
ankerl::unordered_dense::map<K, uint8_t>
make_ids(const std::map<K, std::string> &m, int id_offset = 0,
         int m_offset = 0) {
  ankerl::unordered_dense::map<K, uint8_t> m2;
  int i = 0;
  for (const auto &[k, v] : m) {
    if (i < m_offset) {
      i++;
      continue;
    }
    m2[k] = i - m_offset + id_offset;
    i++;
  }
  return m2;
}

template <class K = char>
ankerl::unordered_dense::map<K, uint8_t>
make_ids(const std::vector<K> &cmds, int id_offset = 0, int m_offset = 0) {
  ankerl::unordered_dense::map<K, uint8_t> m2;
  for (int i = m_offset; i < cmds.size(); i++) {
    m2[cmds[i]] = i - m_offset + id_offset;
  }
  return m2;
}

static std::string reason_to_string(uint8_t reason) {
  // !victory 0x0 Surrendered
  // !victory 0x1 LP reached 0
  // !victory 0x2 Cards can't be drawn
  // !victory 0x3 Time limit up
  // !victory 0x4 Lost connection
  switch (reason) {
  case 0x0:
    return "Surrendered";
  case 0x1:
    return "LP reached 0";
  case 0x2:
    return "Cards can't be drawn";
  case 0x3:
    return "Time limit up";
  case 0x4:
    return "Lost connection";
  default:
    return "Unknown";
  }
}

static const std::map<uint8_t, std::string> location2str = {
    {LOCATION_DECK, "Deck"},
    {LOCATION_HAND, "Hand"},
    {LOCATION_MZONE, "Main Monster Zone"},
    {LOCATION_SZONE, "Spell & Trap Zone"},
    {LOCATION_GRAVE, "Graveyard"},
    {LOCATION_REMOVED, "Banished"},
    {LOCATION_EXTRA, "Extra Deck"},
};

static const ankerl::unordered_dense::map<uint8_t, uint8_t> location2id =
    make_ids(location2str, 1);

inline uint8_t location_to_id(uint8_t location) {
  auto it = location2id.find(location);
  if (it != location2id.end()) {
    return it->second;
  }
  return 0;
}

#define POS_NONE 0x0 // xyz materials (overlay)

static const std::map<uint8_t, std::string> position2str = {
    {POS_NONE, "none"},
    {POS_FACEUP_ATTACK, "face-up attack"},
    {POS_FACEDOWN_ATTACK, "face-down attack"},
    {POS_ATTACK, "attack"},
    {POS_FACEUP_DEFENSE, "face-up defense"},
    {POS_FACEUP, "face-up"},
    {POS_FACEDOWN_DEFENSE, "face-down defense"},
    {POS_FACEDOWN, "face-down"},
    {POS_DEFENSE, "defense"},
};

static const ankerl::unordered_dense::map<uint8_t, uint8_t> position2id =
    make_ids(position2str);

#define ATTRIBUTE_NONE 0x0 // token

static const std::map<uint8_t, std::string> attribute2str = {
    {ATTRIBUTE_NONE, "None"},   {ATTRIBUTE_EARTH, "Earth"},
    {ATTRIBUTE_WATER, "Water"}, {ATTRIBUTE_FIRE, "Fire"},
    {ATTRIBUTE_WIND, "Wind"},   {ATTRIBUTE_LIGHT, "Light"},
    {ATTRIBUTE_DARK, "Dark"},   {ATTRIBUTE_DIVINE, "Divine"},
};

static const ankerl::unordered_dense::map<uint8_t, uint8_t> attribute2id =
    make_ids(attribute2str);

#define RACE_NONE 0x0 // token

static const std::map<uint32_t, std::string> race2str = {
    {RACE_NONE, "None"},
    {RACE_WARRIOR, "Warrior"},
    {RACE_SPELLCASTER, "Spellcaster"},
    {RACE_FAIRY, "Fairy"},
    {RACE_FIEND, "Fiend"},
    {RACE_ZOMBIE, "Zombie"},
    {RACE_MACHINE, "Machine"},
    {RACE_AQUA, "Aqua"},
    {RACE_PYRO, "Pyro"},
    {RACE_ROCK, "Rock"},
    {RACE_WINGEDBEAST, "Windbeast"},
    {RACE_PLANT, "Plant"},
    {RACE_INSECT, "Insect"},
    {RACE_THUNDER, "Thunder"},
    {RACE_DRAGON, "Dragon"},
    {RACE_BEAST, "Beast"},
    {RACE_BEASTWARRIOR, "Beast Warrior"},
    {RACE_DINOSAUR, "Dinosaur"},
    {RACE_FISH, "Fish"},
    {RACE_SEASERPENT, "Sea Serpent"},
    {RACE_REPTILE, "Reptile"},
    {RACE_PSYCHIC, "Psycho"},
    {RACE_DIVINE, "Divine"},
    {RACE_CREATORGOD, "Creator God"},
    {RACE_WYRM, "Wyrm"},
    {RACE_CYBERSE, "Cyberse"},
    {RACE_ILLUSION, "Illusion'"}};

static const ankerl::unordered_dense::map<uint32_t, uint8_t> race2id =
    make_ids(race2str);

static const std::map<uint32_t, std::string> type2str = {
    {TYPE_MONSTER, "Monster"},
    {TYPE_SPELL, "Spell"},
    {TYPE_TRAP, "Trap"},
    {TYPE_NORMAL, "Normal"},
    {TYPE_EFFECT, "Effect"},
    {TYPE_FUSION, "Fusion"},
    {TYPE_RITUAL, "Ritual"},
    {TYPE_TRAPMONSTER, "Trap Monster"},
    {TYPE_SPIRIT, "Spirit"},
    {TYPE_UNION, "Union"},
    {TYPE_GEMINI, "Dual"},
    {TYPE_TUNER, "Tuner"},
    {TYPE_SYNCHRO, "Synchro"},
    {TYPE_TOKEN, "Token"},
    {TYPE_QUICKPLAY, "Quick-play"},
    {TYPE_CONTINUOUS, "Continuous"},
    {TYPE_EQUIP, "Equip"},
    {TYPE_FIELD, "Field"},
    {TYPE_COUNTER, "Counter"},
    {TYPE_FLIP, "Flip"},
    {TYPE_TOON, "Toon"},
    {TYPE_XYZ, "XYZ"},
    {TYPE_PENDULUM, "Pendulum"},
    {TYPE_SPSUMMON, "Special"},
    {TYPE_LINK, "Link"},
};

inline std::vector<uint8_t> type_to_ids(uint32_t type) {
  std::vector<uint8_t> ids;
  ids.reserve(type2str.size());
  for (const auto &[k, v] : type2str) {
    ids.push_back(std::min(1u, type & k));
  }
  return ids;
}

static const std::map<int, std::string> phase2str = {
    {PHASE_DRAW, "draw phase"},
    {PHASE_STANDBY, "standby phase"},
    {PHASE_MAIN1, "main1 phase"},
    {PHASE_BATTLE_START, "battle start phase"},
    {PHASE_BATTLE_STEP, "battle step phase"},
    {PHASE_DAMAGE, "damage phase"},
    {PHASE_DAMAGE_CAL, "damage calculation phase"},
    {PHASE_BATTLE, "battle phase"},
    {PHASE_MAIN2, "main2 phase"},
    {PHASE_END, "end phase"},
};

static const ankerl::unordered_dense::map<int, uint8_t> phase2id =
    make_ids(phase2str);

static const std::vector<int> _msgs = {
    MSG_SELECT_IDLECMD,  MSG_SELECT_CHAIN,     MSG_SELECT_CARD,
    MSG_SELECT_TRIBUTE,  MSG_SELECT_POSITION,  MSG_SELECT_EFFECTYN,
    MSG_SELECT_YESNO,    MSG_SELECT_BATTLECMD, MSG_SELECT_UNSELECT_CARD,
    MSG_SELECT_OPTION,   MSG_SELECT_PLACE,     MSG_SELECT_SUM,
    MSG_SELECT_DISFIELD, MSG_ANNOUNCE_ATTRIB,  MSG_ANNOUNCE_NUMBER,
    MSG_ANNOUNCE_CARD, MSG_ANNOUNCE_RACE,
};

static const ankerl::unordered_dense::map<int, uint8_t> msg2id =
    make_ids(_msgs, 1);

static const ankerl::unordered_dense::map<char, uint8_t> cmd_act2id =
    make_ids({'t', 'r', 'c', 's', 'm', 'a', 'v'}, 1);

static const ankerl::unordered_dense::map<char, uint8_t> cmd_phase2id =
    make_ids(std::vector<char>({'b', 'm', 'e'}), 1);

static const ankerl::unordered_dense::map<char, uint8_t> cmd_yesno2id =
    make_ids(std::vector<char>({'y', 'n'}), 1);

static const ankerl::unordered_dense::map<std::string, uint8_t> cmd_place2id =
    make_ids(std::vector<std::string>(
                 {"m1",  "m2",  "m3",  "m4",  "m5",  "m6",  "m7",  "s1",
                  "s2",  "s3",  "s4",  "s5",  "s6",  "s7",  "s8",  "om1",
                  "om2", "om3", "om4", "om5", "om6", "om7", "os1", "os2",
                  "os3", "os4", "os5", "os6", "os7", "os8"}),
             1);

using PlayerId = uint8_t;
using CardCode = uint32_t;
using CardId = uint16_t;

// --- effect-description decoding for the typed action layer (D4 Stage B) ---
// The modern core packs a card-effect description as (code << 20) | strindex;
// values below 1<<20 are system-string ids. LegalAction.effect_ stores system
// ids as-is and card-effect indices offset by kCardEffectOffset; the obs
// writer compresses both into one uint8 column.
constexpr uint32_t kSystemStringLimit = 1u << 20;
constexpr int kCardEffectOffset = 1 << 20;

inline std::tuple<CardCode, int> unpack_desc(uint64_t desc) {
  if (desc < kSystemStringLimit) {
    return {0, static_cast<int>(desc)};
  }
  CardCode code = static_cast<CardCode>(desc >> 20);
  int idx = static_cast<int>(desc & 0xfffff);
  return {code, idx + kCardEffectOffset};
}

// Stable ids (16+) for the curated system strings; unknown system strings map
// to 255 instead of throwing — the modern core has far more system strings
// than the curated map, and an obs id must never abort a duel.
inline const ankerl::unordered_dense::map<int, uint8_t> &system_string2id() {
  static const auto m = [] {
    ankerl::unordered_dense::map<int, uint8_t> m2;
    std::vector<int> keys;
    keys.reserve(system_strings.size());
    for (const auto &[k, v] : system_strings) {
      keys.push_back(k);
    }
    std::sort(keys.begin(), keys.end());
    uint8_t id = 16;
    for (int k : keys) {
      m2[k] = id++;
    }
    return m2;
  }();
  return m;
}

inline uint8_t system_string_to_id(int s) {
  const auto &m = system_string2id();
  auto it = m.find(s);
  return it != m.end() ? it->second : uint8_t(255);
}

inline std::string phase_to_string(int phase) {
  auto it = phase2str.find(phase);
  if (it != phase2str.end()) {
    return it->second;
  }
  return "unknown";
}

inline std::string position_to_string(int position) {
  auto it = position2str.find(position);
  if (it != position2str.end()) {
    return it->second;
  }
  return "unknown";
}

inline std::pair<uint8_t, uint8_t> float_transform(int x) {
  x = x % 65536;
  return {
      static_cast<uint8_t>(x >> 8),
      static_cast<uint8_t>(x & 0xff),
  };
}

static std::vector<int> find_substrs(const std::string &str,
                                     const std::string &substr) {
  std::vector<int> res;
  int pos = 0;
  while ((pos = str.find(substr, pos)) != std::string::npos) {
    res.push_back(pos);
    pos += substr.length();
  }
  return res;
}

inline std::string time_now() {
  // strftime %Y-%m-%d %H-%M-%S
  time_t now = time(0);
  tm *ltm = localtime(&now);
  char buffer[80];
  strftime(buffer, 80, "%Y-%m-%d %H-%M-%S", ltm);
  return std::string(buffer);
}

// from Multirole/YGOPro/Replay.cpp

enum ReplayTypes
{
	REPLAY_YRP1 = 0x31707279,
	REPLAY_YRPX = 0x58707279
};

enum ReplayFlags
{
	REPLAY_COMPRESSED      = 0x1,
	REPLAY_TAG             = 0x2,
	REPLAY_DECODED         = 0x4,
	REPLAY_SINGLE_MODE     = 0x8,
	REPLAY_LUA64           = 0x10,
	REPLAY_NEWREPLAY       = 0x20,
	REPLAY_HAND_TEST       = 0x40,
	REPLAY_DIRECT_SEED     = 0x80,
	REPLAY_64BIT_DUELFLAG  = 0x100,
	REPLAY_EXTENDED_HEADER = 0x200,
};

struct ReplayHeader
{
	uint32_t type; // See ReplayTypes.
	uint32_t version; // Unused atm, should be set to YGOPro::ClientVersion.
	uint32_t flags; // See ReplayFlags.
	uint32_t timestamp; // Unix timestamp.
	uint32_t size; // Uncompressed size of whatever is after this header.
	uint32_t hash; // Unused.
	uint8_t props[8U]; // Used for LZMA compression (check their apis).
	ReplayHeader()
		: type(0), version(0), flags(0), timestamp(0), size(0), hash(0), props{ 0 } {}
};

struct ExtendedReplayHeader
{
	static constexpr uint64_t CURRENT_VERSION = 1U;

	ReplayHeader base;
	uint64_t version; // Version of this extended header.
	uint64_t seed[4U]; // New 256bit seed.
};

// end from Multirole/YGOPro/Replay.cpp


struct loc_info {
	uint8_t controler;
	uint8_t location;
	uint32_t sequence;
	uint32_t position;
};

// --- typed action layer (D4 Stage B; mirrors ygopro.h so the upstream
// agents' 12-feature action encoding can be produced). During migration a
// LegalAction may instead carry the legacy option string in raw_; when every
// handler is ported, raw_ goes away.

enum class ActionAct {
  None,
  Set,
  Repo,
  SpSummon,
  Summon,
  MSet,
  Attack,
  DirectAttack,
  Activate,
  Cancel,
};

enum class ActionPhase {
  None,
  Battle,
  Main2,
  End,
};

enum class ActionPlace {
  None,
  MZone1, MZone2, MZone3, MZone4, MZone5, MZone6, MZone7,
  SZone1, SZone2, SZone3, SZone4, SZone5, SZone6, SZone7, SZone8,
  OpMZone1, OpMZone2, OpMZone3, OpMZone4, OpMZone5, OpMZone6, OpMZone7,
  OpSZone1, OpSZone2, OpSZone3, OpSZone4, OpSZone5, OpSZone6, OpSZone7,
  OpSZone8,
};

inline std::vector<ActionPlace> flag_to_usable_places(uint32_t flag,
                                                      bool reverse = false) {
  std::vector<ActionPlace> places;
  for (int j = 0; j < 4; j++) {
    uint32_t value = (flag >> (j * 8)) & 0xff;
    for (int i = 0; i < 8; i++) {
      bool avail = (value & (1 << i)) == 0;
      if (reverse) {
        avail = !avail;
      }
      if (avail) {
        ActionPlace place;
        if (j == 0) {
          place = static_cast<ActionPlace>(
              i + static_cast<int>(ActionPlace::MZone1));
        } else if (j == 1) {
          place = static_cast<ActionPlace>(
              i + static_cast<int>(ActionPlace::SZone1));
        } else if (j == 2) {
          place = static_cast<ActionPlace>(
              i + static_cast<int>(ActionPlace::OpMZone1));
        } else {
          place = static_cast<ActionPlace>(
              i + static_cast<int>(ActionPlace::OpSZone1));
        }
        places.push_back(place);
      }
    }
  }
  return places;
}

class LegalAction {
public:
  // Migration only: the legacy option string for handlers not yet emitting
  // typed fields. options_ is rendered from raw_ when non-empty.
  std::string raw_ = "";

  std::string spec_ = "";
  ActionAct act_ = ActionAct::None;
  ActionPhase phase_ = ActionPhase::None;
  bool finish_ = false;
  uint8_t position_ = 0;
  int effect_ = -1;
  uint8_t number_ = 0;
  ActionPlace place_ = ActionPlace::None;
  uint8_t attribute_ = 0;

  // No column exists for an announced race in the upstream 12-feature
  // layout (upstream never handled MSG_ANNOUNCE_RACE); the obs writer maps
  // race_ through the number column, keyed by the msg column.
  uint64_t race_ = 0;

  int spec_index_ = 0;
  CardId cid_ = 0;
  int msg_ = 0;
  uint32_t response_ = 0;

  static LegalAction legacy(const std::string &option) {
    LegalAction la;
    la.raw_ = option;
    return la;
  }

  static LegalAction from_spec(const std::string &spec) {
    LegalAction la;
    la.spec_ = spec;
    return la;
  }

  static LegalAction act_spec(ActionAct act, const std::string &spec) {
    LegalAction la;
    la.act_ = act;
    la.spec_ = spec;
    return la;
  }

  static LegalAction finish() {
    LegalAction la;
    la.finish_ = true;
    return la;
  }

  static LegalAction cancel() {
    LegalAction la;
    la.act_ = ActionAct::Cancel;
    return la;
  }

  static LegalAction activate_spec(int effect_idx, const std::string &spec) {
    LegalAction la;
    la.act_ = ActionAct::Activate;
    la.effect_ = effect_idx;
    la.spec_ = spec;
    return la;
  }

  static LegalAction phase(ActionPhase phase) {
    LegalAction la;
    la.phase_ = phase;
    return la;
  }

  static LegalAction number(uint8_t number) {
    LegalAction la;
    la.number_ = number;
    return la;
  }

  static LegalAction place(ActionPlace place) {
    LegalAction la;
    la.place_ = place;
    return la;
  }

  static LegalAction attribute(int attribute) {
    LegalAction la;
    la.attribute_ = static_cast<uint8_t>(attribute);
    return la;
  }
};

class SpecInfo {
public:
  uint16_t index;
  CardId cid;
};

using SpecInfos = ankerl::unordered_dense::map<std::string, SpecInfo>;

class Card {
  friend class EDOProEnv;

protected:
  CardCode code_ = 0;
  uint32_t alias_;
  // uint64_t setcode_;
  uint32_t type_;
  uint32_t level_;
  uint32_t lscale_;
  uint32_t rscale_;
  int32_t attack_;
  int32_t defense_;
  uint32_t race_;
  uint32_t attribute_;
  uint32_t link_marker_;
  // uint32_t category_;
  std::string name_;
  std::string desc_;
  std::vector<std::string> strings_;

  uint32_t data_ = 0;

  PlayerId controler_ = 0;
  uint32_t location_ = 0;
  uint32_t sequence_ = 0;
  uint32_t position_ = 0;
  uint32_t counter_ = 0;
  uint32_t status_ = 0;

public:
  Card() = default;

  Card(CardCode code, uint32_t alias, uint32_t type,
       uint32_t level, uint32_t lscale, uint32_t rscale, int32_t attack,
       int32_t defense, uint32_t race, uint32_t attribute, uint32_t link_marker,
       const std::string &name, const std::string &desc,
       const std::vector<std::string> &strings)
      : code_(code), alias_(alias), type_(type),
        level_(level), lscale_(lscale), rscale_(rscale), attack_(attack),
        defense_(defense), race_(race), attribute_(attribute),
        link_marker_(link_marker), name_(name), desc_(desc), strings_(strings) {
  }

  ~Card() = default;

  void set_location(uint32_t location) {
    controler_ = location & 0xff;
    location_ = (location >> 8) & 0xff;
    sequence_ = (location >> 16) & 0xff;
    position_ = (location >> 24) & 0xff;
  }

  void set_location(const loc_info &info) {
    controler_ = info.controler;
    location_ = info.location;
    sequence_ = info.sequence;
    position_ = info.position;
  }

  const CardCode &code() const { return code_; }
  const std::string &name() const { return name_; }
  const std::string &desc() const { return desc_; }
  const uint32_t &type() const { return type_; }
  const uint32_t &level() const { return level_; }
  const std::vector<std::string> &strings() const { return strings_; }

  std::string get_spec(bool opponent) const {
    return ls_to_spec(location_, sequence_, position_, opponent);
  }

  std::string get_spec(PlayerId player) const {
    return get_spec(player != controler_);
  }

  uint32_t get_spec_code(PlayerId player) const {
    return ls_to_spec_code(location_, sequence_, position_,
                           player != controler_);
  }

  std::string get_position() const { return position_to_string(position_); }

  std::string get_effect_description(uint64_t desc,
                                     bool existing = false) const {
    // Modern edopro-core packs card-string descs as (code << 20) | index;
    // values below 1<<20 are system strings.
    std::string s;
    bool e = false;
    uint32_t offset = static_cast<uint32_t>(desc & 0xfffff);
    bool is_card_desc = (desc >> 20) != 0;
    if (desc == 0 || (is_card_desc && offset < strings_.size())) {
      std::string str = is_card_desc ? ltrim(strings_[offset]) : "";
      if (str.empty()) {
        s = "Activate " + name_ + ".";
      } else {
        s = name_ + " (" + str + ")";
        e = true;
      }
    } else {
      s = get_system_string(static_cast<uint32_t>(desc));
      if (!s.empty()) {
        e = true;
      }
    }
    if (existing && !e) {
      s = "";
    }
    return s;
  }
};

inline std::string ls_to_spec(const loc_info &info, PlayerId player) {
  return ls_to_spec(info.location, info.sequence, info.position, player != info.controler);
}

inline uint32_t ls_to_spec_code(const loc_info &info, PlayerId player) {
  uint32_t c = player != info.controler ? 1 : 0;
  c |= (info.location << 8);
  c |= (info.sequence << 16);
  c |= (info.position << 24);
  return c;
}


// TODO: 7% performance loss
static std::shared_timed_mutex duel_mtx;

inline Card db_query_card(const SQLite::Database &db, CardCode code, bool may_absent = false) {
  SQLite::Statement query1(db, "SELECT * FROM datas WHERE id=?");
  query1.bind(1, code);
  bool found = query1.executeStep();
  if (!found) {
    if (may_absent) {
      return Card();
    }
    std::string msg = "[db_query_card] Card not found: " + std::to_string(code);
    throw std::runtime_error(msg);
  }

  uint32_t alias = query1.getColumn("alias");
  uint32_t type = query1.getColumn("type");
  uint32_t level_ = query1.getColumn("level");
  uint32_t level = level_ & 0xff;
  uint32_t lscale = (level_ >> 24) & 0xff;
  uint32_t rscale = (level_ >> 16) & 0xff;
  int32_t attack = query1.getColumn("atk");
  int32_t defense = query1.getColumn("def");
  uint32_t link_marker = 0;
  if (type & TYPE_LINK) {
    defense = 0;
    link_marker = defense;
  }
  uint32_t race = query1.getColumn("race");
  uint32_t attribute = query1.getColumn("attribute");

  SQLite::Statement query2(db, "SELECT * FROM texts WHERE id=?");
  query2.bind(1, code);
  query2.executeStep();

  std::string name = query2.getColumn(1);
  std::string desc = query2.getColumn(2);
  std::vector<std::string> strings;
  for (int i = 3; i < query2.getColumnCount(); ++i) {
    std::string str = query2.getColumn(i);
    strings.push_back(str);
  }
  return Card(code, alias, type, level, lscale, rscale, attack,
              defense, race, attribute, link_marker, name, desc, strings);
}

inline OCG_CardData db_query_card_data(
  const SQLite::Database &db, CardCode code, bool may_absent = false) {
  SQLite::Statement query(db, "SELECT * FROM datas WHERE id=?");
  query.bind(1, code);
  query.executeStep();
  OCG_CardData card;
  card.code = code;
  card.alias = query.getColumn("alias");
  uint64_t setcodes_ = query.getColumn("setcode").getInt64();

  std::vector<uint16_t> setcodes;
  for(int i = 0; i < 4; i++) {
    uint16_t setcode = (setcodes_ >> (i * 16)) & 0xffff;
    if (setcode) {
      setcodes.push_back(setcode);
    }
  }
  if (setcodes.size()) {
    setcodes.push_back(0);
    // memory leak here, but we only use it globally
    uint16_t* setcodes_p = new uint16_t[setcodes.size()];
    for (int i = 0; i < setcodes.size(); i++) {
      setcodes_p[i] = setcodes[i];
    }
    card.setcodes = setcodes_p;
  } else {
    card.setcodes = nullptr;
  }

  card.type = query.getColumn("type");
  card.attack = query.getColumn("atk");
  card.defense = query.getColumn("def");
  if (card.type & TYPE_LINK) {
    card.link_marker = card.defense;
    card.defense = 0;
  } else {
    card.link_marker = 0;
  }
  int level_ = query.getColumn("level");
  if (level_ < 0) {
    card.level = -(level_ & 0xff);
  }
  else {
    card.level = level_ & 0xff;
  }
  card.lscale = (level_ >> 24) & 0xff;
  card.rscale = (level_ >> 16) & 0xff;
  card.race = query.getColumn("race").getInt64();
  card.attribute = query.getColumn("attribute");
  return card;
}

// Identifies the binary that produced an incident: an action-index list only
// replays faithfully against the build that recorded it, because engine
// behaviour changes which options are offered.
#define YGOENV_BUILD_ID (__DATE__ " " __TIME__)

struct card_script {
  const char *buf;
  int len;
};

static ankerl::unordered_dense::map<CardCode, Card> cards_;
static ankerl::unordered_dense::map<CardCode, CardId> card_ids_;
static ankerl::unordered_dense::map<CardCode, OCG_CardData> cards_data_;
static ankerl::unordered_dense::map<std::string, card_script> cards_script_;
static ankerl::unordered_dense::map<std::string, std::vector<CardCode>>
    main_decks_;
static ankerl::unordered_dense::map<std::string, std::vector<CardCode>>
    extra_decks_;
static std::vector<std::string> deck_names_;

inline const Card &c_get_card(CardCode code) { return cards_.at(code); }

inline CardId &c_get_card_id(CardCode code) { return card_ids_.at(code); }

// Non-throwing variants for codes decoded out of effect descriptions: a desc
// can name a code outside the registered pool (tokens, alt arts), and an obs
// annotation must never abort a duel.
inline CardId c_get_card_id_or_zero(CardCode code) {
  auto it = card_ids_.find(code);
  return it != card_ids_.end() ? it->second : CardId(0);
}

inline const Card *c_find_card(CardCode code) {
  auto it = cards_.find(code);
  return it != cards_.end() ? &it->second : nullptr;
}

#ifndef CARD_MARINE_DOLPHIN
#define CARD_MARINE_DOLPHIN 78734254
#define CARD_TWINKLE_MOSS 13857930
#endif

// Mirrors is_declarable in ygopro-core/playerop.cpp (MSG_ANNOUNCE_CARD
// opcode filter). Keep in sync with the core.
// Mirrors ygopro-core's MSG_SELECT_SUM acceptance rules
// (field::process(Processors::SelectSum&) step 1).
// exact mode (mode==0): some o1/o2 assignment of the chosen cards sums to acc.
// overflow mode (mode==1): max-sum >= acc and (min-sum - smallest) < acc.
inline bool sum_exact_check(const std::vector<int> &o1,
                            const std::vector<int> &o2,
                            const std::vector<int> &cur, size_t k,
                            int64_t remaining) {
  if (k == cur.size()) {
    return remaining == 0;
  }
  int i = cur[k];
  if (remaining >= o1[i] &&
      sum_exact_check(o1, o2, cur, k + 1, remaining - o1[i])) {
    return true;
  }
  if (o2[i] != 0 && remaining >= o2[i] &&
      sum_exact_check(o1, o2, cur, k + 1, remaining - o2[i])) {
    return true;
  }
  return false;
}

inline void sum_combos_dfs(const std::vector<int> &o1,
                           const std::vector<int> &o2, int64_t acc, bool exact,
                           int min_cnt, int max_cnt, size_t idx,
                           std::vector<int> &cur,
                           std::vector<std::vector<int>> &out, size_t cap) {
  if (out.size() >= cap) {
    return;
  }
  if (!cur.empty() && (int)cur.size() >= min_cnt && (int)cur.size() <= max_cnt) {
    if (exact) {
      if (sum_exact_check(o1, o2, cur, 0, acc)) {
        out.push_back(cur);
      }
    } else {
      int64_t summin = 0, summax = 0, mn = INT64_MAX;
      for (int i : cur) {
        int a = o1[i], b = o2[i];
        int64_t ms = (b != 0 && b < a) ? b : a;
        int64_t mx = (b > a) ? b : a;
        summin += ms;
        summax += mx;
        mn = std::min(mn, ms);
      }
      if (summax >= acc && summin - mn < acc) {
        out.push_back(cur);
      }
    }
  }
  if (idx >= o1.size() || cur.size() >= 8) {
    return;
  }
  cur.push_back((int)idx);
  sum_combos_dfs(o1, o2, acc, exact, min_cnt, max_cnt, idx + 1, cur, out, cap);
  cur.pop_back();
  sum_combos_dfs(o1, o2, acc, exact, min_cnt, max_cnt, idx + 1, cur, out, cap);
}

inline bool is_declarable(const OCG_CardData &cd,
                          const std::vector<uint64_t> &opcodes) {
  std::stack<int64_t> stack;
  bool alias = false, token = false;
  auto binary = [&stack](auto fn) {
    if (stack.size() >= 2) {
      int64_t rhs = stack.top();
      stack.pop();
      int64_t lhs = stack.top();
      stack.pop();
      stack.push(fn(lhs, rhs));
    }
  };
  auto unary = [&stack](auto fn) {
    if (!stack.empty()) {
      int64_t v = stack.top();
      stack.pop();
      stack.push(fn(v));
    }
  };
  for (const auto &opcode : opcodes) {
    switch (opcode) {
    case OPCODE_ADD: binary([](int64_t a, int64_t b) { return a + b; }); break;
    case OPCODE_SUB: binary([](int64_t a, int64_t b) { return a - b; }); break;
    case OPCODE_MUL: binary([](int64_t a, int64_t b) { return a * b; }); break;
    case OPCODE_DIV: binary([](int64_t a, int64_t b) { return b ? a / b : 0; }); break;
    case OPCODE_AND: binary([](int64_t a, int64_t b) -> int64_t { return a && b; }); break;
    case OPCODE_OR: binary([](int64_t a, int64_t b) -> int64_t { return a || b; }); break;
    case OPCODE_NEG: unary([](int64_t v) { return -v; }); break;
    case OPCODE_NOT: unary([](int64_t v) -> int64_t { return !v; }); break;
    case OPCODE_BAND: binary([](int64_t a, int64_t b) { return a & b; }); break;
    case OPCODE_BOR: binary([](int64_t a, int64_t b) { return a | b; }); break;
    case OPCODE_BXOR: binary([](int64_t a, int64_t b) { return a ^ b; }); break;
    case OPCODE_BNOT: unary([](int64_t v) { return ~v; }); break;
    case OPCODE_LSHIFT: binary([](int64_t a, int64_t b) { return a << b; }); break;
    case OPCODE_RSHIFT: binary([](int64_t a, int64_t b) { return a >> b; }); break;
    case OPCODE_ISCODE:
      unary([&cd](int64_t v) -> int64_t { return cd.code == (uint32_t)v; });
      break;
    case OPCODE_ISTYPE:
      unary([&cd](int64_t v) -> int64_t { return cd.type & v; });
      break;
    case OPCODE_ISRACE:
      unary([&cd](int64_t v) -> int64_t { return (cd.race & v) != 0; });
      break;
    case OPCODE_ISATTRIBUTE:
      unary([&cd](int64_t v) -> int64_t { return cd.attribute & v; });
      break;
    case OPCODE_GETCODE: stack.push(cd.code); break;
    case OPCODE_GETTYPE: stack.push(cd.type); break;
    case OPCODE_GETRACE: stack.push(static_cast<int64_t>(cd.race)); break;
    case OPCODE_GETATTRIBUTE: stack.push(cd.attribute); break;
    case OPCODE_ISSETCARD: {
      if (!stack.empty()) {
        int32_t set_code = (int32_t)stack.top();
        stack.pop();
        bool res = false;
        uint16_t settype = set_code & 0xfff;
        uint16_t setsubtype = set_code & 0xf000;
        if (cd.setcodes != nullptr) {
          for (const uint16_t *sc = cd.setcodes; *sc != 0; ++sc) {
            if ((*sc & 0xfff) == settype &&
                (*sc & 0xf000 & setsubtype) == setsubtype) {
              res = true;
              break;
            }
          }
        }
        stack.push(res);
      }
      break;
    }
    case OPCODE_ALLOW_ALIASES: alias = true; break;
    case OPCODE_ALLOW_TOKENS: token = true; break;
    default: stack.push(static_cast<int64_t>(opcode)); break;
    }
  }
  if (stack.size() != 1 || stack.top() == 0) {
    return false;
  }
  return cd.code == CARD_MARINE_DOLPHIN || cd.code == CARD_TWINKLE_MOSS ||
         ((alias || !cd.alias) &&
          (token || ((cd.type & (TYPE_MONSTER + TYPE_TOKEN)) !=
                     (TYPE_MONSTER + TYPE_TOKEN))));
}

inline void sort_extra_deck(std::vector<CardCode> &deck) {
  std::vector<CardCode> c;
  std::vector<std::pair<CardCode, int>> fusion, xyz, synchro, link;

  for (auto code : deck) {
    const Card &cc = c_get_card(code);
    if (cc.type() & TYPE_FUSION) {
      fusion.push_back({code, cc.level()});
    } else if (cc.type() & TYPE_XYZ) {
      xyz.push_back({code, cc.level()});
    } else if (cc.type() & TYPE_SYNCHRO) {
      synchro.push_back({code, cc.level()});
    } else if (cc.type() & TYPE_LINK) {
      link.push_back({code, cc.level()});
    } else {
      throw std::runtime_error("Not extra deck card");
    }
  }

  auto cmp = [](const std::pair<CardCode, int> &a,
                const std::pair<CardCode, int> &b) {
    return a.second < b.second;
  };
  std::sort(fusion.begin(), fusion.end(), cmp);
  std::sort(xyz.begin(), xyz.end(), cmp);
  std::sort(synchro.begin(), synchro.end(), cmp);
  std::sort(link.begin(), link.end(), cmp);

  for (const auto &tc : fusion) {
    c.push_back(tc.first);
  }
  for (const auto &tc : xyz) {
    c.push_back(tc.first);
  }
  for (const auto &tc : synchro) {
    c.push_back(tc.first);
  }
  for (const auto &tc : link) {
    c.push_back(tc.first);
  }

  deck = c;
}

inline void preload_deck(const SQLite::Database &db,
                         const std::vector<CardCode> &deck,
                         bool may_absent = false) {
  for (const auto &code : deck) {
    auto it = cards_.find(code);
    if (it == cards_.end()) {
      auto card = db_query_card(db, code, may_absent);
      if ((card.code() == 0) && may_absent) {
        fmt::println("[preload_deck] Card not found: {}", code);
        continue;
      }
      cards_[code] = card;
      if (card_ids_.find(code) == card_ids_.end()) {
        throw std::runtime_error("Card not found in code list: " +
                                 std::to_string(code));
      }
    }

    auto it2 = cards_data_.find(code);
    if (it2 == cards_data_.end()) {
      cards_data_[code] = db_query_card_data(db, code, may_absent);
    }
  }
}

inline void g_DataReader(void* payload, uint32_t code, OCG_CardData* data) {
  auto it = cards_data_.find(code);
  if (it == cards_data_.end()) {
    throw std::runtime_error("[g_DataReader] Card not found: " + std::to_string(code));
  }
  *data = it->second;
}

static std::shared_timed_mutex scripts_mtx;

inline const char *read_card_script(const std::string &path, int *lenptr) {
  // edopro_script/c*.lua copied from ProjectIgnis/script/official
  auto full_path = "edopro_script/" + path;
  std::ifstream file(full_path, std::ios::binary);
  if (!file) {
    if (std::getenv("YGOENV_CORE_LOG")) {
      fmt::print(stderr, "Unable to open script file: {}\n", full_path);
    }
    *lenptr = 0;
    return nullptr;
  }
  file.seekg(0, std::ios::end);
  int len = file.tellg();
  file.seekg(0, std::ios::beg);
  const char *buf = new char[len];
  file.read((char *)buf, len);
  *lenptr = len;
  return buf;
}

inline int g_ScriptReader(void* payload, OCG_Duel duel, const char* name) {
  // Called concurrently by every env thread (scripts load lazily on first
  // use). Copy the cache entry out under a lock and never hold an iterator
  // across an unlocked region: unordered_dense invalidates iterators on
  // insert, so the previous "unlock, load, re-find, read outside the lock"
  // shape raced with concurrent inserts (observed as release-build segfaults
  // seconds into a run, when script loading is heaviest).
  std::string path(name);
  card_script entry{nullptr, 0};
  bool found = false;
  {
    std::shared_lock<std::shared_timed_mutex> lock(scripts_mtx);
    auto it = cards_script_.find(path);
    if (it != cards_script_.end()) {
      entry = it->second;
      found = true;
    }
  }
  if (!found) {
    int len = 0;
    const char *buf = read_card_script(path, &len);
    std::unique_lock<std::shared_timed_mutex> ulock(scripts_mtx);
    auto [it, inserted] = cards_script_.try_emplace(path, card_script{buf, len});
    if (!inserted && buf != nullptr) {
      // another thread loaded the same script first; drop our copy so the
      // cached pointer stays immutable for the process lifetime
      delete[] buf;
    }
    entry = it->second;
  }
  // Safe unlocked: cache entries are immutable once inserted.
  auto res = entry.len && OCG_LoadScript(duel, entry.buf,
                                         static_cast<uint32_t>(entry.len), name);
  // if (!res) {
  //   fmt::print("Failed to load script: {}\n", path);
  // }
  return res;
}

void g_LogHandler(void* payload, const char* string, int type) {
  if (std::getenv("YGOENV_CORE_LOG")) {
    fmt::println(stderr, "[LOG] type: {}, string: {}", type, string);
  }
}

static void init_module(const std::string &db_path,
                        const std::string &code_list_file,
                        const std::map<std::string, std::string> &decks) {
  // parse code from code_list_file
  std::ifstream file(code_list_file);
  std::string line;
  int i = 0;
  while (std::getline(file, line)) {
    i++;
    CardCode code = std::stoul(line);
    card_ids_[code] = i;
  }

  SQLite::Database db(db_path, SQLite::OPEN_READONLY);

  // Preload every card in the code list (not just deck cards): effects can
  // put tokens and non-deck cards into play, and a cards_ miss is fatal.
  {
    std::vector<CardCode> all_codes;
    all_codes.reserve(card_ids_.size());
    for (const auto &[code, idx] : card_ids_) {
      all_codes.push_back(code);
    }
    preload_deck(db, all_codes, true);
  }

  // code 0 appears in messages for hidden/face-down cards (e.g. face-down
  // special summons write code 0): give c_get_card a uniform placeholder.
  if (cards_.find(0) == cards_.end()) {
    cards_[0] = Card(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "[hidden]", "", {});
    card_ids_[0] = 0;
  }

  for (const auto &[name, deck] : decks) {
    auto [main_deck, extra_deck, side_deck] = read_decks(deck);
    main_decks_[name] = main_deck;
    extra_decks_[name] = extra_deck;
    if (name[0] != '_') {
      deck_names_.push_back(name);
    }

    preload_deck(db, main_deck);
    preload_deck(db, extra_deck);
    preload_deck(db, side_deck, true);
  }

  for (auto &[name, deck] : extra_decks_) {
    sort_extra_deck(deck);
  }

}

// from edopro/gframe/RNG/SplitMix64.hpp
class SplitMix64
{
public:
	using ResultType = uint64_t;
	using StateType = uint64_t;

	constexpr SplitMix64(StateType initialState) noexcept : s(initialState)
	{}

	ResultType operator()() noexcept
	{
		uint64_t z = (s += 0x9e3779b97f4a7c15);
		z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
		z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
		return z ^ (z >> 31);
	}

	// NOTE: std::shuffle requires these.
	using result_type = ResultType;
	static constexpr ResultType min() noexcept { return ResultType(0U); }
	static constexpr ResultType max() noexcept { return ResultType(~ResultType(0U)); }
private:
	StateType s;
};


inline std::string getline() {
  char *line = nullptr;
  size_t len = 0;
  ssize_t read;

  read = getline(&line, &len, stdin);

  if (read != -1) {
    // Remove line ending character(s)
    if (line[read - 1] == '\n')
      line[read - 1] = '\0'; // Replace newline character with null terminator
    else if (line[read - 2] == '\r' && line[read - 1] == '\n') {
      line[read - 2] = '\0'; // Replace carriage return and newline characters
                             // with null terminator
      line[read - 1] = '\0';
    }

    std::string input(line);
    free(line);
    return input;
  } else {
    exit(0);
  }

  free(line);
  return "";
}

class Player {
  friend class EDOProEnv;

protected:
  const std::string nickname_;
  const int init_lp_;
  const PlayerId duel_player_;
  const bool verbose_;

  bool seen_waiting_ = false;

public:
  Player(const std::string &nickname, int init_lp, PlayerId duel_player,
         bool verbose = false)
      : nickname_(nickname), init_lp_(init_lp), duel_player_(duel_player),
        verbose_(verbose) {}
  virtual ~Player() = default;

  void notify(const std::string &text) {
    if (verbose_) {
      fmt::println("{} {}", duel_player_, text);
    }
  }

  const int &init_lp() const { return init_lp_; }

  virtual int think(const std::vector<std::string> &options) = 0;
};

class GreedyAI : public Player {
protected:
public:
  GreedyAI(const std::string &nickname, int init_lp, PlayerId duel_player,
           bool verbose = false)
      : Player(nickname, init_lp, duel_player, verbose) {}

  int think(const std::vector<std::string> &options) override { return 0; }
};

class RandomAI : public Player {
protected:
  std::mt19937 gen_;
  std::uniform_int_distribution<int> dist_;

public:
  RandomAI(int max_options, int seed, const std::string &nickname, int init_lp,
           PlayerId duel_player, bool verbose = false)
      : Player(nickname, init_lp, duel_player, verbose), gen_(seed),
        dist_(0, max_options - 1) {}

  int think(const std::vector<std::string> &options) override {
    return dist_(gen_) % options.size();
  }
};

class HumanPlayer : public Player {
protected:
public:
  HumanPlayer(const std::string &nickname, int init_lp, PlayerId duel_player,
              bool verbose = false)
      : Player(nickname, init_lp, duel_player, verbose) {}

  int think(const std::vector<std::string> &options) override {
    while (true) {
      std::string input = getline();
      if (input == "quit") {
        exit(0);
      }
      auto it = std::find(options.begin(), options.end(), input);
      if (it != options.end()) {
        return std::distance(options.begin(), it);
      } else {
        fmt::println("{} Choose from {}", duel_player_, options);
      }
    }
  }
};

class EDOProEnvFns {
public:
  static decltype(auto) DefaultConfig() {
    return MakeDict("deck1"_.Bind(std::string("OldSchool")),
                    "deck2"_.Bind(std::string("OldSchool")), "player"_.Bind(-1),
                    "play_mode"_.Bind(std::string("bot")),
                    "verbose"_.Bind(false), "max_options"_.Bind(16),
                    "max_cards"_.Bind(80), "n_history_actions"_.Bind(16),
                    "record"_.Bind(false),
                    // Forcing the duel seed makes an incident exactly
                    // reproducible: (deck1, deck2, duel_seed, actions) is a
                    // complete case file for judge-style investigation.
                    // uint32: duel seeds span the full 32-bit range, and an int32
                    // binding silently rejects any seed above 2^31.
                    "duel_seed"_.Bind(uint32_t(0)),
                    // Exact post-shuffle deck order. The deck is shuffled with
                    // the env's own RNG, so duel_seed alone does NOT reproduce
                    // a duel; an incident must carry the drawn order too.
                    "deck_order_file"_.Bind(std::string("")),
                    // EDOPro single-mode puzzle: a Lua script that builds the
                    // board with Debug.AddCard/SetPlayerInfo/ReloadFieldEnd.
                    // When set, it replaces deck loading, giving small
                    // targeted scenarios with known-correct answers.
                    "puzzle"_.Bind(std::string("")),
                    // ygopro-layout config keys (D4 obs port; see
                    // notes/06-obs-port-plan.md). max_steps: episode decision
                    // cap; 0 = fall back to the envpool-common
                    // max_episode_steps so existing tools keep their
                    // behaviour (the upstream trainer always passes it).
                    "async_reset"_.Bind(false), "greedy_reward"_.Bind(true),
                    "timeout"_.Bind(600), "oppo_info"_.Bind(false),
                    "max_steps"_.Bind(0));
  }
  template <typename Config>
  static decltype(auto) StateSpec(const Config &conf) {
    // ygopro layout (D4): 12 typed action features; history rows add
    // turn-diff and phase columns.
    int n_action_feats = 12;
    return MakeDict(
        "obs:cards_"_.Bind(Spec<uint8_t>({conf["max_cards"_] * 2, 41})),
        "obs:global_"_.Bind(Spec<uint8_t>({23})),
        "obs:actions_"_.Bind(
            Spec<uint8_t>({conf["max_options"_], n_action_feats})),
        "obs:h_actions_"_.Bind(
            Spec<uint8_t>({conf["n_history_actions"_], n_action_feats + 2})),
        "obs:mask_"_.Bind(Spec<uint8_t>({conf["max_cards"_] * 2, 14})),
        "info:num_options"_.Bind(Spec<int>({}, std::tuple<int, int>{0, conf["max_options"_] - 1})),
        "info:to_play"_.Bind(Spec<int>({}, std::tuple<int, int>{0, 1})),
        "info:is_selfplay"_.Bind(Spec<int>({}, std::tuple<int, int>{0, 1})),
        "info:win_reason"_.Bind(Spec<int>({}, std::tuple<int, int>{-1, 1})),
        "info:step_time"_.Bind(Spec<double>({2})),
        "info:deck"_.Bind(Spec<int>({2})));
  }
  template <typename Config>
  static decltype(auto) ActionSpec(const Config &conf) {
    return MakeDict(
        "action"_.Bind(Spec<int>({}, std::tuple<int, int>{0, conf["max_options"_] - 1})));
  }
};

using EDOProEnvSpec = EnvSpec<EDOProEnvFns>;

enum PlayMode { kHuman, kSelfPlay, kRandomBot, kGreedyBot, kCount };

// parse play modes seperated by '+'
inline std::vector<PlayMode> parse_play_modes(const std::string &play_mode) {
  std::vector<PlayMode> modes;
  std::istringstream ss(play_mode);
  std::string token;
  while (std::getline(ss, token, '+')) {
    if (token == "human") {
      modes.push_back(kHuman);
    } else if (token == "self") {
      modes.push_back(kSelfPlay);
    } else if (token == "bot") {
      modes.push_back(kGreedyBot);
    } else if (token == "random") {
      modes.push_back(kRandomBot);
    } else {
      throw std::runtime_error("Unknown play mode: " + token);
    }
  }
  // human mode can't be combined with other modes
  if (std::find(modes.begin(), modes.end(), kHuman) != modes.end() &&
      modes.size() > 1) {
    throw std::runtime_error("Human mode can't be combined with other modes");
  }
  return modes;
}


// from edopro-deskbot/src/client.cpp#L46
constexpr uint64_t duel_options_ = DUEL_MODE_MR5;

class EDOProEnv : public Env<EDOProEnvSpec> {
protected:
  std::string puzzle_path_;
  std::string deck_order_file_;
  std::string deck1_;
  std::string deck2_;
  std::vector<uint32_t> main_deck0_;
  std::vector<uint32_t> main_deck1_;
  std::vector<uint32_t> extra_deck0_;
  std::vector<uint32_t> extra_deck1_;

  std::string deck_name_[2] = {"", ""};
  std::string nickname_[2] = {"Alice", "Bob"};

  const std::vector<PlayMode> play_modes_;

  // if play_mode_ == 'bot' or 'human', player_ is the order of the ai player
  // -1 means random, 0 and 1 means the first and second player respectively
  const int player_;

  PlayMode play_mode_;
  bool verbose_ = false;
  bool compat_mode_ = false;

  int max_episode_steps_, elapsed_step_;

  PlayerId ai_player_;

  OCG_Duel pduel_;
  // Value-initialized: Reset() and ~EDOProEnv() both delete non-null entries,
  // so indeterminate values here are a delete of garbage (crashes whenever the
  // object lands on recycled heap rather than fresh zeroed pages).
  // Incident case file: replaying (deck1, deck2, duel_seed, action_history)
  // reproduces a duel exactly, so loops can be investigated as a judge would
  // rather than guessed at from thresholds.
  uint32_t duel_seed_ = 0;
  std::vector<int> action_history_;

  // Involuntary-loop detection (see MSG_CHAINING handler)
  uint64_t chains_since_decision_ = 0;
  CardCode loop_cause_code_ = 0;
  static constexpr uint64_t kInvoluntaryLoopChains = 2000;

  Player *players_[2]{}; //  abstract class must be pointer

  std::uniform_int_distribution<uint64_t> dist_int_;
  bool done_{true};
  bool duel_started_{false};
  int duel_status_{OCG_DUEL_STATUS_CONTINUE};

  PlayerId winner_;
  uint8_t win_reason_;

  int lp_[2];

  // turn player
  PlayerId tp_;
  int current_phase_;
  int turn_count_;

  int msg_;
  std::vector<std::string> options_;
  // Typed twin of options_ (D4 Stage B). Ported handlers call push_action(),
  // which keeps both in lockstep; when every handler is ported, options_
  // becomes a pure render of this vector.
  std::vector<LegalAction> legal_actions_;

  // Iterative multi-select state (D4 Stage B): SELECT_CARD/TRIBUTE/SUM pick
  // ONE card per env step instead of enumerating combinations (which explode
  // and forced random fallbacks that silently hid actions). ms_idx_ != -1
  // while a selection sequence is in progress; next() then re-presents
  // without consuming a new core message.
  // mode 0: free choice among remaining specs, finish allowed at >= min.
  // mode 1: constrained; ms_combs_ holds the valid (sorted) index combos and
  //         each pick prefix-filters them, auto-finishing on a complete one.
  int ms_idx_ = -1;
  int ms_mode_ = 0;
  int ms_min_ = 0;
  int ms_max_ = 0;
  std::vector<std::string> ms_specs_;
  std::vector<std::vector<int>> ms_combs_;
  std::vector<int> ms_r_idxs_;
  ankerl::unordered_dense::map<std::string, int> ms_spec2idx_;
  std::function<void(const std::vector<int> &)> ms_send_;
  PlayerId to_play_;
  std::function<void(int)> callback_;

  uint8_t data_[4096];
  int dp_ = 0;
  int dl_ = 0;
  int fdl_ = 0;

  uint8_t query_buf_[16384];
  int qdp_ = 0;

  uint8_t resp_buf_[128];

  // data is the effect description: u64 in the modern core
  // ((code << 20) | strindex overflows u32 for almost every card code).
  using IdleCardSpec = std::tuple<CardCode, std::string, uint64_t>;

  // chain
  PlayerId chaining_player_;

  double step_time_ = 0;
  uint64_t step_time_count_ = 0;

  double reset_time_ = 0;
  uint64_t reset_time_count_ = 0;

  const int n_history_actions_;

  // circular buffer for history actions of player 0
  TArray<uint8_t> history_actions_0_;
  int ha_p_0_ = 0;

  // circular buffer for history actions of player 1
  TArray<uint8_t> history_actions_1_;
  int ha_p_1_ = 0;

  std::vector<std::string> revealed_;

  // discard hand cards
  bool discard_hand_ = false;

  // replay
  bool record_ = false;
  FILE* fp_ = nullptr;
  bool is_recording = false;

  // reward shaping (ygopro-layout config key; flat ±1 when false)
  bool greedy_reward_ = true;

public:
  EDOProEnv(const Spec &spec, int env_id)
      : Env<EDOProEnvSpec>(spec, env_id),
        // ygopro-layout key max_steps (trainer-passed) wins when set; 0 falls
        // back to the envpool-common max_episode_steps our tools use.
        max_episode_steps_(int(spec.config["max_steps"_]) > 0
                               ? int(spec.config["max_steps"_])
                               : int(spec.config["max_episode_steps"_])),
        elapsed_step_(max_episode_steps_ + 1), dist_int_(0, 0xffffffff),
        deck1_(spec.config["deck1"_]), deck2_(spec.config["deck2"_]),
        puzzle_path_(spec.config["puzzle"_]),
        deck_order_file_(spec.config["deck_order_file"_]),
        player_(spec.config["player"_]),
        play_modes_(parse_play_modes(spec.config["play_mode"_])),
        verbose_(spec.config["verbose"_]), record_(spec.config["record"_]),
        n_history_actions_(spec.config["n_history_actions"_]) {
    greedy_reward_ = spec.config["greedy_reward"_];
    if (record_) {
      if (!verbose_) {
        throw std::runtime_error("record mode must be used with verbose mode and num_envs=1");
      }
    }

    int max_options = spec.config["max_options"_];
    int n_h_action_feats = spec.state_spec["obs:h_actions_"_].shape[1];
    history_actions_0_ = TArray<uint8_t>(Array(
        ShapeSpec(sizeof(uint8_t), {n_history_actions_, n_h_action_feats})));
    history_actions_1_ = TArray<uint8_t>(Array(
        ShapeSpec(sizeof(uint8_t), {n_history_actions_, n_h_action_feats})));
  }

  ~EDOProEnv() {
    for (int i = 0; i < 2; i++) {
      if (players_[i] != nullptr) {
        delete players_[i];
      }
    }
  }

  int max_options() const { return spec_.config["max_options"_]; }

  int max_cards() const { return spec_.config["max_cards"_]; }

  bool IsDone() override { return done_; }

  bool random_mode() const { return play_modes_.size() > 1; }

  bool self_play() const {
    return std::find(play_modes_.begin(), play_modes_.end(), kSelfPlay) !=
           play_modes_.end();
  }

  void Reset() override {
    // clock_t start = clock();
    // A truncated (loop-guard) episode can leave stale message-buffer
    // indices; parsing leftovers as the new duel's stream desyncs everything.
    dp_ = 0;
    fdl_ = 0;
    chains_since_decision_ = 0;
    loop_cause_code_ = 0;
    if (random_mode()) {
      play_mode_ = play_modes_[dist_int_(gen_) % play_modes_.size()];
    } else {
      play_mode_ = play_modes_[0];
    }

    if (play_mode_ != kSelfPlay) {
      if (player_ == -1) {
        ai_player_ = dist_int_(gen_) % 2;
      } else {
        ai_player_ = player_;
      }
    }

    turn_count_ = 0;

    history_actions_0_.Zero();
    history_actions_1_.Zero();
    ha_p_0_ = 0;
    ha_p_1_ = 0;

    uint32_t forced_seed = static_cast<uint32_t>(spec_.config["duel_seed"_]);
    auto duel_seed = forced_seed != 0 ? forced_seed : dist_int_(gen_);
    duel_seed_ = duel_seed;
    action_history_.clear();

    constexpr uint32_t init_lp = 8000;
    constexpr uint32_t startcount = 5;
    constexpr uint32_t drawcount = 1;

    std::unique_lock<std::shared_timed_mutex> ulock(duel_mtx);
    auto opts = YGO_CreateDuel(duel_seed, init_lp, startcount, drawcount);
    ulock.unlock();

    for (PlayerId i = 0; i < 2; i++) {
      if (players_[i] != nullptr) {
        delete players_[i];
      }
      std::string nickname = i == 0 ? "Alice" : "Bob";
      if (i == ai_player_) {
        nickname = "Agent";
      }
      nickname_[i] = nickname;
      if ((play_mode_ == kHuman) && (i != ai_player_)) {
        players_[i] = new HumanPlayer(nickname_[i], init_lp, i, verbose_);
      } else if (play_mode_ == kRandomBot) {
        players_[i] = new RandomAI(max_options(), dist_int_(gen_), nickname_[i],
                                   init_lp, i, verbose_);
      } else {
        players_[i] = new GreedyAI(nickname_[i], init_lp, i, verbose_);
      }
      if (puzzle_path_.empty()) {
        load_deck(i);
      }
      lp_[i] = players_[i]->init_lp_;
    }
    if (!puzzle_path_.empty()) {
      std::ifstream pf(puzzle_path_, std::ios::binary);
      if (!pf) {
        throw std::runtime_error("puzzle script not found: " + puzzle_path_);
      }
      std::string script((std::istreambuf_iterator<char>(pf)),
                         std::istreambuf_iterator<char>());
      if (!OCG_LoadScript(pduel_, script.c_str(),
                          static_cast<uint32_t>(script.size()),
                          puzzle_path_.c_str())) {
        throw std::runtime_error("puzzle script failed to load: " +
                                 puzzle_path_);
      }
    }

    if (record_) {
      if (is_recording && fp_ != nullptr) {
        close_replay();
      }
      auto time_str = time_now();
      // Use last 4 digits of seed as unique id
      auto seed_ = duel_seed % 10000;
      std::string fname;
      while (true) {
        fname = fmt::format("./replay/a{} {:04d}.yrp", time_str, seed_);
        // check existence
        if (std::filesystem::exists(fname)) {
          seed_ = (seed_ + 1) % 10000;
        } else {
          break;
        } 
      }
      fp_ = fopen(fname.c_str(), "wb");
      if (!fp_) {
        throw std::runtime_error("Failed to open file for replay: " + fname);
      }

      is_recording = true;

      ReplayHeader rh;
      rh.type = REPLAY_YRP1;
      rh.version = 0x000A0128;
      rh.flags = REPLAY_LUA64 | REPLAY_64BIT_DUELFLAG | REPLAY_NEWREPLAY | REPLAY_EXTENDED_HEADER;
      rh.timestamp = (uint32_t)time(nullptr);

      ExtendedReplayHeader erh;
      erh.base = rh;
      erh.version = 1U;
      for (int i = 0; i < 4; i++) {
        erh.seed[i] = opts.seed[i];
      }

      fwrite(&erh, sizeof(erh), 1, fp_);

      for (PlayerId i = 0; i < 2; i++) {
        uint16_t name[20];
        memset(name, 0, 40);
        std::string name_str = fmt::format("{} {}", nickname_[i], deck_name_[i]);
        if (name_str.size() > 20) {
          // truncate
          name_str = name_str.substr(0, 20);
        }
        fmt::println("name: {}", name_str);
        str_to_uint16(name_str.c_str(), name);
        ReplayWriteInt32(1);
        fwrite(name, 40, 1, fp_);
      }

      ReplayWriteInt32(init_lp);
      ReplayWriteInt32(startcount);
      ReplayWriteInt32(drawcount);
      ReplayWriteInt64(opts.flags);

      for (PlayerId i = 0; i < 2; i++) {
        auto &main_deck = i == 0 ? main_deck0_ : main_deck1_;
        auto &extra_deck = i == 0 ? extra_deck0_ : extra_deck1_;
        ReplayWriteInt32(main_deck.size());
        for (auto code : main_deck) {
          ReplayWriteInt32(code);
        }
        ReplayWriteInt32(extra_deck.size());
        for (int j = int(extra_deck.size()) - 1; j >= 0; --j) {
        // for (int j = 0; j < extra_deck.size(); ++j) {
          ReplayWriteInt32(extra_deck[j]);
        }
      }

      ReplayWriteInt32(0);

    }

    YGO_StartDuel(pduel_);
    duel_started_ = true;
    winner_ = 255;
    win_reason_ = 255;

    next();

    done_ = false;
    elapsed_step_ = 0;
    ms_idx_ = -1;
    WriteState(0.0);

    // double seconds = static_cast<double>(clock() - start) / CLOCKS_PER_SEC;
    // // update reset_time by moving average
    // reset_time_ = reset_time_* (static_cast<double>(reset_time_count_) /
    // (reset_time_count_ + 1)) + seconds / (reset_time_count_ + 1);
    // reset_time_count_++;
    // if (reset_time_count_ % 20 == 0) {
    //   fmt::println("Reset time: {:.3f}", reset_time_);
    // }
  }

  void update_history_actions(PlayerId player, const LegalAction &action) {
    if (action.act_ == ActionAct::Cancel) {
      return;
    }
    auto &history_actions =
        player == 0 ? history_actions_0_ : history_actions_1_;
    auto &ha_p = player == 0 ? ha_p_0_ : ha_p_1_;

    ha_p--;
    if (ha_p < 0) {
      ha_p = n_history_actions_ - 1;
    }
    history_actions[ha_p].Zero();
    _set_obs_action(history_actions, ha_p, action);
    // Spec index is meaningless across states; cols 12/13 carry the turn
    // (converted to turn-diff at WriteState) and phase.
    history_actions[ha_p](0) = 0;
    history_actions[ha_p](12) = static_cast<uint8_t>(std::min(turn_count_, 255));
    history_actions[ha_p](13) = phase2id.at(current_phase_);
  }

  void show_deck(const std::vector<CardCode> &deck, const std::string &prefix) const {
    fmt::print("{} deck: [", prefix);
    for (int i = 0; i < deck.size(); i++) {
      fmt::print(" '{}'", c_get_card(deck[i]).name());
    }
    fmt::print(" ]\n");
  }

  void show_turn() const {
    fmt::println("turn: {}, phase: {}, tplayer: {}", turn_count_, phase_to_string(current_phase_), tp_);
  }

  void show_deck(PlayerId player) const {
    fmt::print("Player {}'s deck:\n", player);
    show_deck(player == 0 ? main_deck0_ : main_deck1_, "Main");
    show_deck(player == 0 ? extra_deck0_ : extra_deck1_, "Extra");
  }

  void show_history_actions(PlayerId player) const {
    const auto &ha = player == 0 ? history_actions_0_ : history_actions_1_;
    for (int i = 0; i < n_history_actions_; ++i) {
      uint8_t msg_id = uint8_t(ha(i, 3));
      if (msg_id == 0) {
        break;
      }
      int msg = _msgs[msg_id - 1];
      CardId cid = (CardId(uint8_t(ha(i, 1))) << 8) + uint8_t(ha(i, 2));
      fmt::print("history {}: msg {}, cid {},", i, msg_to_string(msg), cid);
      for (int j = 4; j < ha.Shape()[1]; j++) {
        fmt::print(" {}", uint8_t(ha(i, j)));
      }
      fmt::print("\n");
    }
  }

  void Step(const Action &action) override {
    // clock_t start = clock();

    int idx = action["action"_];
    action_history_.push_back(idx);
    prev_msg_for_retry_ = msg_;
    prev_option_for_retry_ = idx < options_.size() ? options_[idx] : "?";
    // Record before advancing: legal_actions_ still holds this decision's
    // typed actions (spec_index_/cid_ filled by WriteState).
    if (idx < legal_actions_.size()) {
      update_history_actions(to_play_, legal_actions_[idx]);
    }
    callback_(idx);

    PlayerId player = to_play_;

    if (verbose_) {
      show_decision(idx);
    }

    next();

    // Loop guard: Yu-Gi-Oh! admits unbounded games (arXiv:2603.02863), and a
    // wedged duel would silently occupy this env slot forever. Truncate as a
    // draw once the decision budget is exhausted.
    ++elapsed_step_;
    if (!done_ && max_episode_steps_ > 0 &&
        elapsed_step_ >= max_episode_steps_) {
      done_ = true;
      winner_ = 255;  // no winner: truncation draw
      if (record_) {
        close_replay();
      }
    }

    float reward = 0;
    int reason = 0;
    // winner_ == 255 means truncation draw (loop guard): reward stays 0.
    if (done_ && winner_ != 255) {
      // Upstream ygopro shaping, gated on greedy_reward: fast wins pay more,
      // with the going-first winner on a doubled ladder (turn 1 = FTK).
      float base_reward = 1.0;
      if (greedy_reward_) {
        if (winner_ == 0) {
          if (turn_count_ <= 1) {
            base_reward = 16.0;
          } else if (turn_count_ <= 3) {
            base_reward = 8.0;
          } else if (turn_count_ <= 5) {
            base_reward = 4.0;
          } else if (turn_count_ <= 7) {
            base_reward = 2.0;
          } else {
            base_reward = 0.5 + 1.0 / (turn_count_ - 7);
          }
        } else {
          if (turn_count_ <= 1) {
            base_reward = 8.0;
          } else if (turn_count_ <= 3) {
            base_reward = 4.0;
          } else if (turn_count_ <= 5) {
            base_reward = 2.0;
          } else {
            base_reward = 0.5 + 1.0 / (turn_count_ - 5);
          }
        }
      }
      if (play_mode_ == kSelfPlay) {
        // to_play_ is the previous player
        reward = winner_ == to_play_ ? base_reward : -base_reward;
      } else {
        reward = winner_ == ai_player_ ? base_reward : -base_reward;
      }

      if (win_reason_ == 0x01) {
        reason = 1;
      } else if (win_reason_ == 0x02) {
        reason = -1;
      }

      if (record_) {
        if (!is_recording || fp_ == nullptr) {
          throw std::runtime_error("Recording is not started");
        }
        close_replay();
      }
    }

    WriteState(reward, win_reason_);

    // double seconds = static_cast<double>(clock() - start) / CLOCKS_PER_SEC;
    // // update step_time by moving average
    // step_time_ = step_time_* (static_cast<double>(step_time_count_) /
    // (step_time_count_ + 1)) + seconds / (step_time_count_ + 1);
    // step_time_count_++;
    // if (step_time_count_ % 500 == 0) {
    //   fmt::println("Step time: {:.3f}", step_time_);
    // }
  }

private:

  // ygopro layout (D4): rows are contiguous across both players (no fixed
  // per-player halves), and the per-location card counts are returned for
  // global_ features 8..21. Defensive row clamp: a fully cycled 60-card deck
  // plus tokens can exceed the row budget, and an OOB write here corrupts
  // the obs arena silently.
  std::tuple<SpecInfos, std::vector<int>> _set_obs_cards(
      TArray<uint8_t> &f_cards, PlayerId to_play) {
    SpecInfos spec_infos;
    std::vector<int> loc_n_cards;
    const int max_rows = spec_.config["max_cards"_] * 2;
    int offset = 0;
    for (auto pi = 0; pi < 2; pi++) {
      const PlayerId player = (to_play + pi) % 2;
      const bool opponent = pi == 1;
      std::vector<std::pair<uint8_t, bool>> configs = {
          {LOCATION_DECK, true},   {LOCATION_HAND, true},
          {LOCATION_MZONE, false}, {LOCATION_SZONE, false},
          {LOCATION_GRAVE, false}, {LOCATION_REMOVED, false},
          {LOCATION_EXTRA, true},
      };
      for (auto &[location, hidden_for_opponent] : configs) {
        // check this
        if (opponent && (location == LOCATION_HAND) &&
            (revealed_.size() != 0)) {
          hidden_for_opponent = false;
        }
        if (opponent && hidden_for_opponent) {
          auto n_cards = YGO_QueryFieldCount(pduel_, player, location);
          loc_n_cards.push_back(n_cards);
          for (auto i = 0; i < n_cards; i++) {
            if (offset >= max_rows) {
              break;
            }
            f_cards(offset, 2) = location2id.at(location);
            f_cards(offset, 4) = 1;
            offset++;
          }
        } else {
          std::vector<Card> cards = get_cards_in_location(player, location);
          loc_n_cards.push_back(cards.size());
          for (int i = 0; i < cards.size(); ++i) {
            if (offset >= max_rows) {
              break;
            }
            const auto &c = cards[i];
            auto spec = c.get_spec(opponent);
            bool hide = false;
            if (opponent) {
              hide = c.position_ & POS_FACEDOWN;
              if ((location == LOCATION_HAND) &&
                  (std::find(revealed_.begin(), revealed_.end(), spec) !=
                   revealed_.end())) {
                hide = false;
              }
            }
            CardId card_id = 0;
            if (!hide) {
              card_id = c_get_card_id_or_zero(c.code_);
            }
            _set_obs_card_(f_cards, offset, c, hide);
            offset++;
            spec_infos[spec] = {static_cast<uint16_t>(offset), card_id};
          }
        }
      }
    }
    return {spec_infos, loc_n_cards};
  }

  static int deck_name_id(const std::string &name) {
    for (int i = 0; i < deck_names_.size(); ++i) {
      if (deck_names_[i] == name) {
        return i;
      }
    }
    return -1;
  }

  const SpecInfo &find_spec_info(SpecInfos &spec_infos,
                                 const std::string &spec) {
    auto it = spec_infos.find(spec);
    if (it == spec_infos.end()) {
      if (std::getenv("YGOENV_QUERY_DEBUG")) {
        fmt::println(stderr, "Spec not found: {}; spec_infos:", spec);
        for (auto &[k, v] : spec_infos) {
          fmt::print(stderr, "{}: {} {}, ", k, v.index, v.cid);
        }
        fmt::print(stderr, "\n");
      }
      spec_infos[spec] = {0, 0};
      return spec_infos[spec];
    }
    return it->second;
  }

  void _set_obs_card_(TArray<uint8_t> &f_cards, int offset, const Card &c,
                      bool hide) {
    uint8_t location = c.location_;
    bool overlay = location & LOCATION_OVERLAY;
    if (overlay) {
      location = location & 0x7f;
    }
    if (overlay) {
      hide = false;
    }

    if (!hide) {
      auto card_id = c_get_card_id(c.code_);
      f_cards(offset, 0) = static_cast<uint8_t>(card_id >> 8);
      f_cards(offset, 1) = static_cast<uint8_t>(card_id & 0xff);
    }
    f_cards(offset, 2) = location2id.at(location);

    uint8_t seq = 0;
    if (location == LOCATION_MZONE || location == LOCATION_SZONE ||
        location == LOCATION_GRAVE) {
      seq = c.sequence_ + 1;
    }
    f_cards(offset, 3) = seq;
    f_cards(offset, 4) = (c.controler_ != to_play_) ? 1 : 0;
    if (overlay) {
      f_cards(offset, 5) = position2id.at(POS_FACEUP);
      f_cards(offset, 6) = 1;
    } else if (location == LOCATION_DECK || location == LOCATION_HAND ||
               location == LOCATION_EXTRA) {
      // ygopro-layout convention: hidden-pile cards only carry an explicit
      // facedown marker; a visible hand card's position column stays 0.
      if (hide || (c.position_ & POS_FACEDOWN)) {
        f_cards(offset, 5) = position2id.at(POS_FACEDOWN);
      }
    } else {
      f_cards(offset, 5) = position2id.at(c.position_);
    }
    if (!hide) {
      f_cards(offset, 7) = attribute2id.at(c.attribute_);
      f_cards(offset, 8) = race2id.at(c.race_);
      f_cards(offset, 9) = c.level_;
      f_cards(offset, 10) = std::min(c.counter_, static_cast<uint32_t>(15));
      f_cards(offset, 11) = static_cast<uint8_t>(
          (c.status_ & (STATUS_DISABLED | STATUS_FORBIDDEN)) != 0);
      auto [atk1, atk2] = float_transform(c.attack_);
      f_cards(offset, 12) = atk1;
      f_cards(offset, 13) = atk2;

      auto [def1, def2] = float_transform(c.defense_);
      f_cards(offset, 14) = def1;
      f_cards(offset, 15) = def2;

      auto type_ids = type_to_ids(c.type_);
      for (int j = 0; j < type_ids.size(); ++j) {
        f_cards(offset, 16 + j) = type_ids[j];
      }
    }
  }

  // ygopro layout (D4): 23 features — 8 scalars, then the 14 per-location
  // card counts from _set_obs_cards, then the abnormal-termination flag [22].
  void _set_obs_global(TArray<uint8_t> &feat, PlayerId player,
                       const std::vector<int> &loc_n_cards) {
    uint8_t me = player;
    uint8_t op = 1 - player;

    auto [me_lp_1, me_lp_2] = float_transform(lp_[me]);
    feat(0) = me_lp_1;
    feat(1) = me_lp_2;

    auto [op_lp_1, op_lp_2] = float_transform(lp_[op]);
    feat(2) = op_lp_1;
    feat(3) = op_lp_2;

    feat(4) = std::min(turn_count_, 16);
    feat(5) = phase2id.at(current_phase_);
    feat(6) = (me == 0) ? 1 : 0;
    feat(7) = (me == tp_) ? 1 : 0;

    for (int i = 0; i < loc_n_cards.size() && i < 14; i++) {
      feat(8 + i) = static_cast<uint8_t>(std::min(loc_n_cards[i], 255));
    }
  }

  // --- 12-column typed action writer (ygopro layout, D4 Stage B) ---
  // 0 spec_index (row+1 into cards_) | 1-2 card id | 3 msg | 4 act |
  // 5 finish | 6 effect | 7 phase | 8 position | 9 number (also announced
  // race, keyed by msg) | 10 place | 11 attribute.
  void _set_obs_action_spec_idx(TArray<uint8_t> &feat, int i, int idx) {
    feat(i, 0) = static_cast<uint8_t>(std::min(idx, 255));
  }

  void _set_obs_action_card_id(TArray<uint8_t> &feat, int i, CardId cid) {
    feat(i, 1) = static_cast<uint8_t>(cid >> 8);
    feat(i, 2) = static_cast<uint8_t>(cid & 0xff);
  }

  void _set_obs_action_msg(TArray<uint8_t> &feat, int i, int msg) {
    feat(i, 3) = msg2id.at(msg);
  }

  void _set_obs_action_act(TArray<uint8_t> &feat, int i, ActionAct act) {
    feat(i, 4) = static_cast<uint8_t>(act);
  }

  void _set_obs_action_finish(TArray<uint8_t> &feat, int i) {
    feat(i, 5) = 1;
  }

  void _set_obs_action_effect(TArray<uint8_t> &feat, int i, int effect) {
    // 0: none | 1: default | 2-15: card effect index (clamped; modern
    // strindex can exceed 14) | 16+: curated system string, 255 unknown.
    int e;
    if (effect == -1) {
      e = 0;
    } else if (effect == 0) {
      e = 1;
    } else if (effect >= kCardEffectOffset) {
      e = std::min(effect - kCardEffectOffset + 2, 15);
    } else {
      e = system_string_to_id(effect);
    }
    feat(i, 6) = static_cast<uint8_t>(e);
  }

  void _set_obs_action_phase(TArray<uint8_t> &feat, int i, ActionPhase phase) {
    feat(i, 7) = static_cast<uint8_t>(phase);
  }

  void _set_obs_action_position(TArray<uint8_t> &feat, int i,
                                uint8_t position) {
    feat(i, 8) = position2id.at(position);
  }

  void _set_obs_action_number(TArray<uint8_t> &feat, int i, uint8_t number) {
    feat(i, 9) = number;
  }

  void _set_obs_action_place(TArray<uint8_t> &feat, int i, ActionPlace place) {
    feat(i, 10) = static_cast<uint8_t>(place);
  }

  void _set_obs_action_attrib(TArray<uint8_t> &feat, int i, uint8_t attrib) {
    feat(i, 11) = attribute2id.at(attrib);
  }

  void _set_obs_action(TArray<uint8_t> &feat, int i,
                       const LegalAction &action) {
    auto msg = action.msg_;
    _set_obs_action_msg(feat, i, msg);
    _set_obs_action_card_id(feat, i, action.cid_);
    if (msg == MSG_SELECT_CARD || msg == MSG_SELECT_TRIBUTE ||
        msg == MSG_SELECT_SUM || msg == MSG_SELECT_UNSELECT_CARD) {
      if (action.finish_) {
        _set_obs_action_finish(feat, i);
      } else {
        _set_obs_action_spec_idx(feat, i, action.spec_index_);
      }
    } else if (msg == MSG_SELECT_POSITION) {
      _set_obs_action_position(feat, i, action.position_);
    } else if (msg == MSG_SELECT_EFFECTYN) {
      _set_obs_action_spec_idx(feat, i, action.spec_index_);
      _set_obs_action_act(feat, i, action.act_);
      _set_obs_action_effect(feat, i, action.effect_);
    } else if (msg == MSG_SELECT_YESNO || msg == MSG_SELECT_OPTION) {
      _set_obs_action_act(feat, i, action.act_);
      _set_obs_action_effect(feat, i, action.effect_);
    } else if (msg == MSG_SELECT_BATTLECMD || msg == MSG_SELECT_IDLECMD ||
               msg == MSG_SELECT_CHAIN) {
      _set_obs_action_phase(feat, i, action.phase_);
      _set_obs_action_spec_idx(feat, i, action.spec_index_);
      _set_obs_action_act(feat, i, action.act_);
      _set_obs_action_effect(feat, i, action.effect_);
    } else if (msg == MSG_SELECT_PLACE || msg == MSG_SELECT_DISFIELD) {
      _set_obs_action_place(feat, i, action.place_);
    } else if (msg == MSG_ANNOUNCE_CARD) {
      // card id, already set
    } else if (msg == MSG_ANNOUNCE_ATTRIB) {
      _set_obs_action_attrib(feat, i, action.attribute_);
    } else if (msg == MSG_ANNOUNCE_NUMBER) {
      _set_obs_action_number(feat, i, action.number_);
    } else if (msg == MSG_ANNOUNCE_RACE) {
      // No dedicated column upstream; the race id rides the number column,
      // keyed by the msg column.
      auto it = race2id.find(static_cast<uint32_t>(action.race_));
      _set_obs_action_number(feat, i, it != race2id.end() ? it->second : 0);
    } else {
      throw std::runtime_error("Unsupported message " + msg_to_string(msg));
    }
  }

  void _set_obs_actions(TArray<uint8_t> &feat,
                        const std::vector<LegalAction> &actions) {
    for (int i = 0; i < actions.size(); ++i) {
      _set_obs_action(feat, i, actions[i]);
    }
  }

  CardId spec_to_card_id(const std::string &spec, PlayerId player) {
    int offset = 0;
    if (spec[0] == 'o') {
      player = 1 - player;
      offset++;
    }
    auto [loc, seq, pos] = spec_to_ls(spec.substr(offset));
    return card_ids_.at(get_card_code(player, loc, seq));
  }

  void str_to_uint16(const char* src, uint16_t* dest) {
      for (int i = 0; i < strlen(src); i += 1) {
        dest[i] = src[i];
      }

      // Add null terminator
      dest[strlen(src) + 1] = '\0';
  }

  void ReplayWriteInt8(int8_t value) {
    fwrite(&value, sizeof(value), 1, fp_);
  }

  void ReplayWriteInt32(int32_t value) {
    fwrite(&value, sizeof(value), 1, fp_);
  }

  void ReplayWriteInt64(uint64_t value) {
    fwrite(&value, sizeof(value), 1, fp_);
  }

  // edopro-core API
  OCG_DuelOptions YGO_CreateDuel(uint32_t seed, uint32_t init_lp, uint32_t startcount, uint32_t drawcount) {
    SplitMix64 generator(seed);
    OCG_DuelOptions opts;
    for (int i = 0; i < 4; i++) {
      opts.seed[i] = generator(); 
    }
    // from edopro-deskbot/src/client.cpp#L46
    opts.flags = duel_options_;
    opts.team1 = {init_lp, startcount, drawcount};
    opts.team2 = {init_lp, startcount, drawcount};
    opts.cardReader = &g_DataReader;
    opts.payload1 = nullptr;
    opts.scriptReader = &g_ScriptReader;
    opts.payload2 = nullptr;

		// opts.logHandler = [](void* /*payload*/, const char* /*string*/, int /*type*/) {};
    opts.logHandler = &g_LogHandler;
		opts.payload3 = nullptr;

		opts.cardReaderDone = [](void* /*payload*/, OCG_CardData* /*data*/) {};
		opts.payload4 = nullptr;

    opts.enableUnsafeLibraries = 1;
    int create_status = OCG_CreateDuel(&pduel_, &opts);
    if (create_status != OCG_DUEL_CREATION_SUCCESS) {
      throw std::runtime_error("Failed to create duel");
    }
    g_ScriptReader(nullptr, pduel_, "constant.lua");
    g_ScriptReader(nullptr, pduel_, "utility.lua");
    return opts;
  }

  void YGO_NewCard(OCG_Duel pduel, uint32_t code, uint8_t owner, uint8_t playerid, uint8_t location, uint8_t sequence, uint8_t position) {
    OCG_NewCardInfo info;
    info.team = playerid;
    info.duelist = 0;
    info.code = code;
    info.con = owner;
    info.loc = location;
    info.seq = sequence;
    info.pos = position;
    OCG_DuelNewCard(pduel, &info);
  }

  void YGO_StartDuel(OCG_Duel pduel) {
    OCG_StartDuel(pduel);
  }

  void YGO_EndDuel(OCG_Duel pduel) {
    OCG_DestroyDuel(pduel);
  }

  uint32_t YGO_GetMessage(OCG_Duel pduel, uint8_t* buf) {
    uint32_t len;
    auto buf_ = OCG_DuelGetMessage(pduel, &len);
    memcpy(buf, buf_, len);
    return len;
  }

  int YGO_Process(OCG_Duel pduel) {
    return OCG_DuelProcess(pduel);
  }

  int32_t YGO_QueryCard(OCG_Duel pduel, uint8_t playerid, uint8_t location, uint8_t sequence, uint32_t query_flag, uint8_t* buf) {
    // TODO: overlay
    OCG_QueryInfo info = {query_flag, playerid, location, sequence};
    uint32_t length;
    auto buf_ = static_cast<uint8_t*>(OCG_DuelQuery(pduel, &length, &info));
    if (length > 0) {
      memcpy(buf, buf_, length);      
    }
    return length;
  }

  int32_t YGO_QueryFieldCount(OCG_Duel pduel, uint8_t playerid, uint8_t location) {
    return OCG_DuelQueryCount(pduel, playerid, location);
  }

  int32_t OCG_QueryFieldCard(OCG_Duel pduel, uint8_t playerid, uint8_t location, uint32_t query_flag, uint8_t* buf, int32_t use_cache) {
    // TODO: overlay
    OCG_QueryInfo info = {query_flag, playerid, location};
    uint32_t length;
    auto buf_ = static_cast<uint8_t*>(OCG_DuelQueryLocation(pduel, &length, &info));
    if (length > 0) {
      memcpy(buf, buf_, length);      
    }
    return length;
  }

  void YGO_SetResponsei(OCG_Duel pduel, int32_t value) {
    prev_msg_for_retry_ = msg_;
    prev_option_for_retry_ = fmt::format("i:{}", value);
    if (record_) {
      ReplayWriteInt8(4);
      ReplayWriteInt32(value);
    }
    uint32_t len = sizeof(value);
    memcpy(resp_buf_, &value, len);
    OCG_DuelSetResponse(pduel, resp_buf_, len);
  }

  void YGO_SetResponseb(OCG_Duel pduel, uint8_t* buf, uint32_t len = 0) {
    prev_msg_for_retry_ = msg_;
    prev_option_for_retry_ = fmt::format("b:len={}", len);
    if (record_) {
      if (len == 0) {
        // len = buf[0];
        // ReplayWriteInt8(len);
        // fwrite(buf + 1, len, 1, fp_);
        fwrite(buf, len, 1, fp_);
      } else {
        ReplayWriteInt8(len);
        fwrite(buf, len, 1, fp_);
      }
    }
    if (len == 0) {
      len = buf[0];
      OCG_DuelSetResponse(pduel, buf + 1, len);
    } else {
      OCG_DuelSetResponse(pduel, buf, len);
    }
  }

  // edopro-core API

  void WriteState(float reward, int win_reason = 0) {
    State state = Allocate();

    int n_options = options_.size();
    state["reward"_] = reward;
    state["info:to_play"_] = int(to_play_);
    state["info:is_selfplay"_] = int(play_mode_ == kSelfPlay);
    state["info:win_reason"_] = win_reason;
    // Episode end (done_), not reward != 0: truncation draws end with
    // reward 0 and still need their matchup label for eval bucketing.
    if (done_) {
      state["info:step_time"_][0] = 0;
      state["info:step_time"_][1] = 0;
      state["info:deck"_][0] = deck_name_id(deck_name_[0]);
      state["info:deck"_][1] = deck_name_id(deck_name_[1]);
    }

    if (n_options == 0) {
      state["info:num_options"_] = 1;
      // ygopro layout: global_[22] is the abnormal-termination flag.
      state["obs:global_"_][22] = uint8_t(1);
      return;
    }

    if (options_.size() != legal_actions_.size()) {
      // Migration invariant: every handler emits typed actions in lockstep
      // with its option strings. A mismatch means an unported emission path.
      throw std::runtime_error(fmt::format(
          "options/legal_actions mismatch at msg {}: {} vs {}",
          msg_to_string(msg_), options_.size(), legal_actions_.size()));
    }

    auto [spec_infos, loc_n_cards] =
        _set_obs_cards(state["obs:cards_"_], to_play_);

    _set_obs_global(state["obs:global_"_], to_play_, loc_n_cards);

    // we can't shuffle because idx must be stable in callback
    if (n_options > max_options()) {
      // This CLIPS legal play (drops trailing options). Diagnostic keeps the
      // rate measurable in run logs; the fix is raising max_options.
      fmt::print(stderr, "[optclip] msg={} options={} max={}\n",
                 msg_to_string(msg_), n_options, max_options());
      options_.resize(max_options());
      legal_actions_.resize(max_options());
    }

    n_options = options_.size();
    state["info:num_options"_] = n_options;

    for (int i = 0; i < n_options; ++i) {
      auto &action = legal_actions_[i];
      action.msg_ = msg_;
      const auto &spec = action.spec_;
      if (!spec.empty()) {
        const auto &spec_info = find_spec_info(spec_infos, spec);
        action.spec_index_ = spec_info.index;
        if (action.cid_ == 0) {
          action.cid_ = spec_info.cid;
        }
      }
    }

    _set_obs_actions(state["obs:actions_"_], legal_actions_);

    // write history actions (newest first), then convert the stored turn
    // number into a turn-diff relative to now
    const auto &ha_p = to_play_ == 0 ? ha_p_0_ : ha_p_1_;
    const auto &history_actions =
        to_play_ == 0 ? history_actions_0_ : history_actions_1_;
    int n1 = n_history_actions_ - ha_p;
    int n_h_action_feats = history_actions.Shape()[1];

    state["obs:h_actions_"_].Assign((uint8_t *)history_actions[ha_p].Data(),
                                    n_h_action_feats * n1);
    state["obs:h_actions_"_][n1].Assign((uint8_t *)history_actions.Data(),
                                        n_h_action_feats * ha_p);
    for (int i = 0; i < n_history_actions_; ++i) {
      if (uint8_t(state["obs:h_actions_"_](i, 3)) == 0) {
        break;
      }
      int turn_diff = std::min(
          16, turn_count_ - int(uint8_t(state["obs:h_actions_"_](i, 12))));
      state["obs:h_actions_"_](i, 12) = static_cast<uint8_t>(turn_diff);
    }
  }

  int prev_msg_for_retry_ = 0;
  std::string prev_option_for_retry_;

  // Finalise a replay: YRP1's header carries the payload size, and EDOPro
  // uses it to read the body. The writer never set it, producing files with
  // size=0 that the client cannot load.
  void close_replay() {
    if (fp_ == nullptr) {
      is_recording = false;
      return;
    }
    long end = ftell(fp_);
    long payload = end - static_cast<long>(sizeof(ExtendedReplayHeader));
    if (payload > 0) {
      uint32_t sz = static_cast<uint32_t>(payload);
      fseek(fp_, offsetof(ReplayHeader, size), SEEK_SET);
      fwrite(&sz, sizeof(sz), 1, fp_);
    }
    fclose(fp_);
    fp_ = nullptr;
    is_recording = false;
  }

  void report_loop_incident(const char *cause) {
    if (std::getenv("YGOENV_NO_INCIDENTS")) {
      return;
    }
    auto join = [](const std::vector<CardCode> &v) {
      std::string out;
      for (size_t i = 0; i < v.size(); ++i) {
        if (i) out += ",";
        out += std::to_string(v[i]);
      }
      return out;
    };
    std::string acts;
    acts.reserve(action_history_.size() * 4);
    for (size_t i = 0; i < action_history_.size(); ++i) {
      if (i) acts += ",";
      acts += std::to_string(action_history_[i]);
    }
    fmt::print(stderr,
               "[incident] {{\"cause\":\"{}\",\"build\":\"{}\","
               "\"deck1\":\"{}\",\"deck2\":\"{}\","
               "\"duel_seed\":{},\"turn\":{},\"last_chain_code\":{},"
               "\"chains_since_decision\":{},\"actions\":[{}],"
               "\"main0\":\"{}\",\"extra0\":\"{}\","
               "\"main1\":\"{}\",\"extra1\":\"{}\"}}\n",
               cause, YGOENV_BUILD_ID, deck_name_[0], deck_name_[1], duel_seed_,
               turn_count_,
               loop_cause_code_, chains_since_decision_, acts,
               join(main_deck0_), join(extra_deck0_),
               join(main_deck1_), join(extra_deck1_));
  }

  void show_decision(int idx) {
    fmt::println("Player {} chose \"{}\" in {}", to_play_, options_[idx],
                 options_);
  }

  void load_deck(PlayerId player, bool shuffle = true) {
    std::string deck = player == 0 ? deck1_ : deck2_;
    std::vector<CardCode> &main_deck = player == 0 ? main_deck0_ : main_deck1_;
    std::vector<CardCode> &extra_deck =
        player == 0 ? extra_deck0_ : extra_deck1_;

    if (deck == "random") {
      // generate random deck name
      std::uniform_int_distribution<uint64_t> dist_int(0,
                                                       deck_names_.size() - 1);
      deck_name_[player] = deck_names_[dist_int(gen_)];
    } else {
      deck_name_[player] = deck;
    }
    deck = deck_name_[player];

    main_deck = main_decks_.at(deck);
    extra_deck = extra_decks_.at(deck);

    if (verbose_) {
      fmt::println("{} {}: {}, main({}), extra({})", player, nickname_[player],
        deck, main_deck.size(), extra_deck.size());
    }

    if (!deck_order_file_.empty()) {
      // Exact reproduction: use the recorded post-shuffle order.
      std::ifstream df(deck_order_file_);
      std::string line;
      std::string want_main = fmt::format("main{}:", (int)player);
      std::string want_extra = fmt::format("extra{}:", (int)player);
      while (std::getline(df, line)) {
        std::vector<CardCode> *dst = nullptr;
        if (line.rfind(want_main, 0) == 0) {
          dst = &main_deck;
        } else if (line.rfind(want_extra, 0) == 0) {
          dst = &extra_deck;
        } else {
          continue;
        }
        dst->clear();
        std::string nums = line.substr(line.find(':') + 1);
        size_t pos = 0;
        while (pos < nums.size()) {
          size_t comma = nums.find(',', pos);
          if (comma == std::string::npos) comma = nums.size();
          std::string tok = nums.substr(pos, comma - pos);
          if (!tok.empty()) dst->push_back(std::stoul(tok));
          pos = comma + 1;
        }
      }
    } else if (shuffle) {
      std::shuffle(main_deck.begin(), main_deck.end(), gen_);
    }

    // add main deck in reverse order following ygopro
    // but since we have shuffled deck, so just add in order

    for (int i = 0; i < main_deck.size(); i++) {
      YGO_NewCard(pduel_, main_deck[i], player, player, LOCATION_DECK, 0, POS_FACEDOWN_DEFENSE);
    }

    // TODO: check this for EDOPro
    // add extra deck in reverse order following ygopro
    for (int i = int(extra_deck.size()) - 1; i >= 0; --i) {
      YGO_NewCard(pduel_, extra_deck[i], player, player, LOCATION_EXTRA, 0, POS_FACEDOWN_DEFENSE);
    }
  }

  void next() {
    // Per-decision processing budget (loop guard, layer a): an involuntary
    // multi-card loop cycles this drive loop with every script call finite.
    // Victory-condition loops (deck-out, LP 0) resolve naturally well inside
    // the budget; tripping it means no victory condition is reachable.
    // Interim adjudication: truncation draw. TODO(policy): emulate the
    // Tournament Policy v2.5 judge call instead (send the primary-cause card
    // to the GY via injected Lua and continue play).
    int64_t process_iters = 0;
    const int64_t kProcessBudget = 100000;
    while (duel_started_) {
      if (duel_status_ == OCG_DUEL_STATUS_END) {
        break;
      }

      if (++process_iters >= kProcessBudget) {
        report_loop_incident("process_budget");
        done_ = true;
        winner_ = 255;  // truncation draw
        duel_started_ = false;
        dp_ = 0;
        fdl_ = 0;
        ms_idx_ = -1;
        return;
      }

      if (dp_ == fdl_ && ms_idx_ == -1) {
        duel_status_ = YGO_Process(pduel_);
        fdl_ = YGO_GetMessage(pduel_, data_);
        if (fdl_ == 0) {
          if (duel_status_ == OCG_DUEL_STATUS_CONTINUE) {
            report_loop_incident("lua_budget");
            // CONTINUE with an empty buffer only happens when the core's
            // script instruction budget tripped (see the lua-budget core
            // patch): the duel is stuck in an unbounded loop. Interim
            // adjudication: truncation draw. TODO(policy): Tournament Policy
            // v2.5 judge emulation (remove the primary-cause card, resume).
            done_ = true;
            winner_ = 255;
            duel_started_ = false;
            dp_ = 0;
            fdl_ = 0;
            ms_idx_ = -1;
            return;
          }
          continue;
        }
        dp_ = 0;
      }
      while (dp_ != fdl_ || ms_idx_ != -1) {
        if (ms_idx_ != -1) {
          // Mid multi-select: present the next iteration of the selection
          // sequence; no new core message is consumed.
          ms_present();
        } else {
        handle_message();
        // A handler may end the duel mid-buffer (loop-guard truncation,
        // MSG_RETRY abort, MSG_WIN). The remaining bytes belong to a duel
        // that no longer exists, and dp_/fdl_ have been reset, so continuing
        // to parse them reads garbage. Break rather than return: the
        // end-of-duel bookkeeping (done_ = true, options_.clear()) lives
        // after the loops, and _duel_end() does not set done_ itself.
        if (!duel_started_) {
          break;
        }
        // Desync guard: a handler that under-reads its payload would make the
        // next message parse from garbage (and can run off the buffer). Snap
        // to the declared end and warn so goldens surface the bug.
        if (dp_ != dl_) {
          if (std::getenv("YGOENV_CORE_LOG") || verbose_) {
            fmt::print(stderr,
                       "[env] WARNING: msg {} consumed {} of {} bytes\n",
                       msg_, dp_, dl_);
          }
          dp_ = dl_;
        }
        if (options_.empty()) {
          if (chains_since_decision_ > kInvoluntaryLoopChains) {
            if (std::getenv("YGOENV_CORE_LOG")) {
              fmt::print(stderr,
                         "[env] involuntary loop: {} chains since last "
                         "decision, primary cause code {}\n",
                         chains_since_decision_, loop_cause_code_);
            }
            chains_since_decision_ = 0;
          }
          continue;
        }
        // A decision means the player can stop: by policy this is a
        // controlled loop, not an infinite one.
        if (chains_since_decision_ >= 50 && std::getenv("YGOENV_LOOP_STATS")) {
          fmt::print(stderr, "[loopstats] chains_before_decision={}\n",
                     chains_since_decision_);
        }
        chains_since_decision_ = 0;
        }
        if ((play_mode_ == kSelfPlay) || (to_play_ == ai_player_)) {
          if (options_.size() == 1) {
            prev_msg_for_retry_ = msg_;
            prev_option_for_retry_ = options_[0];
            callback_(0);
            {
              auto la = legal_actions_[0];
              la.msg_ = msg_;
              if (la.cid_ == 0 && !la.spec_.empty()) {
                la.cid_ = spec_to_card_id(la.spec_, to_play_);
              }
              update_history_actions(to_play_, la);
            }
            if (verbose_) {
              show_decision(0);
            }
          } else {
            return;
          }
        } else {
          auto idx = players_[to_play_]->think(options_);
          prev_msg_for_retry_ = msg_;
    prev_option_for_retry_ = idx < options_.size() ? options_[idx] : "?";
    callback_(idx);
          if (verbose_) {
            show_decision(idx);
          }
        }
      }
    }
    done_ = true;
    options_.clear();
    legal_actions_.clear();
  }

  uint8_t read_u8() { return data_[dp_++]; }

  uint16_t read_u16() {
    uint16_t v = *reinterpret_cast<uint16_t *>(data_ + dp_);
    dp_ += 2;
    return v;
  }

  uint32_t read_u32() {
    uint32_t v = *reinterpret_cast<uint32_t *>(data_ + dp_);
    dp_ += 4;
    return v;
  }

  uint64_t read_u64() {
    uint64_t v;
    memcpy(&v, data_ + dp_, 8);
    dp_ += 8;
    return v;
  }

  template<typename T1, typename T2>
  T2 compat_read() {
    if(compat_mode_) {
      T1 v = *reinterpret_cast<T1 *>(data_ + dp_);
      dp_ += sizeof(T1);
      return static_cast<T2>(v);
    }
    T2 v = *reinterpret_cast<T2 *>(data_ + dp_);
    dp_ += sizeof(T2);
    return v;
  }

  uint32_t q_read_u8() {
    qdp_ += 6;
    uint8_t v = *reinterpret_cast<uint8_t *>(query_buf_ + qdp_);
    qdp_ += 1;
    return v;
  }

  uint32_t q_read_u16_() {
    uint32_t v = *reinterpret_cast<uint16_t *>(query_buf_ + qdp_);
    qdp_ += 2;
    return v;
  }

  uint32_t q_read_u16() {
    qdp_ += 6;
    uint32_t v = *reinterpret_cast<uint16_t *>(query_buf_ + qdp_);
    qdp_ += 2;
    return v;
  }

  uint32_t q_read_u32() {
    qdp_ += 6;
    uint32_t v = *reinterpret_cast<uint32_t *>(query_buf_ + qdp_);
    qdp_ += 4;
    return v;
  }

  uint32_t q_read_u32_() {
    uint32_t v = *reinterpret_cast<uint32_t *>(query_buf_ + qdp_);
    qdp_ += 4;
    return v;
  }

  // --- TLV query parsing (modern edopro-core format) ---
  // Each queried card is serialized as {u16 size, u32 query, payload[size-4]}
  // entries terminated by {u16 4, u32 QUERY_END}; an empty zone slot is a
  // single u16 0. See card::get_infos in ygopro-core/card.cpp.

  struct QueryResult {
    bool present = false;
    CardCode code = 0;
    uint32_t position = 0;
    uint32_t level = 0;
    uint32_t rank = 0;
    uint32_t attack = 0;
    uint32_t defense = 0;
    uint32_t lscale = 0;
    uint32_t rscale = 0;
    uint32_t link = 0;
    uint32_t link_marker = 0;
    uint32_t counter = 0;
    uint32_t status = 0;
    std::vector<CardCode> overlay_codes;
  };

  uint32_t q_at_u32(int off) const {
    uint32_t v;
    memcpy(&v, query_buf_ + off, 4);
    return v;
  }

  // Parses one card block starting at qdp_; advances qdp_ past it.
  // Returns false for an empty slot or an exhausted/truncated buffer.
  bool q_parse_card_tlv(int32_t bl, QueryResult &r) {
    if (qdp_ + 2 > bl) {
      return false;
    }
    {
      uint16_t first;
      memcpy(&first, query_buf_ + qdp_, 2);
      if (first == 0) {
        qdp_ += 2;
        return false;
      }
    }
    while (qdp_ + 2 <= bl) {
      uint16_t size;
      memcpy(&size, query_buf_ + qdp_, 2);
      qdp_ += 2;
      if (size < 4 || qdp_ + size > bl) {
        qdp_ = bl;
        break;
      }
      int next = qdp_ + static_cast<int>(size);
      uint32_t q = q_at_u32(qdp_);
      int payload = qdp_ + 4;
      switch (q) {
      case QUERY_CODE:
        r.code = q_at_u32(payload);
        break;
      case QUERY_POSITION:
        r.position = q_at_u32(payload);
        break;
      case QUERY_LEVEL:
        r.level = q_at_u32(payload);
        break;
      case QUERY_RANK:
        r.rank = q_at_u32(payload);
        break;
      case QUERY_ATTACK:
        r.attack = q_at_u32(payload);
        break;
      case QUERY_DEFENSE:
        r.defense = q_at_u32(payload);
        break;
      case QUERY_LSCALE:
        r.lscale = q_at_u32(payload);
        break;
      case QUERY_RSCALE:
        r.rscale = q_at_u32(payload);
        break;
      case QUERY_LINK:
        r.link = q_at_u32(payload);
        r.link_marker = q_at_u32(payload + 4);
        break;
      case QUERY_OVERLAY_CARD: {
        uint32_t n = q_at_u32(payload);
        for (uint32_t i = 0; i < n; ++i) {
          r.overlay_codes.push_back(q_at_u32(payload + 4 + 4 * i));
        }
        break;
      }
      case QUERY_COUNTERS: {
        uint32_t n = q_at_u32(payload);
        if (n > 0) {
          r.counter = q_at_u32(payload + 4);
        }
        break;
      }
      case QUERY_STATUS:
        r.status = q_at_u32(payload);
        break;
      default:
        break;
      }
      qdp_ = next;
      if (q == QUERY_END) {
        break;
      }
    }
    r.present = r.code != 0;
    return r.present;
  }

  Card q_result_to_card(const QueryResult &r, PlayerId player, uint8_t loc,
                        uint32_t seq) const {
    Card c = c_get_card(r.code);
    c.controler_ = player;
    c.location_ = loc;
    c.sequence_ = seq;
    c.position_ = r.position;
    if ((r.level & 0xff) > 0) {
      c.level_ = r.level & 0xff;
    }
    if ((r.rank & 0xff) > 0) {
      c.level_ = r.rank & 0xff;
    }
    c.attack_ = r.attack;
    c.defense_ = r.defense;
    c.lscale_ = r.lscale;
    c.rscale_ = r.rscale;
    // Pre-existing convention: link rating stored in level_, markers in
    // defense_.
    if ((r.link & 0xff) > 0) {
      c.level_ = r.link & 0xff;
    }
    if (r.link_marker > 0) {
      c.defense_ = r.link_marker;
    }
    c.counter_ = r.counter;
    c.status_ = r.status;
    return c;
  }

  CardCode get_card_code(PlayerId player, uint8_t loc, uint8_t seq) {
    int32_t flags = QUERY_CODE;
    int32_t bl = YGO_QueryCard(pduel_, player, loc, seq, flags, query_buf_);
    qdp_ = 0;
    QueryResult r;
    if (bl <= 0 || !q_parse_card_tlv(bl, r)) {
      throw std::runtime_error("[get_card_code] Invalid card");
    }
    return r.code;
  }

  Card get_card(PlayerId player, uint8_t loc, uint8_t seq) {
    int32_t flags = QUERY_CODE | QUERY_POSITION | QUERY_LEVEL | QUERY_RANK |
                    QUERY_ATTACK | QUERY_DEFENSE | QUERY_LSCALE | QUERY_RSCALE |
                    QUERY_LINK;
    int32_t bl = YGO_QueryCard(pduel_, player, loc, seq, flags, query_buf_);
    qdp_ = 0;
    QueryResult r;
    if (bl <= 0 || !q_parse_card_tlv(bl, r)) {
      std::string err = fmt::format("Player: {}, loc: {}, seq: {}, length: {}",
                                    player, loc, seq, bl);
      throw std::runtime_error("[get_card] Invalid card " + err);
    }
    return q_result_to_card(r, player, loc, seq);
  }

  std::vector<Card> get_cards_in_location(PlayerId player, uint8_t loc) {
    int32_t flags = QUERY_CODE | QUERY_POSITION | QUERY_LEVEL | QUERY_RANK |
                    QUERY_ATTACK | QUERY_DEFENSE | QUERY_EQUIP_CARD |
                    QUERY_OVERLAY_CARD | QUERY_COUNTERS | QUERY_STATUS |
                    QUERY_LSCALE | QUERY_RSCALE | QUERY_LINK;
    int32_t bl = OCG_QueryFieldCard(pduel_, player, loc, flags, query_buf_, 0);
    if (std::getenv("YGOENV_QUERY_DEBUG")) {
      fmt::print(stderr, "[qdbg] player={} loc={} bl={} bytes:", player, loc, bl);
      for (int i = 0; i < std::min(bl, 32); ++i) {
        fmt::print(stderr, " {:02x}", query_buf_[i]);
      }
      fmt::print("\n");
    }
    // OCG_DuelQueryLocation prefixes the buffer with a u32 total length
    // (single-card OCG_DuelQuery does not).
    qdp_ = 4;
    std::vector<Card> cards;
    uint32_t seq = 0;
    while (qdp_ + 2 <= bl) {
      QueryResult r;
      if (q_parse_card_tlv(bl, r)) {
        cards.push_back(q_result_to_card(r, player, loc, seq));
        for (uint32_t i = 0; i < r.overlay_codes.size(); ++i) {
          Card c_ = c_get_card(r.overlay_codes[i]);
          c_.controler_ = player;
          c_.location_ = loc | LOCATION_OVERLAY;
          c_.sequence_ = seq;
          c_.position_ = i;
          cards.push_back(c_);
        }
      }
      ++seq;
    }
    return cards;
  }


  std::vector<Card> read_cardlist(bool extra = false, bool extra8 = false) {
    std::vector<Card> cards;
    auto count = read_u8();
    cards.reserve(count);
    for (int i = 0; i < count; ++i) {
      auto code = read_u32();
      auto controller = read_u8();
      auto loc = read_u8();
      auto seq = read_u8();
      auto card = get_card(controller, loc, seq);
      if (extra) {
        if (extra8) {
          card.data_ = read_u8();
        } else {
          card.data_ = read_u32();
        }
      }
      cards.push_back(card);
    }
    return cards;
  }

  std::vector<IdleCardSpec> read_cardlist_spec(
    bool u32_seq = true, bool extra = false, bool extra8 = false) {
    std::vector<IdleCardSpec> card_specs;
    // TODO: different with ygopro-core
    auto count = compat_read<uint8_t, uint32_t>();
    card_specs.reserve(count);
    for (int i = 0; i < count; ++i) {
      CardCode code = read_u32();
      auto controller = read_u8();
      auto loc = read_u8();
      // TODO: different with ygopro-core
      uint32_t seq;
      if (u32_seq) {
        seq = compat_read<uint8_t, uint32_t>();;
      } else {
        seq = read_u8();
      }
      uint64_t data = -1;
      if (extra) {
        data = compat_read<uint32_t, uint64_t>();
        if (!compat_mode_) {
          // TODO: handle this
          read_u8();
        }
      }
      if (extra8) {
        read_u8();
      }
      card_specs.push_back({code, ls_to_spec(loc, seq, 0), data});
    }
    return card_specs;
  }

  loc_info read_loc_info() {
    loc_info info;
    info.controler = read_u8();
    info.location = read_u8();
    if (compat_mode_) {
      info.sequence = read_u8();
      info.position = read_u8();
    } else {
      info.sequence = read_u32();
      info.position = read_u32();
    }
    return info;
  }

  std::string cardlist_info_for_player(const Card &card, PlayerId pl) {
    std::string spec = card.get_spec(pl);
    if (card.location_ == LOCATION_DECK) {
      spec = "deck";
    }
    if ((card.controler_ != pl) && (card.position_ & POS_FACEDOWN)) {
      return position2str.at(card.position_) + "card (" + spec + ")";
    }
    return card.name_ + " (" + spec + ")";
  }

  void ms_present() {
    options_.clear();
    legal_actions_.clear();
    if (ms_mode_ == 0) {
      for (int j = 0; j < ms_specs_.size(); ++j) {
        if (ms_spec2idx_.find(ms_specs_[j]) != ms_spec2idx_.end()) {
          push_action(LegalAction::from_spec(ms_specs_[j]), ms_specs_[j]);
        }
      }
      if (static_cast<int>(ms_r_idxs_.size()) >= ms_min_) {
        push_action(LegalAction::finish(), "f");
      }
    } else {
      std::set<int> firsts;
      for (const auto &c : ms_combs_) {
        firsts.insert(c[0]);
      }
      for (int i : firsts) {
        push_action(LegalAction::from_spec(ms_specs_[i]), ms_specs_[i]);
      }
    }
    callback_ = [this](int idx) { ms_callback(idx); };
  }

  void ms_finish() {
    ms_idx_ = -1;
    ms_send_(ms_r_idxs_);
  }

  void ms_callback(int idx) {
    const auto &action = legal_actions_[idx];
    if (action.finish_) {
      ms_finish();
      return;
    }
    auto it = ms_spec2idx_.find(action.spec_);
    if (it == ms_spec2idx_.end()) {
      throw std::runtime_error("multi-select spec not found: " + action.spec_);
    }
    int sel = it->second;
    ms_r_idxs_.push_back(sel);
    if (ms_mode_ == 0) {
      ms_spec2idx_.erase(it);
      if (static_cast<int>(ms_r_idxs_.size()) >= ms_max_) {
        ms_finish();
        return;
      }
    } else {
      std::vector<std::vector<int>> remaining;
      bool complete = false;
      for (auto &c : ms_combs_) {
        if (c[0] == sel) {
          c.erase(c.begin());
          if (c.empty()) {
            complete = true;
            break;
          }
          remaining.push_back(std::move(c));
        }
      }
      if (complete) {
        ms_finish();
        return;
      }
      ms_combs_ = std::move(remaining);
    }
    ms_idx_++;
  }

  void init_multi_select(
      int min, int max, const std::vector<std::string> &specs, int mode,
      std::vector<std::vector<int>> combs,
      std::function<void(const std::vector<int> &)> send) {
    ms_idx_ = 0;
    ms_mode_ = mode;
    ms_min_ = min;
    ms_max_ = max;
    ms_specs_ = specs;
    ms_combs_ = std::move(combs);
    ms_r_idxs_.clear();
    ms_spec2idx_.clear();
    ms_send_ = std::move(send);
    for (int j = 0; j < ms_specs_.size(); ++j) {
      ms_spec2idx_[ms_specs_[j]] = j;
    }
    ms_present();
  }

  // D4 Stage B migration: a ported handler emits its typed LegalAction and
  // the exact option string it produced before, keeping options_ (the
  // transcript/puzzle/golden surface) byte-stable while legal_actions_ grows
  // handler by handler. When all handlers are ported, options_ becomes a
  // render of legal_actions_ and the string argument goes away.
  void push_action(LegalAction la, const std::string &option) {
    la.msg_ = msg_;
    options_.push_back(option);
    legal_actions_.push_back(std::move(la));
  }

  void handle_message() {
    int l_ = read_u32();
    dl_ = dp_ + l_;
    if (dl_ > fdl_) {
      throw std::runtime_error(
          fmt::format("message length {} overruns buffer {}", dl_, fdl_));
    }
    msg_ = int(data_[dp_++]);
    options_ = {};
    legal_actions_.clear();

    if (verbose_) {
      fmt::println("Message {}, full {}, length {}, dp {}", msg_to_string(msg_), fdl_, dl_, dp_);
      // print byte by byte
      for (int i = dp_; i < dl_; ++i) {
        fmt::print("{:02x} ", data_[i]);
      }
      fmt::print("\n");
    }

    if (msg_ == MSG_DRAW) {
      if (!verbose_) {
        dp_ = dl_;
        return;
      }
      auto player = read_u8();
      // TODO: different with ygopro-core
      auto drawed = compat_read<uint8_t, uint32_t>();
      std::vector<uint32_t> codes;
      for (int i = 0; i < drawed; ++i) {
        uint32_t code = read_u32();
        dp_ += 4;
        codes.push_back(code & 0x7fffffff);
      }
      const auto &pl = players_[player];
      pl->notify(fmt::format("Drew {} cards:", drawed));
      for (int i = 0; i < drawed; ++i) {
        const auto &c = c_get_card(codes[i]);
        pl->notify(fmt::format("{}: {}", i + 1, c.name_));
      }
      const auto &op = players_[1 - player];
      op->notify(fmt::format("Opponent drew {} cards.", drawed));
    } else if (msg_ == MSG_NEW_TURN) {
      tp_ = int(read_u8());
      turn_count_++;
      if (!verbose_) {
        return;
      }
      auto player = players_[tp_];
      player->notify("Your turn.");
      players_[1 - tp_]->notify(fmt::format("{}'s turn.", player->nickname_));
    } else if (msg_ == MSG_NEW_PHASE) {
      current_phase_ = int(read_u16());
      if (!verbose_) {
        return;
      }
      auto phase_str = phase_to_string(current_phase_);
      for (int i = 0; i < 2; ++i) {
        players_[i]->notify(fmt::format("Entering {} phase.", phase_str));
      }
    } else if (msg_ == MSG_MOVE) {
      if (!verbose_) {
        dp_ = dl_;
        return;
      }
      CardCode code = read_u32();
      loc_info location = read_loc_info();
      loc_info newloc = read_loc_info();
      uint32_t reason = read_u32();
      Card card = c_get_card(code);
      card.set_location(location);
      Card cnew = c_get_card(code);
      cnew.set_location(newloc);
      auto pl = players_[card.controler_];
      auto op = players_[1 - card.controler_];

      auto plspec = card.get_spec(false);
      auto opspec = card.get_spec(true);
      auto plnewspec = cnew.get_spec(false);
      auto opnewspec = cnew.get_spec(true);

      auto getspec = [&](Player *p) { return p == pl ? plspec : opspec; };
      auto getnewspec = [&](Player *p) {
        return p == pl ? plnewspec : opnewspec;
      };
      bool card_visible = true;
      if ((card.position_ & POS_FACEDOWN) && (cnew.position_ & POS_FACEDOWN)) {
        card_visible = false;
      }
      auto getvisiblename = [&](Player *p) {
        return card_visible ? card.name_ : "Face-down card";
      };

      if (card.location_ != cnew.location_) {
        if (reason & REASON_DESTROY) {
          pl->notify(fmt::format("Card {} ({}) destroyed.", plspec, card.name_));
          op->notify(fmt::format("Card {} ({}) destroyed.", opspec, card.name_));
        } else if (cnew.location_ == LOCATION_REMOVED) {
          pl->notify(
              fmt::format("Your card {} ({}) was banished.", plspec, card.name_));
          op->notify(fmt::format("{}'s card {} ({}) was banished.", pl->nickname_,
                                opspec, getvisiblename(op)));
          }
      } else if ((card.location_ == cnew.location_) &&
                 (card.location_ & LOCATION_ONFIELD)) {
        if (card.controler_ != cnew.controler_) {
          pl->notify(
              fmt::format("Your card {} ({}) changed controller to {} and is "
                          "now located at {}.",
                          plspec, card.name_, op->nickname_, plnewspec));
          op->notify(
              fmt::format("You now control {}'s card {} ({}) and it's located "
                          "at {}.",
                          pl->nickname_, opspec, card.name_, opnewspec));
        } else {
          pl->notify(fmt::format("Your card {} ({}) switched its zone to {}.",
                                 plspec, card.name_, plnewspec));
          op->notify(fmt::format("{}'s card {} ({}) switched its zone to {}.",
                                 pl->nickname_, opspec, card.name_, opnewspec));
        }
      } else if ((reason & REASON_DISCARD) &&
                 (card.location_ != cnew.location_)) {
        pl->notify(fmt::format("You discarded {} ({})", plspec, card.name_));
        op->notify(fmt::format("{} discarded {} ({})", pl->nickname_, opspec,
                               card.name_));
      } else if ((card.location_ == LOCATION_REMOVED) &&
                 (cnew.location_ & LOCATION_ONFIELD)) {
        pl->notify(
            fmt::format("Your banished card {} ({}) returns to the field at "
                        "{}.",
                        plspec, card.name_, plnewspec));
        op->notify(
            fmt::format("{}'s banished card {} ({}) returns to the field at "
                        "{}.",
                        pl->nickname_, opspec, card.name_, opnewspec));
      } else if ((card.location_ == LOCATION_GRAVE) &&
                 (cnew.location_ & LOCATION_ONFIELD)) {
        pl->notify(
            fmt::format("Your card {} ({}) returns from the graveyard to the "
                        "field at {}.",
                        plspec, card.name_, plnewspec));
        op->notify(
            fmt::format("{}'s card {} ({}) returns from the graveyard to the "
                        "field at {}.",
                        pl->nickname_, opspec, card.name_, opnewspec));
      } else if ((cnew.location_ == LOCATION_HAND) &&
                 (card.location_ != cnew.location_)) {
        pl->notify(
            fmt::format("Card {} ({}) returned to hand.", plspec, card.name_));
      } else if ((reason & (REASON_RELEASE | REASON_SUMMON)) &&
                 (card.location_ != cnew.location_)) {
        pl->notify(fmt::format("You tribute {} ({}).", plspec, card.name_));
        op->notify(fmt::format("{} tributes {} ({}).", pl->nickname_, opspec,
                               getvisiblename(op)));
      } else if ((card.location_ == (LOCATION_OVERLAY | LOCATION_MZONE)) &&
                 (cnew.location_ & LOCATION_GRAVE)) {
        pl->notify(fmt::format("You detached {}.", card.name_));
        op->notify(fmt::format("{} detached {}.", pl->nickname_, card.name_));
      } else if ((card.location_ != cnew.location_) &&
                 (cnew.location_ == LOCATION_GRAVE)) {
        pl->notify(fmt::format("Your card {} ({}) was sent to the graveyard.",
                               plspec, card.name_));
        op->notify(fmt::format("{}'s card {} ({}) was sent to the graveyard.",
                               pl->nickname_, opspec, card.name_));
      } else if ((card.location_ != cnew.location_) &&
                 (cnew.location_ == LOCATION_DECK)) {
        pl->notify(fmt::format("Your card {} ({}) returned to your deck.",
                               plspec, card.name_));
        op->notify(fmt::format("{}'s card {} ({}) returned to their deck.",
                               pl->nickname_, opspec, getvisiblename(op)));
      } else if ((card.location_ != cnew.location_) &&
                 (cnew.location_ == LOCATION_EXTRA)) {
        pl->notify(fmt::format("Your card {} ({}) returned to your extra deck.",
                               plspec, card.name_));
        op->notify(
            fmt::format("{}'s card {} ({}) returned to their extra deck.",
                        pl->nickname_, opspec, getvisiblename(op)));
      } else if ((card.location_ == LOCATION_DECK) &&
                 (cnew.location_ == LOCATION_SZONE) &&
                 (cnew.position_ != POS_FACEDOWN)) {
        pl->notify(fmt::format("Activating {} ({})", plnewspec, card.name_));
        op->notify(fmt::format("{} activating {} ({})", pl->nickname_, opspec,
                               cnew.name_));
      } else {
        fmt::println("Unknown move reason {}", reason);
      }
    } else if (msg_ == MSG_SWAP) {
      if (!verbose_) {
        dp_ = dl_;
        return;
      }
      CardCode code1 = read_u32();
      auto loc1 = read_loc_info();
      CardCode code2 = read_u32();
      auto loc2 = read_loc_info();
      Card cards[2];
      cards[0] = c_get_card(code1);
      cards[1] = c_get_card(code2);
      cards[0].set_location(loc1);
      cards[1].set_location(loc2);

      for (PlayerId pl = 0; pl < 2; pl++) {
        for (int i = 0; i < 2; i++) {
          auto c = cards[i];
          auto spec = c.get_spec(pl);
          auto plname = players_[1 - c.controler_]->nickname_;
          players_[pl]->notify("Card " + c.name_ + " swapped control towards " +
                               plname + " and is now located at " + spec + ".");
        }
      }
    } else if (msg_ == MSG_SET) {
      if (!verbose_) {
        dp_ = dl_;
        return;
      }
      CardCode code = read_u32();
      Card card = c_get_card(code);
      card.set_location(read_loc_info());
      auto c = card.controler_;
      auto cpl = players_[c];
      auto opl = players_[1 - c];
      cpl->notify(fmt::format("You set {} ({}) in {} position.", card.name_,
                              card.get_spec(c), card.get_position()));
      opl->notify(fmt::format("{} sets {} in {} position.", cpl->nickname_,
                              card.get_spec(PlayerId(1 - c)),
                              card.get_position()));
    } else if (msg_ == MSG_EQUIP) {
      if (!verbose_) {
        dp_ = dl_;
        return;
      }
      auto info = read_loc_info();
      auto c = info.controler;
      auto loc = info.location;
      auto seq = info.sequence;
      auto pos = info.position;
      Card card = get_card(c, loc, seq);
      info = read_loc_info();
      c = info.controler;
      loc = info.location;
      seq = info.sequence;
      pos = info.position;
      Card target = get_card(c, loc, seq);
      for (PlayerId pl = 0; pl < 2; pl++) {
        auto c = cardlist_info_for_player(card, pl);
        auto t = cardlist_info_for_player(target, pl);
        players_[pl]->notify(fmt::format("{} equipped to {}.", c, t));
      }
    } else if (msg_ == MSG_PLAYER_HINT) {
      if (!verbose_) {
        dp_ = dl_;
        return;
      }
      auto player = read_u8();
      auto hint_type = read_u8();
      auto value = compat_read<uint32_t, uint64_t>();
      // TODO: handle this
      return;
    } else if (msg_ == MSG_HINT) {
      auto hint_type = read_u8();
      auto player = read_u8();
      auto value = compat_read<uint32_t, uint64_t>();

      if (hint_type == HINT_SELECTMSG && value == 501) {
        discard_hand_ = true;
      }
      // non-GUI don't need hint
      return;
      if (hint_type == HINT_SELECTMSG) {
        if (value > 2000) {
          CardCode code = value;
          players_[player]->notify(fmt::format("{} select {}",
                                               players_[player]->nickname_,
                                               c_get_card(code).name_));
        } else {
          players_[player]->notify(get_system_string(value));
        }
      } else if (hint_type == HINT_NUMBER) {
        players_[1 - player]->notify(
            fmt::format("Choice of player: {}", value));
      } else {
        fmt::println("Unknown hint type {} with value {}", hint_type, value);
      }
    } else if (msg_ == MSG_CARD_HINT) {
      if (!verbose_) {
        dp_ = dl_;
        return;
      }
      auto info = read_loc_info();
      uint8_t player = info.controler;
      uint8_t loc = info.location;
      uint8_t seq = info.sequence;
      uint8_t pos = info.position;
      uint8_t type = read_u8();
      uint32_t value = compat_read<uint32_t, uint64_t>();
      Card card = get_card(player, loc, seq);
      if (card.code_ == 0) {
        return;
      }
      if (type == CHINT_RACE) {
        std::string races_str = "TODO";
        for (PlayerId pl = 0; pl < 2; pl++) {
          players_[pl]->notify(fmt::format("{} ({}) selected {}.",
                                           card.get_spec(pl), card.name_,
                                           races_str));
        }
      } else if (type == CHINT_ATTRIBUTE) {
        std::string attributes_str = "TODO";
        for (PlayerId pl = 0; pl < 2; pl++) {
          players_[pl]->notify(fmt::format("{} ({}) selected {}.",
                                           card.get_spec(pl), card.name_,
                                           attributes_str));
        }
      } else {
        fmt::println("Unknown card hint type {} with value {}", type, value);
      }
    } else if (msg_ == MSG_POS_CHANGE) {
      if (!verbose_) {
        dp_ = dl_;
        return;
      }
      CardCode code = read_u32();
      Card card = c_get_card(code);
      card.set_location(read_u32());
      uint8_t prevpos = card.position_;
      card.position_ = read_u8();

      auto pl = players_[card.controler_];
      auto op = players_[1 - card.controler_];
      auto plspec = card.get_spec(false);
      auto opspec = card.get_spec(true);
      auto prevpos_str = position_to_string(prevpos);
      auto pos_str = position_to_string(card.position_);
      pl->notify("The position of card " + plspec + " (" + card.name_ +
                 ") changed from " + prevpos_str + " to " + pos_str + ".");
      op->notify("The position of card " + opspec + " (" + card.name_ +
                 ") changed from " + prevpos_str + " to " + pos_str + ".");
    } else if (msg_ == MSG_BECOME_TARGET || msg_ == MSG_CARD_SELECTED) {
      if (!verbose_) {
        dp_ = dl_;
        return;
      }
      auto count = compat_read<uint8_t, uint32_t>();
      std::vector<Card> cards;
      cards.reserve(count);
      for (int i = 0; i < count; ++i) {
        auto info = read_loc_info();
        auto c = info.controler;
        auto loc = info.location;
        auto seq = info.sequence;
        cards.push_back(get_card(c, loc, seq));
      }
      auto name = players_[chaining_player_]->nickname_;
      for (PlayerId pl = 0; pl < 2; pl++) {
        std::string str = name;
        if (msg_ == MSG_BECOME_TARGET) {
          str += " targets ";
        } else {
          str += " selects ";
        }
        for (int i = 0; i < count; ++i) {
          auto card = cards[i];
          auto spec = card.get_spec(pl);
          auto tcname = card.name_;
          if ((card.controler_ != pl) && (card.position_ & POS_FACEDOWN)) {
            tcname = position_to_string(card.position_) + " card";
          }
          str += spec + " (" + tcname + ")";
          if (i < count - 1) {
            str += ", ";
          }
        }
        players_[pl]->notify(str);
      }
    } else if (msg_ == MSG_CONFIRM_DECKTOP) {
      if (!verbose_) {
        dp_ = dl_;
        return;
      }
      auto player = read_u8();
      auto size = compat_read<uint8_t, uint32_t>();
      std::vector<Card> cards;
      for (int i = 0; i < size; ++i) {
        read_u32();
        auto c = read_u8();
        auto loc = read_u8();
        auto seq = compat_read<uint8_t, uint32_t>();
        cards.push_back(get_card(c, loc, seq));
      }

      for (PlayerId pl = 0; pl < 2; pl++) {
        auto p = players_[pl];
        if (pl == player) {
          p->notify(fmt::format("You reveal {} cards from your deck:", size));
        } else {
          p->notify(fmt::format("{} reveals {} cards from their deck:",
                                players_[player]->nickname_, size));
        }
        for (int i = 0; i < size; ++i) {
          p->notify(fmt::format("{}: {}", i + 1, cards[i].name_));
        }
      }
    } else if (msg_ == MSG_RANDOM_SELECTED) {
      if (!verbose_) {
        dp_ = dl_;
        return;
      }
      auto player = read_u8();
      auto count = compat_read<uint8_t, uint32_t>();
      std::vector<Card> cards;

      for (int i = 0; i < count; ++i) {
        auto info = read_loc_info();
        auto c = info.controler;
        auto loc = info.location;
        if (loc & LOCATION_OVERLAY) {
          throw std::runtime_error("Overlay not supported for random selected");
        }
        auto seq = info.sequence;
        auto pos = info.position;
        cards.push_back(get_card(c, loc, seq));
      }

      for (PlayerId pl = 0; pl < 2; pl++) {
        auto p = players_[pl];
        auto s = "card is";
        if (count > 1) {
          s = "cards are";
        }
        if (pl == player) {
          p->notify(fmt::format("Your {} {} randomly selected:", s, count));
        } else {
          p->notify(fmt::format("{}'s {} {} randomly selected:",
                                players_[player]->nickname_, s, count));
        }
        for (int i = 0; i < count; ++i) {
          p->notify(fmt::format("{}: {}", cards[i].get_spec(pl), cards[i].name_));
        }
      }

    } else if (msg_ == MSG_CONFIRM_CARDS) {
      auto player = read_u8();
      auto size = compat_read<uint8_t, uint32_t>();
      std::vector<Card> cards;
      for (int i = 0; i < size; ++i) {
        read_u32();
        auto c = read_u8();
        auto loc = read_u8();
        auto seq = compat_read<uint8_t, uint32_t>();
        if (verbose_) {
          cards.push_back(get_card(c, loc, seq));
        }
        revealed_.push_back(ls_to_spec(loc, seq, 0, c == player));
      }
      if (!verbose_) {
        return;
      }

      auto pl = players_[player];
      auto op = players_[1 - player];

      op->notify(fmt::format("{} shows you {} cards.", pl->nickname_, size));
      for (int i = 0; i < size; ++i) {
        pl->notify(fmt::format("{}: {}", i + 1, cards[i].name_));
      }
    } else if (msg_ == MSG_MISSED_EFFECT) {
      if (!verbose_) {
        dp_ = dl_;
        return;
      }
      dp_ += 4;
      CardCode code = read_u32();
      Card card = c_get_card(code);
      for (PlayerId pl = 0; pl < 2; pl++) {
        auto spec = card.get_spec(pl);
        auto str = get_system_string(1622);
        std::string fmt_str = "[%ls]";
        str = str.replace(str.find(fmt_str), fmt_str.length(), card.name_);
        players_[pl]->notify(str);
      }
    } else if (msg_ == MSG_SORT_CARD) {
      // TODO: implement action
      if (!verbose_) {
        dp_ = dl_;
        YGO_SetResponsei(pduel_, -1);
        return;
      }
      auto player = read_u8();
      auto size = compat_read<uint8_t, uint32_t>();
      std::vector<Card> cards;
      for (int i = 0; i < size; ++i) {
        read_u32();
        auto c = read_u8();
        auto loc = compat_read<uint8_t, uint32_t>();
        auto seq = compat_read<uint8_t, uint32_t>();
        cards.push_back(get_card(c, loc, seq));
      }
      auto pl = players_[player];
      pl->notify(
          "Sort " + std::to_string(size) +
          " cards by entering numbers separated by spaces (c = cancel):");
      for (int i = 0; i < size; ++i) {
        pl->notify(fmt::format("{}: {}", i + 1, cards[i].name_));
      }

      fmt::println("sort card action not implemented");
      YGO_SetResponsei(pduel_, -1);

      // // generate all permutations
      // std::vector<int> perm(size);
      // std::iota(perm.begin(), perm.end(), 0);
      // std::vector<std::vector<int>> perms;
      // do {
      //   auto option = std::accumulate(perm.begin(), perm.end(),
      //   std::string(),
      //                                 [&](std::string &acc, int i) {
      //                                   return acc + std::to_string(i + 1) +
      //                                   " ";
      //                                 });
      //   options_.push_back(option);
      // } while (std::next_permutation(perm.begin(), perm.end()));
      // options_.push_back("c");
      // callback_ = [this](int idx) {
      //   const auto &option = options_[idx];
      //   if (option == "c") {
      //     resp_buf_[0] = 255;
      //     YGO_SetResponseb(pduel_, resp_buf_);
      //     return;
      //   }
      //   std::istringstream iss(option);
      //   int x;
      //   int i = 0;
      //   while (iss >> x) {
      //     resp_buf_[i] = uint8_t(x);
      //     i++;
      //   }
      //   YGO_SetResponseb(pduel_, resp_buf_);
      // };
    } else if (msg_ == MSG_ADD_COUNTER) {
      if (!verbose_) {
        dp_ = dl_;
        return;
      }
      auto ctype = read_u16();
      auto player = read_u8();
      auto loc = read_u8();
      auto seq = read_u8();
      auto count = read_u16();
      auto c = get_card(player, loc, seq);
      auto pl = players_[player];
      PlayerId op_id = 1 - player;
      auto op = players_[op_id];
      // TODO: counter type to string
      pl->notify(fmt::format("{} counter(s) of type {} placed on {} ().", count, "UNK", c.name_, c.get_spec(player)));
      op->notify(fmt::format("{} counter(s) of type {} placed on {} ().", count, "UNK", c.name_, c.get_spec(op_id)));
    } else if (msg_ == MSG_REMOVE_COUNTER) {
      if (!verbose_) {
        dp_ = dl_;
        return;
      }
      auto ctype = read_u16();
      auto player = read_u8();
      auto loc = read_u8();
      auto seq = read_u8();
      auto count = read_u16();
      auto c = get_card(player, loc, seq);
      auto pl = players_[player];
      PlayerId op_id = 1 - player;
      auto op = players_[op_id];
      pl->notify(fmt::format("{} counter(s) of type {} removed from {} ().", count, "UNK", c.name_, c.get_spec(player)));
      op->notify(fmt::format("{} counter(s) of type {} removed from {} ().", count, "UNK", c.name_, c.get_spec(op_id)));
    } else if (msg_ == MSG_ATTACK_DISABLED) {
      if (!verbose_) {
        dp_ = dl_;
        return;
      }
      for (PlayerId pl = 0; pl < 2; pl++) {
        players_[pl]->notify(get_system_string(1621));
      }
    } else if (msg_ == MSG_SHUFFLE_SET_CARD) {
      if (!verbose_) {
        dp_ = dl_;
        return;
      }
      // TODO: implement output
      dp_ = dl_;
    } else if (msg_ == MSG_SHUFFLE_DECK) {
      if (!verbose_) {
        dp_ = dl_;
        return;
      }
      auto player = read_u8();
      auto pl = players_[player];
      auto op = players_[1 - player];
      pl->notify("You shuffled your deck.");
      op->notify(pl->nickname_ + " shuffled their deck.");
    } else if (msg_ == MSG_RELOAD_FIELD) {
      // Full field resync: emitted by Debug.ReloadFieldEnd() (puzzle setup)
      // and by the server on reconnection. The observation is rebuilt from
      // OCG_DuelQuery* rather than tracked from messages, so the payload can
      // be skipped — but it must be consumed, not treated as unknown.
      dp_ = dl_;
      if (verbose_) {
        players_[0]->notify("Field reloaded.");
        players_[1]->notify("Field reloaded.");
      }
      return;
    } else if (msg_ == MSG_SHUFFLE_EXTRA) {
      if (!verbose_) {
        dp_ = dl_;
        return;
      }
      auto player = read_u8();
      auto count = compat_read<uint8_t, uint32_t>();
      for (int i = 0; i < count; ++i) {
        read_u32();
      }
      auto pl = players_[player];
      auto op = players_[1 - player];
      pl->notify(fmt::format("You shuffled your extra deck ({}).", count));
      op->notify(fmt::format("{} shuffled their extra deck ({}).", pl->nickname_, count));
    } else if (msg_ == MSG_SHUFFLE_HAND) {
      if (!verbose_) {
        dp_ = dl_;
        return;
      }

      auto player = read_u8();
      dp_ = dl_;

      auto pl = players_[player];
      auto op = players_[1 - player];
      pl->notify("You shuffled your hand.");
      op->notify(pl->nickname_ + " shuffled their hand.");
    } else if (msg_ == MSG_SUMMONED) {
      dp_ = dl_;
    } else if (msg_ == MSG_SUMMONING) {
      if (!verbose_) {
        dp_ = dl_;
        return;
      }
      CardCode code = read_u32();
      Card card = c_get_card(code);
      card.set_location(read_loc_info());
      const auto &nickname = players_[card.controler_]->nickname_;
      for (auto pl : players_) {
        pl->notify(nickname + " summoning " + card.name_ + " (" +
                   std::to_string(card.attack_) + "/" +
                   std::to_string(card.defense_) + ") in " +
                   card.get_position() + " position.");
      }
    } else if (msg_ == MSG_SPSUMMONED) {
      dp_ = dl_;
    } else if (msg_ == MSG_FLIPSUMMONED) {
      dp_ = dl_;
    } else if (msg_ == MSG_FLIPSUMMONING) {
      if (!verbose_) {
        dp_ = dl_;
        return;
      }

      auto code = read_u32();
      auto loc_info = read_loc_info();
      Card card = c_get_card(code);
      card.set_location(loc_info);

      auto cpl = players_[card.controler_];
      for (PlayerId pl = 0; pl < 2; pl++) {
        auto spec = card.get_spec(pl);
        players_[1 - pl]->notify(cpl->nickname_ + " flip summons " + spec +
                                 " (" + card.name_ + ")");
      }
    } else if (msg_ == MSG_SPSUMMONING) {
      if (!verbose_) {
        dp_ = dl_;
        return;
      }
      CardCode code = read_u32();
      Card card = c_get_card(code);
      card.set_location(read_loc_info());
      const auto &nickname = players_[card.controler_]->nickname_;
      for (PlayerId p = 0; p < 2; p++) {
        auto pl = players_[p];
        auto pos = card.get_position();
        auto atk = std::to_string(card.attack_);
        auto def = std::to_string(card.defense_);
        std::string name = p == card.controler_ ? "You" : nickname;
        if (card.type_ & TYPE_LINK) {
          pl->notify(name + " special summoning " + card.name_ + " (" +
                     atk + ") in " + pos + " position.");
        } else {
          pl->notify(name + " special summoning " + card.name_ + " (" +
                     atk + "/" + def + ") in " + pos + " position.");
        }
      }
    } else if (msg_ == MSG_CHAIN_NEGATED) {
      dp_ = dl_;
    } else if (msg_ == MSG_CHAIN_DISABLED) {
      dp_ = dl_;
    } else if (msg_ == MSG_CHAIN_SOLVED) {
      dp_ = dl_;
      revealed_.clear();
    } else if (msg_ == MSG_CHAIN_SOLVING) {
      dp_ = dl_;
    } else if (msg_ == MSG_CHAINED) {
      dp_ = dl_;
    } else if (msg_ == MSG_CHAIN_END) {
      dp_ = dl_;
    } else if (msg_ == MSG_CHAINING) {
      // Loop detection (Tournament Policy v2.5 "Infinite Loop"): an
      // involuntary loop is one the player cannot stop, i.e. it presents no
      // decisions. Count chain activations since the last decision; the most
      // recent activation is the policy's "primary cause" candidate.
      {
        CardCode chained = *reinterpret_cast<uint32_t *>(data_ + dp_);
        ++chains_since_decision_;
        loop_cause_code_ = chained;
      }
      if (!verbose_) {
        dp_ = dl_;
        return;
      }
      CardCode code = read_u32();
      Card card = c_get_card(code);
      card.set_location(read_loc_info());
      auto tc = read_u8();
      auto tl = read_u8();
      auto ts = compat_read<uint8_t, uint32_t>();
      uint32_t desc = compat_read<uint32_t, uint64_t>();
      auto cs = compat_read<uint8_t, uint32_t>();
      auto c = card.controler_;
      PlayerId o = 1 - c;
      chaining_player_ = c;
      players_[c]->notify("Activating " + card.get_spec(c) + " (" + card.name_ +
                          ")");
      players_[o]->notify(players_[c]->nickname_ + " activating " +
                          card.get_spec(o) + " (" + card.name_ + ")");
    } else if (msg_ == MSG_DAMAGE) {
      auto player = read_u8();
      auto amount = read_u32();
      _damage(player, amount);
    } else if (msg_ == MSG_RECOVER) {
      auto player = read_u8();
      auto amount = read_u32();
      _recover(player, amount);
    } else if (msg_ == MSG_LPUPDATE) {
      auto player = read_u8();
      auto lp = read_u32();
      if (lp >= lp_[player]) {
        _recover(player, lp - lp_[player]);
      } else {
        _damage(player, lp_[player] - lp);
      }
    } else if (msg_ == MSG_PAY_LPCOST) {
      auto player = read_u8();
      auto cost = read_u32();
      lp_[player] -= cost;
      if (!verbose_) {
        return;
      }
      auto pl = players_[player];
      pl->notify("You pay " + std::to_string(cost) + " LP. Your LP is now " +
                 std::to_string(lp_[player]) + ".");
      players_[1 - player]->notify(
          pl->nickname_ + " pays " + std::to_string(cost) + " LP. " +
          pl->nickname_ + "'s LP is now " + std::to_string(lp_[player]) + ".");
    } else if (msg_ == MSG_ATTACK) {
      if (!verbose_) {
        dp_ = dl_;
        return;
      }
      auto attacker = read_loc_info();
      PlayerId ac = attacker.controler;
      auto aloc = attacker.location;
      auto aseq = attacker.sequence;
      auto apos = attacker.position;
      auto target = read_loc_info();
      PlayerId tc = target.controler;
      auto tloc = target.location;
      auto tseq = target.sequence;
      auto tpos = target.position;

      if ((ac == 0) && (aloc == 0) && (aseq == 0) && (apos == 0)) {
        return;
      }

      Card acard = get_card(ac, aloc, aseq);
      auto name = players_[ac]->nickname_;
      if ((tc == 0) && (tloc == 0) && (tseq == 0) && (tpos == 0)) {
        for (PlayerId i = 0; i < 2; i++) {
          players_[i]->notify(name + " prepares to attack with " +
                              acard.get_spec(i) + " (" + acard.name_ + ")");
        }
        return;
      }

      Card tcard = get_card(tc, tloc, tseq);
      for (PlayerId i = 0; i < 2; i++) {
        auto aspec = acard.get_spec(i);
        auto tspec = tcard.get_spec(i);
        auto tcname = tcard.name_;
        if ((tcard.controler_ != i) && (tcard.position_ & POS_FACEDOWN)) {
          tcname = tcard.get_position() + " card";
        }
        players_[i]->notify(name + " prepares to attack " + tspec + " (" +
                            tcname + ") with " + aspec + " (" + acard.name_ +
                            ")");
      }
    } else if (msg_ == MSG_DAMAGE_STEP_START) {
      if (!verbose_) {
        return;
      }
      for (int i = 0; i < 2; i++) {
        players_[i]->notify("begin damage");
      }
    } else if (msg_ == MSG_DAMAGE_STEP_END) {
      if (!verbose_) {
        return;
      }
      for (int i = 0; i < 2; i++) {
        players_[i]->notify("end damage");
      }
    } else if (msg_ == MSG_BATTLE) {
      if (!verbose_) {
        dp_ = dl_;
        return;
      }
      auto attacker = read_loc_info();
      auto aa = read_u32();
      auto ad = read_u32();
      auto bd0 = read_u8();
      auto target = read_loc_info();
      auto da = read_u32();
      auto dd = read_u32();
      auto bd1 = read_u8();

      auto ac = attacker.controler;
      auto aloc = attacker.location;
      auto aseq = attacker.sequence;

      auto tc = target.controler;
      auto tloc = target.location;
      auto tseq = target.sequence;
      auto tpos = target.position;

      Card acard = get_card(ac, aloc, aseq);
      Card tcard;
      if (tloc != 0) {
        tcard = get_card(tc, tloc, tseq);
      }
      for (int i = 0; i < 2; i++) {
        auto pl = players_[i];
        std::string attacker_points;
        if (acard.type_ & TYPE_LINK) {
          attacker_points = std::to_string(aa);
        } else {
          attacker_points = std::to_string(aa) + "/" + std::to_string(ad);
        }
        if (tloc != 0) {
          std::string defender_points;
          if (tcard.type_ & TYPE_LINK) {
            defender_points = std::to_string(da);
          } else {
            defender_points = std::to_string(da) + "/" + std::to_string(dd);
          }
          pl->notify(acard.name_ + "(" + attacker_points + ")" + " attacks " +
                     tcard.name_ + " (" + defender_points + ")");
        } else {
          pl->notify(acard.name_ + "(" + attacker_points + ")" + " attacks");
        }
      }
    } else if (msg_ == MSG_WIN) {
      auto player = read_u8();
      auto reason = read_u8();
      auto winner = players_[player];
      auto loser = players_[1 - player];

      _duel_end(player, reason);

      auto l_reason = reason_to_string(reason);
      if (verbose_) {
        winner->notify("You won (" + l_reason + ").");
        loser->notify("You lost (" + l_reason + ").");
      }
    } else if (msg_ == MSG_RETRY) {
      // After a script-budget trip the duel state can be inconsistent and
      // reject all responses; treat retry as unrecoverable and truncate the
      // episode as a draw. YGOENV_STRICT=1 restores the hard error for
      // debugging response-format bugs.
      if (std::getenv("YGOENV_STRICT")) {
        throw std::runtime_error(fmt::format(
            "Retry (prev msg {}, chosen option '{}')", prev_msg_for_retry_,
            prev_option_for_retry_));
      }
      report_loop_incident("msg_retry");
      done_ = true;
      winner_ = 255;
      duel_started_ = false;
      dp_ = 0;
      fdl_ = 0;
      return;
    } else if (msg_ == MSG_SELECT_BATTLECMD) {
      auto player = read_u8();
      auto activatable = read_cardlist_spec(true, true);
      auto attackable = read_cardlist_spec(false, false, true);
      bool to_m2 = read_u8();
      bool to_ep = read_u8();

      auto pl = players_[player];
      if (verbose_) {
        pl->notify("Battle menu:");
      }
      for (const auto [code, spec, data] : activatable) {
        auto [code_d, eff_idx] = unpack_desc(data);
        auto la = LegalAction::activate_spec(eff_idx, spec);
        la.cid_ = c_get_card_id_or_zero(code_d != 0 ? code_d : code);
        push_action(la, "v " + spec);
        if (verbose_) {
          auto [loc, seq, pos] = spec_to_ls(spec);
          auto c = get_card(player, loc, seq);
          pl->notify("v " + spec + ": activate " + c.name_ + " (" +
                     std::to_string(c.attack_) + "/" +
                     std::to_string(c.defense_) + ")");
        }
      }
      for (const auto [code, spec, data] : attackable) {
        auto la = LegalAction::act_spec(ActionAct::Attack, spec);
        la.cid_ = c_get_card_id_or_zero(code);
        push_action(la, "a " + spec);
        if (verbose_) {
          auto [loc, seq, pos] = spec_to_ls(spec);
          auto c = get_card(player, loc, seq);
          if (c.type_ & TYPE_LINK) {
            pl->notify("a " + spec + ": " + c.name_ + " (" +
                       std::to_string(c.attack_) + ") attack");
          } else {
            pl->notify("a " + spec + ": " + c.name_ + " (" +
                       std::to_string(c.attack_) + "/" +
                       std::to_string(c.defense_) + ") attack");
          }
        }
      }
      if (to_m2) {
        push_action(LegalAction::phase(ActionPhase::Main2), "m");
        if (verbose_) {
          pl->notify("m: Main phase 2.");
        }
      }
      if (to_ep) {
        if (!to_m2) {
          push_action(LegalAction::phase(ActionPhase::End), "e");
          if (verbose_) {
            pl->notify("e: End phase.");
          }
        }
      }
      int n_activatables = activatable.size();
      int n_attackables = attackable.size();
      to_play_ = player;
      callback_ = [this, n_activatables, n_attackables, to_ep, to_m2](int idx) {
        if (idx < n_activatables) {
          YGO_SetResponsei(pduel_, idx << 16);
        } else if (idx < (n_activatables + n_attackables)) {
          idx = idx - n_activatables;
          YGO_SetResponsei(pduel_, (idx << 16) + 1);
        } else if ((options_[idx] == "e") && to_ep) {
          YGO_SetResponsei(pduel_, 3);
        } else if ((options_[idx] == "m") && to_m2) {
          YGO_SetResponsei(pduel_, 2);
        } else {
          throw std::runtime_error("Invalid option");
        }
      };
    } else if (msg_ == MSG_SELECT_UNSELECT_CARD) {
      auto player = read_u8();
      bool finishable = read_u8();
      bool cancelable = read_u8();
      auto min = compat_read<uint8_t, uint32_t>();
      auto max = compat_read<uint8_t, uint32_t>();
      auto select_size = compat_read<uint8_t, uint32_t>();

      std::vector<std::string> select_specs;
      select_specs.reserve(select_size);
      if (verbose_) {
        std::vector<Card> cards;
        for (int i = 0; i < select_size; ++i) {
          auto code = read_u32();
          auto loc_info = read_loc_info();
          Card card = c_get_card(code);
          card.set_location(loc_info);
          cards.push_back(card);
        }
        auto pl = players_[player];
        pl->notify("Select " + std::to_string(min) + " to " +
                   std::to_string(max) + " cards:");
        for (const auto &card : cards) {
          auto spec = card.get_spec(player);
          select_specs.push_back(spec);
          pl->notify(spec + ": " + card.name_);
        }
      } else {
        for (int i = 0; i < select_size; ++i) {
          dp_ += 4;
          auto loc_info = read_loc_info();
          auto spec = ls_to_spec(loc_info, player);
          select_specs.push_back(spec);
        }
      }

      auto unselect_size = compat_read<uint8_t, uint32_t>();

      // unselect not allowed (no regrets!)
      if (compat_mode_) {
        dp_ += 8 * unselect_size;
      } else {
        dp_ += 14 * unselect_size;
      }

      for (int j = 0; j < select_specs.size(); ++j) {
        push_action(LegalAction::from_spec(select_specs[j]), select_specs[j]);
      }

      if (finishable) {
        push_action(LegalAction::finish(), "f");
      }

      // cancelable and finishable not needed

      to_play_ = player;
      if (compat_mode_) {
        callback_ = [this](int idx) {
          if (options_[idx] == "f") {
            YGO_SetResponsei(pduel_, -1);
          } else {
            resp_buf_[0] = 1;
            resp_buf_[1] = idx;
            YGO_SetResponseb(pduel_, resp_buf_);
          }
        };
      } else {
        callback_ = [this](int idx) {
          if (options_[idx] == "f") {
            YGO_SetResponsei(pduel_, -1);
          } else {
            uint32_t ret = 1;
            memcpy(resp_buf_, &ret, sizeof(ret));
            uint32_t v = idx;
            memcpy(resp_buf_ + 4, &v, sizeof(v));
            YGO_SetResponseb(pduel_, resp_buf_, 8);
          }
        };        
      }
    } else if (msg_ == MSG_SELECT_CARD) {
      auto player = read_u8();
      bool cancelable = read_u8();
      auto min = compat_read<uint8_t, uint32_t>();
      auto max = compat_read<uint8_t, uint32_t>();
      auto size = compat_read<uint8_t, uint32_t>();

      std::vector<std::string> specs;
      specs.reserve(size);
      if (verbose_) {
        std::vector<Card> cards;
        for (int i = 0; i < size; ++i) {
          auto code = read_u32();
          Card card = c_get_card(code);
          card.set_location(read_loc_info());
          cards.push_back(card);
        }
        auto pl = players_[player];
        pl->notify("Select " + std::to_string(min) + " to " +
                   std::to_string(max) + " cards separated by spaces:");
        for (const auto &card : cards) {
          auto spec = card.get_spec(player);
          specs.push_back(spec);
          if (card.controler_ != player && card.position_ & POS_FACEDOWN) {
            pl->notify(spec + ": " + card.get_position() + " card");
          } else {
            pl->notify(spec + ": " + card.name_);
          }
        }
      } else {
        for (int i = 0; i < size; ++i) {
          dp_ += 4;
          loc_info info = read_loc_info();
          auto spec = ls_to_spec(info, player);
          specs.push_back(spec);
        }
      }

      // Iterative multi-select (D4 Stage B): the agent picks one card per
      // env step and finishes at >= min, instead of choosing among
      // enumerated combinations. This also retires the random fallback for
      // min > max_multi_select, which silently hid legal choices.
      discard_hand_ = false;
      bool compat = compat_mode_;
      auto send = [this, compat](const std::vector<int> &comb) {
        if (compat) {
          resp_buf_[0] = comb.size();
          for (int i = 0; i < comb.size(); ++i) {
            resp_buf_[i + 1] = comb[i];
          }
          YGO_SetResponseb(pduel_, resp_buf_);
          return;
        }
        uint32_t maxseq = 0;
        uint32_t size = comb.size();
        for (auto &c : comb) {
          maxseq = std::max(maxseq, static_cast<uint32_t>(c));
        }
        auto ret = GetSuitableReturn(maxseq, size);
        memcpy(resp_buf_, &ret, sizeof(ret));
        if (ret == 3) {
          // bitmap over all selectable indices: ceil((maxseq+1)/8) bytes
          uint32_t nbytes = maxseq / 8 + 1;
          memset(resp_buf_ + 4, 0, nbytes);
          for (int i = 0; i < comb.size(); ++i) {
            resp_buf_[4 + comb[i] / 8] |= (1 << (comb[i] % 8));
          }
          YGO_SetResponseb(pduel_, resp_buf_, 4 + nbytes);
        } else if (ret == 2) {
          memcpy(resp_buf_ + 4, &size, sizeof(size));
          for (int i = 0; i < size; ++i) {
            uint8_t v = comb[i];
            memcpy(resp_buf_ + 8 + i, &v, sizeof(v));
          }
          YGO_SetResponseb(pduel_, resp_buf_, 8 + size);
        } else {
          auto err = fmt::format("Invalid return value: {}", ret);
          throw std::runtime_error(err);
        }
      };
      to_play_ = player;
      init_multi_select(min, max, specs, 0, {}, send);
    } else if (msg_ == MSG_SELECT_TRIBUTE) {
      auto player = read_u8();
      bool cancelable = read_u8();
      auto min = compat_read<uint8_t, uint32_t>();
      auto max = compat_read<uint8_t, uint32_t>();
      auto size = compat_read<uint8_t, uint32_t>();

      if (max > 3) {
        throw std::runtime_error("Max > 3 not implemented for select tribute");
      }

      std::vector<int> release_params;
      release_params.reserve(size);
      std::vector<std::string> specs;
      specs.reserve(size);
      if (verbose_) {
        std::vector<Card> cards;
        for (int i = 0; i < size; ++i) {
          auto code = read_u32();
          auto controller = read_u8();
          auto loc = read_u8();
          auto seq = compat_read<uint8_t, uint32_t>();
          auto release_param = read_u8();
          Card card = get_card(controller, loc, seq);
          cards.push_back(card);
          release_params.push_back(release_param);
        }
        auto pl = players_[player];
        pl->notify("Select " + std::to_string(min) + " to " +
                   std::to_string(max) +
                   " cards to tribute separated by spaces:");
        for (const auto &card : cards) {
          auto spec = card.get_spec(player);
          specs.push_back(spec);
          pl->notify(spec + ": " + card.name_);
        }
      } else {
        for (int i = 0; i < size; ++i) {
          dp_ += 4;
          auto controller = read_u8();
          auto loc = read_u8();
          auto seq = compat_read<uint8_t, uint32_t>();
          auto release_param = read_u8();

          auto spec = ls_to_spec(loc, seq, 0, controller != player);
          specs.push_back(spec);

          release_params.push_back(release_param);
        }
      }

      bool has_weight =
          std::any_of(release_params.begin(), release_params.end(),
                      [](int i) { return i != 1; });

      if (min != max) {
        throw std::runtime_error(
          fmt::format("min({}) != max({}), not implemented for select tribute", min, max));
      }

      // Iterative multi-select (D4 Stage B). Unweighted tributes are a free
      // choice of exactly `min` cards (mode 0); weighted ones (monsters that
      // count as two tributes) keep the valid-combination filter (mode 1) —
      // a capability upstream ygopro punts on entirely.
      bool compat = compat_mode_;
      auto send = [this, compat](const std::vector<int> &comb) {
        if (compat) {
          resp_buf_[0] = comb.size();
          for (int i = 0; i < comb.size(); ++i) {
            resp_buf_[i + 1] = comb[i];
          }
          YGO_SetResponseb(pduel_, resp_buf_);
          return;
        }
        uint32_t maxseq = 0;
        uint32_t size = comb.size();
        for (auto &c : comb) {
          maxseq = std::max(maxseq, static_cast<uint32_t>(c));
        }
        auto ret = GetSuitableReturn(maxseq, size);
        memcpy(resp_buf_, &ret, sizeof(ret));
        if (ret == 3) {
          uint32_t nbytes = maxseq / 8 + 1;
          memset(resp_buf_ + 4, 0, nbytes);
          for (int i = 0; i < comb.size(); ++i) {
            resp_buf_[4 + comb[i] / 8] |= (1 << (comb[i] % 8));
          }
          YGO_SetResponseb(pduel_, resp_buf_, 4 + nbytes);
        } else if (ret == 2) {
          memcpy(resp_buf_ + 4, &size, sizeof(size));
          for (int i = 0; i < size; ++i) {
            uint8_t v = comb[i];
            memcpy(resp_buf_ + 8 + i, &v, sizeof(v));
          }
          YGO_SetResponseb(pduel_, resp_buf_, 8 + size);
        } else {
          auto err = fmt::format("Invalid return value: {}", ret);
          throw std::runtime_error(err);
        }
      };
      to_play_ = player;
      if (has_weight) {
        init_multi_select(min, max, specs, 1,
                          combinations_with_weight(release_params, min), send);
      } else {
        init_multi_select(min, max, specs, 0, {}, send);
      }
    } else if (msg_ == MSG_SELECT_SUM) {
      uint8_t mode;
      uint8_t player;
      if (compat_mode_) {
        mode = read_u8();
        player = read_u8();
      } else {
        player = read_u8();
        mode = read_u8();
      }
      auto val = read_u32();
      int min = compat_read<uint8_t, uint32_t>();
      int max = compat_read<uint8_t, uint32_t>();
      auto must_select_size = compat_read<uint8_t, uint32_t>();

      if (mode == 0) {
        // any must_select size: their values are subtracted from the target
      } else {
        if (min != 0 || max != 0 || must_select_size != 0) {
          std::string err = fmt::format(
              "min: {}, max: {}, must select size: {} not implemented for "
              "MSG_SELECT_SUM, mode: {}",
              min, max, must_select_size, mode);
          throw std::runtime_error(err);
        }
      }

      std::vector<int> must_select_params;
      std::vector<std::string> must_select_specs;
      std::vector<int> select_params;
      std::vector<std::string> select_specs;

      must_select_params.reserve(must_select_size);
      must_select_specs.reserve(must_select_size);

      int expected = val;
      if (verbose_) {
        std::vector<Card> must_select;
        must_select.reserve(must_select_size);
        for (int i = 0; i < must_select_size; ++i) {
          auto code = read_u32();
          auto controller = read_u8();
          auto loc = read_u8();
          uint32_t seq;
          if (compat_mode_) {
            seq = read_u8();
          } else {
            seq = read_u32();
            dp_ += 4;
          }
          auto param = read_u32();
          Card card = get_card(controller, loc, seq);
          must_select.push_back(card);
          must_select_params.push_back(param);
        }
        for (int i = 0; i < must_select_size; ++i) {
          expected -= must_select_params[i] & 0xffff;
        }
        auto pl = players_[player];
        pl->notify("Select cards with a total value of " +
                  std::to_string(expected) + ", seperated by spaces.");
        for (const auto &card : must_select) {
          auto spec = card.get_spec(player);
          must_select_specs.push_back(spec);
          pl->notify(card.name_ + " (" + spec +
                    ") must be selected, automatically selected.");
        }
      } else {
        for (int i = 0; i < must_select_size; ++i) {
          dp_ += 4;
          auto controller = read_u8();
          auto loc = read_u8();
          uint32_t seq;
          if (compat_mode_) {
            seq = read_u8();
          } else {
            seq = read_u32();
            dp_ += 4;
          }
          auto param = read_u32();

          auto spec = ls_to_spec(loc, seq, 0, controller != player);
          must_select_specs.push_back(spec);
          must_select_params.push_back(param);
        }
        for (int i = 0; i < must_select_size; ++i) {
          expected -= must_select_params[i] & 0xffff;
        }
      }

      uint8_t select_size = compat_read<uint8_t, uint32_t>();
      select_params.reserve(select_size);
      select_specs.reserve(select_size);

      if (verbose_) {
        std::vector<Card> select;
        select.reserve(select_size);
        for (int i = 0; i < select_size; ++i) {
          auto code = read_u32();
          auto controller = read_u8();
          auto loc = read_u8();
          uint32_t seq;
          if (compat_mode_) {
            seq = read_u8();
          } else {
            seq = read_u32();
            dp_ += 4;
          }
          auto param = read_u32();
          Card card = get_card(controller, loc, seq);
          select.push_back(card);
          select_params.push_back(param);
        }
        auto pl = players_[player];
        for (const auto &card : select) {
          auto spec = card.get_spec(player);
          select_specs.push_back(spec);
          pl->notify(spec + ": " + card.name_);
        }
      } else {
        for (int i = 0; i < select_size; ++i) {
          dp_ += 4;
          auto controller = read_u8();
          auto loc = read_u8();
          uint32_t seq;
          if (compat_mode_) {
            seq = read_u8();
          } else {
            seq = read_u32();
            dp_ += 4;
          }
          auto param = read_u32();

          auto spec = ls_to_spec(loc, seq, 0, controller != player);
          select_specs.push_back(spec);
          select_params.push_back(param);
        }
      }

      std::vector<int> sum_o1(select_size), sum_o2(select_size);
      for (int i = 0; i < select_size; ++i) {
        sum_o1[i] = select_params[i] & 0xffff;
        sum_o2[i] = (select_params[i] >> 16) & 0xffff;
      }
      // mode 0 = exact sum within [min,max] cards; mode 1 = minimal overflow
      std::vector<std::vector<int>> combs;
      {
        std::vector<int> cur;
        bool exact = (mode == 0);
        int min_cnt = exact ? std::max(min, 1) : 1;
        int max_cnt = exact ? std::max(max, min_cnt) : 8;
        sum_combos_dfs(sum_o1, sum_o2, expected, exact, min_cnt, max_cnt, 0,
                       cur, combs, 64);
      }
      if (std::getenv("YGOENV_QUERY_DEBUG")) {
        fmt::print(stderr,
                   "[sdbg] select_sum mode={} acc={} min={} max={} must={} "
                   "expected={} params={} n_combs={}\n",
                   mode, val, min, max, must_select_size, expected,
                   fmt::join(select_params, ","), combs.size());
      }

      // Iterative multi-select (D4 Stage B, mode 1): the sum solver's combs
      // define validity; the agent picks one card per env step and the pick
      // prefix-filters the remaining combs.
      bool compat = compat_mode_;
      uint32_t must_n = must_select_size;
      auto send = [this, compat, must_n](const std::vector<int> &comb) {
        if (compat) {
          resp_buf_[0] = must_n + comb.size();
          for (int i = 0; i < must_n; ++i) {
            resp_buf_[i + 1] = 0;
          }
          for (int i = 0; i < comb.size(); ++i) {
            resp_buf_[i + must_n + 1] = comb[i];
          }
          YGO_SetResponseb(pduel_, resp_buf_);
          return;
        }
        int32_t ret = 3;
        memcpy(resp_buf_, &ret, sizeof(ret));
        uint32_t maxidx = 0;
        for (int c : comb) {
          maxidx = std::max(maxidx, static_cast<uint32_t>(c));
        }
        uint32_t nbytes = maxidx / 8 + 1;
        memset(resp_buf_ + 4, 0, nbytes);
        for (int i = 0; i < comb.size(); ++i) {
          resp_buf_[4 + comb[i] / 8] |= (1 << (comb[i] % 8));
        }
        YGO_SetResponseb(pduel_, resp_buf_, 4 + nbytes);
      };
      to_play_ = player;
      if (combs.empty()) {
        throw std::runtime_error("select_sum: no valid combination found");
      }
      init_multi_select(1, select_size, select_specs, 1, combs, send);
    } else if (msg_ == MSG_SELECT_CHAIN) {
      auto player = read_u8();
      uint32_t size;
      if (compat_mode_) {
        size = read_u8();
      }
      auto spe_count = read_u8();
      bool forced = read_u8();
      dp_ += 8;
      if (!compat_mode_) {
        size = read_u32();
      }
      // auto hint_timing = read_u32();
      // auto other_timing = read_u32();

      std::vector<Card> cards;
      // u64: the modern core packs (code << 20) | strindex, which a u32
      // truncates for almost every card code.
      std::vector<uint64_t> descs;
      std::vector<uint32_t> spec_codes;
      for (int i = 0; i < size; ++i) {
        uint8_t flag;
        if (compat_mode_) {
          flag = read_u8();
        }
        CardCode code = read_u32();
        auto loc_info = read_loc_info();
        if (verbose_) {
          Card card = c_get_card(code);
          card.set_location(loc_info);
          cards.push_back(card);
          spec_codes.push_back(card.get_spec_code(player));
        } else {
          spec_codes.push_back(
            ls_to_spec_code(loc_info, player));
        }
        uint64_t desc = compat_read<uint32_t, uint64_t>();
        descs.push_back(desc);
        if (!compat_mode_) {
          flag = read_u8();
        }
      }

      if ((size == 0) && (spe_count == 0)) {
        // non-GUI don't need this
        // if (verbose_) {
        //   fmt::println("keep processing");
        // }
        YGO_SetResponsei(pduel_, -1);
        return;
      }

      auto pl = players_[player];
      auto op = players_[1 - player];
      chaining_player_ = player;
      if (!op->seen_waiting_) {
        if (verbose_) {
          op->notify("Waiting for opponent.");
        }
        op->seen_waiting_ = true;
      }

      std::vector<int> chain_index;
      ankerl::unordered_dense::map<uint32_t, int> chain_counts;
      ankerl::unordered_dense::map<uint32_t, int> chain_orders;
      std::vector<std::string> chain_specs;
      std::vector<std::string> effect_descs;
      for (int i = 0; i < size; i++) {
        chain_index.push_back(i);
        chain_counts[spec_codes[i]] += 1;
      }
      for (int i = 0; i < size; i++) {
        auto spec_code = spec_codes[i];
        auto cs = code_to_spec(spec_code);
        auto chain_count = chain_counts[spec_code];
        if (chain_count > 1) {
          cs.push_back('a' + chain_orders[spec_code]);
        }
        chain_orders[spec_code]++;
        chain_specs.push_back(cs);
        if (verbose_) {
          const auto &card = cards[i];
          effect_descs.push_back(card.get_effect_description(descs[i], true));
        }
      }

      if (verbose_) {
        if (forced) {
          pl->notify("Select chain:");
        } else {
          pl->notify("Select chain (c to cancel):");
        }
        for (int i = 0; i < size; i++) {
          const auto &effect_desc = effect_descs[i];
          if (effect_desc.empty()) {
            pl->notify(chain_specs[i] + ": " + cards[i].name_);
          } else {
            pl->notify(chain_specs[i] + " (" + cards[i].name_ +
                       "): " + effect_desc);
          }
        }
      }

      for (int i = 0; i < size; i++) {
        auto [code_d, eff_idx] = unpack_desc(descs[i]);
        // spec_ carries the bare spec (the option string keeps edopro's
        // duplicate-disambiguation letter); the effect index disambiguates
        // in the typed encoding, as in ygopro.
        auto la = LegalAction::activate_spec(eff_idx, code_to_spec(spec_codes[i]));
        if (code_d != 0) {
          la.cid_ = c_get_card_id_or_zero(code_d);
        }
        push_action(la, chain_specs[i]);
      }
      if (!forced) {
        push_action(LegalAction::cancel(), "c");
      }
      to_play_ = player;
      callback_ = [this, forced](int idx) {
        const auto &option = options_[idx];
        if ((option == "c") && (!forced)) {
          YGO_SetResponsei(pduel_, -1);
          return;
        }
        YGO_SetResponsei(pduel_, idx);
      };
    } else if (msg_ == MSG_SELECT_YESNO) {
      auto player = read_u8();

      auto desc = compat_read<uint32_t, uint64_t>();
      auto [code_d, eff_idx] = unpack_desc(desc);
      if (verbose_) {
        auto pl = players_[player];
        std::string opt;
        const Card *card = code_d != 0 ? c_find_card(code_d) : nullptr;
        if (card != nullptr) {
          uint32_t opt_idx = eff_idx - kCardEffectOffset;
          if (opt_idx < card->strings_.size()) {
            opt = card->strings_[opt_idx];
          }
          if (opt.empty()) {
            opt = "Unknown question from " + card->name_ + ". Yes or no?";
          }
        } else {
          opt = get_system_string(static_cast<uint32_t>(desc));
        }
        pl->notify(opt);
        pl->notify("Please enter y or n.");
      }
      {
        auto la = LegalAction::activate_spec(eff_idx, "");
        if (code_d != 0) {
          la.cid_ = c_get_card_id_or_zero(code_d);
        }
        push_action(la, "y");
        push_action(LegalAction::cancel(), "n");
      }
      to_play_ = player;
      callback_ = [this](int idx) {
        if (idx == 0) {
          YGO_SetResponsei(pduel_, 1);
        } else if (idx == 1) {
          YGO_SetResponsei(pduel_, 0);
        } else {
          throw std::runtime_error("Invalid option");
        }
      };
    } else if (msg_ == MSG_SELECT_EFFECTYN) {
      auto player = read_u8();

      std::string spec;
      uint64_t desc_v = 0;
      if (verbose_) {
        CardCode code = read_u32();
        auto loc_info = read_loc_info();
        Card card = c_get_card(code);
        card.set_location(loc_info);
        auto desc = compat_read<uint32_t, uint64_t>();
        desc_v = desc;
        auto pl = players_[player];
        spec = card.get_spec(player);
        auto name = card.name_;
        std::string s;
        if (desc == 0) {
          // From [%ls], activate [%ls]?
          s = "From " + card.get_spec(player) + ", activate " + name + "?";
        } else if (desc < 2048) {
          s = get_system_string(desc);
          std::string fmt_str = "[%ls]";
          auto pos = find_substrs(s, fmt_str);
          if (pos.size() == 0) {
            // nothing to replace
          } else if (pos.size() == 1) {
            auto p = pos[0];
            s = s.substr(0, p) + name + s.substr(p + fmt_str.size());
          } else if (pos.size() == 2) {
            auto p1 = pos[0];
            auto p2 = pos[1];
            s = s.substr(0, p1) + card.get_spec(player) +
                s.substr(p1 + fmt_str.size(), p2 - p1 - fmt_str.size()) + name +
                s.substr(p2 + fmt_str.size());
          } else {
            throw std::runtime_error("Unknown effectyn desc " +
                                     std::to_string(desc) + " of " + name);
          }
        }  else if (desc < (1u << 20)) {
          s = get_system_string(desc);
        } else {
          // modern packing: (code << 20) | string index
          CardCode code = static_cast<CardCode>(desc >> 20);
          uint32_t offset = static_cast<uint32_t>(desc & 0xfffff);
          auto it = cards_.find(code);
          if (it != cards_.end() && offset < it->second.strings_.size()) {
            s = it->second.strings_[offset];
          }
          if (s.empty()) {
            s = "???";
          }
        }
        pl->notify(s);
        pl->notify("Please enter y or n.");
      } else {
        dp_ += 4;
        auto loc_info = read_loc_info();
        desc_v = compat_read<uint32_t, uint64_t>();
        spec = ls_to_spec(loc_info, player);
      }
      {
        auto [code_d, eff_idx] = unpack_desc(desc_v);
        auto la = LegalAction::activate_spec(eff_idx, spec);
        if (code_d != 0) {
          la.cid_ = c_get_card_id_or_zero(code_d);
        }
        push_action(la, "y " + spec);
        auto no = LegalAction::cancel();
        no.spec_ = spec;
        push_action(no, "n " + spec);
      }
      to_play_ = player;
      callback_ = [this](int idx) {
        if (idx == 0) {
          YGO_SetResponsei(pduel_, 1);
        } else if (idx == 1) {
          YGO_SetResponsei(pduel_, 0);
        } else {
          throw std::runtime_error("Invalid option");
        }
      };
    } else if (msg_ == MSG_SELECT_OPTION) {
      auto player = read_u8();
      auto size = read_u8();
      for (int i = 0; i < size; ++i) {
        auto opt = compat_read<uint32_t, uint64_t>();
        auto [code_d, eff_idx] = unpack_desc(opt);
        auto la = LegalAction::activate_spec(eff_idx, "");
        if (code_d != 0) {
          la.cid_ = c_get_card_id_or_zero(code_d);
        }
        push_action(la, std::to_string(i + 1));
        if (verbose_) {
          auto pl = players_[player];
          if (i == 0) {
            pl->notify("Select an option:");
          }
          std::string s;
          const Card *card = code_d != 0 ? c_find_card(code_d) : nullptr;
          if (card != nullptr) {
            uint32_t opt_idx = eff_idx - kCardEffectOffset;
            if (opt_idx < card->strings_.size()) {
              s = card->strings_[opt_idx];
            }
            if (s.empty()) {
              s = "option of " + card->name_;
            }
          } else {
            s = get_system_string(static_cast<uint32_t>(opt));
          }
          pl->notify(std::to_string(i + 1) + ": " + s);
        }
      }
      to_play_ = player;
      callback_ = [this](int idx) {
        if (verbose_) {
          players_[to_play_]->notify("You selected option " + options_[idx] +
                                     ".");
          players_[1 - to_play_]->notify(players_[to_play_]->nickname_ +
                                         " selected option " + options_[idx] +
                                         ".");
        }

        YGO_SetResponsei(pduel_, idx);
      };
    } else if (msg_ == MSG_SELECT_IDLECMD) {
      int32_t player = read_u8();
      auto summonable_ = read_cardlist_spec();
      auto spsummon_ = read_cardlist_spec();
      auto repos_ = read_cardlist_spec(false);
      auto idle_mset_ = read_cardlist_spec();
      auto idle_set_ = read_cardlist_spec();
      auto idle_activate_ = read_cardlist_spec(true, true);
      bool to_bp_ = read_u8();
      bool to_ep_ = read_u8();
      read_u8(); // can_shuffle

      int offset = 0;

      auto pl = players_[player];
      if (verbose_) {
        pl->notify("Select a card and action to perform.");
      }
      for (const auto &[code, spec, data] : summonable_) {
        std::string option = "s " + spec;
        auto la = LegalAction::act_spec(ActionAct::Summon, spec);
        la.cid_ = c_get_card_id_or_zero(code);
        push_action(la, option);
        if (verbose_) {
          const auto &name = c_get_card(code).name_;
          pl->notify(option + ": Summon " + name +
                     " in face-up attack position.");
        }
      }
      offset += summonable_.size();
      int spsummon_offset = offset;
      for (const auto &[code, spec, data] : spsummon_) {
        std::string option = "c " + spec;
        auto la = LegalAction::act_spec(ActionAct::SpSummon, spec);
        la.cid_ = c_get_card_id_or_zero(code);
        push_action(la, option);
        if (verbose_) {
          const auto &name = c_get_card(code).name_;
          pl->notify(option + ": Special summon " + name + ".");
        }
      }
      offset += spsummon_.size();
      int repos_offset = offset;
      for (const auto &[code, spec, data] : repos_) {
        std::string option = "r " + spec;
        auto la = LegalAction::act_spec(ActionAct::Repo, spec);
        la.cid_ = c_get_card_id_or_zero(code);
        push_action(la, option);
        if (verbose_) {
          const auto &name = c_get_card(code).name_;
          pl->notify(option + ": Reposition " + name + ".");
        }
      }
      offset += repos_.size();
      int mset_offset = offset;
      for (const auto &[code, spec, data] : idle_mset_) {
        std::string option = "m " + spec;
        auto la = LegalAction::act_spec(ActionAct::MSet, spec);
        la.cid_ = c_get_card_id_or_zero(code);
        push_action(la, option);
        if (verbose_) {
          const auto &name = c_get_card(code).name_;
          pl->notify(option + ": Summon " + name +
                     " in face-down defense position.");
        }
      }
      offset += idle_mset_.size();
      int set_offset = offset;
      for (const auto &[code, spec, data] : idle_set_) {
        std::string option = "t " + spec;
        auto la = LegalAction::act_spec(ActionAct::Set, spec);
        la.cid_ = c_get_card_id_or_zero(code);
        push_action(la, option);
        if (verbose_) {
          const auto &name = c_get_card(code).name_;
          pl->notify(option + ": Set " + name + ".");
        }
      }
      offset += idle_set_.size();
      int activate_offset = offset;
      ankerl::unordered_dense::map<std::string, int> idle_activate_count;
      for (const auto &[code, spec, data] : idle_activate_) {
        idle_activate_count[spec] += 1;
      }
      ankerl::unordered_dense::map<std::string, int> activate_count;
      for (const auto &[code, spec, data] : idle_activate_) {
        std::string option = "v " + spec;
        int count = idle_activate_count[spec];
        activate_count[spec]++;
        if (count > 1) {
          option.push_back('a' + activate_count[spec] - 1);
        }
        {
          auto [code_d, eff_idx] = unpack_desc(data);
          auto la = LegalAction::activate_spec(eff_idx, spec);
          la.cid_ = c_get_card_id_or_zero(code_d != 0 ? code_d : code);
          push_action(la, option);
        }
        if (verbose_) {
          pl->notify(option + ": " +
                     c_get_card(code).get_effect_description(data));
        }
      }

      if (to_bp_) {
        std::string cmd = "b";
        push_action(LegalAction::phase(ActionPhase::Battle), cmd);
        if (verbose_) {
          pl->notify(cmd + ": Enter the battle phase.");
        }
      }
      if (to_ep_) {
        if (!to_bp_) {
          std::string cmd = "e";
          push_action(LegalAction::phase(ActionPhase::End), cmd);
          if (verbose_) {
            pl->notify(cmd + ": End phase.");
          }
        }
      }

      to_play_ = player;
      callback_ = [this, spsummon_offset, repos_offset, mset_offset, set_offset,
                   activate_offset](int idx) {
        const auto &option = options_[idx];
        char cmd = option[0];
        if (cmd == 'b') {
          YGO_SetResponsei(pduel_, 6);
        } else if (cmd == 'e') {
          YGO_SetResponsei(pduel_, 7);
        } else {
          auto spec = option.substr(2);
          if (cmd == 's') {
            uint32_t idx_ = idx;
            YGO_SetResponsei(pduel_, idx_ << 16);
          } else if (cmd == 'c') {
            uint32_t idx_ = idx - spsummon_offset;
            YGO_SetResponsei(pduel_, (idx_ << 16) + 1);
          } else if (cmd == 'r') {
            uint32_t idx_ = idx - repos_offset;
            YGO_SetResponsei(pduel_, (idx_ << 16) + 2);
          } else if (cmd == 'm') {
            uint32_t idx_ = idx - mset_offset;
            YGO_SetResponsei(pduel_, (idx_ << 16) + 3);
          } else if (cmd == 't') {
            uint32_t idx_ = idx - set_offset;
            YGO_SetResponsei(pduel_, (idx_ << 16) + 4);
          } else if (cmd == 'v') {
            uint32_t idx_ = idx - activate_offset;
            YGO_SetResponsei(pduel_, (idx_ << 16) + 5);
          } else {
            throw std::runtime_error("Invalid option: " + option);
          }
        }
      };
    } else if (msg_ == MSG_SELECT_PLACE) {
      auto player = read_u8();
      auto count = read_u8();
      if (count == 0) {
        count = 1;
      }
      auto flag = read_u32();
      for (const auto &spec : flag_to_usable_cardspecs(flag)) {
        auto la = LegalAction::place(
            static_cast<ActionPlace>(cmd_place2id.at(spec)));
        push_action(la, spec);
      }
      if (verbose_) {
        std::string specs_str = options_[0];
        for (int i = 1; i < options_.size(); ++i) {
          specs_str += ", " + options_[i];
        }
        if (count == 1) {
          players_[player]->notify("Select place for card, one of " +
                                   specs_str + ".");
        } else {
          players_[player]->notify("Select " + std::to_string(count) +
                                   " places for card, from " + specs_str + ".");
        }
      }
      to_play_ = player;
      callback_ = [this, player](int idx) {
        int y = player + 1;
        std::string spec = options_[idx];
        auto plr = player;
        if (spec[0] == 'o') {
          plr = 1 - player;
          spec = spec.substr(1);
        }
        auto [loc, seq, pos] = spec_to_ls(spec);
        resp_buf_[0] = plr;
        resp_buf_[1] = loc;
        resp_buf_[2] = seq;
        YGO_SetResponseb(pduel_, resp_buf_, 3);
      };
    } else if (msg_ == MSG_SELECT_DISFIELD) {
      auto player = read_u8();
      auto count = read_u8();
      if (count == 0) {
        count = 1;
      }
      auto flag = read_u32();
      for (const auto &spec : flag_to_usable_cardspecs(flag)) {
        auto la = LegalAction::place(
            static_cast<ActionPlace>(cmd_place2id.at(spec)));
        push_action(la, spec);
      }
      if (verbose_) {
        std::string specs_str = options_[0];
        for (int i = 1; i < options_.size(); ++i) {
          specs_str += ", " + options_[i];
        }
        if (count == 1) {
          players_[player]->notify("Select place for card, one of " +
                                   specs_str + ".");
        } else {
          throw std::runtime_error("Select disfield count " +
                                   std::to_string(count) + " not implemented");
        }
      }
      to_play_ = player;
      callback_ = [this, player](int idx) {
        int y = player + 1;
        std::string spec = options_[idx];
        auto plr = player;
        if (spec[0] == 'o') {
          plr = 1 - player;
          spec = spec.substr(1);
        }
        auto [loc, seq, pos] = spec_to_ls(spec);
        resp_buf_[0] = plr;
        resp_buf_[1] = loc;
        resp_buf_[2] = seq;
        YGO_SetResponseb(pduel_, resp_buf_, 3);
      };
    } else if (msg_ == MSG_ANNOUNCE_NUMBER) {
      auto player = read_u8();
      int count = read_u8();
      std::vector<int> numbers;
      for (int i = 0; i < count; ++i) {
        int number = compat_read<uint32_t, uint64_t>();
        if (number <= 0 || number > 12) {
          throw std::runtime_error("Number " + std::to_string(number) +
                                   " not implemented for announce number");
        }
        numbers.push_back(number);
        push_action(LegalAction::number(static_cast<uint8_t>(number)),
                    std::string(1, '0' + number));
      }
      if (std::getenv("YGOENV_QUERY_DEBUG")) {
        fmt::print(stderr, "[adbg] announce_number player={} count={} numbers={}\n",
                   player, count, fmt::join(numbers, ","));
      }
      if (verbose_) {
        auto pl = players_[player];
        std::string str = "Select a number, one of: [";
        for (int i = 0; i < count; ++i) {
          str += std::to_string(numbers[i]);
          if (i < count - 1) {
            str += ", ";
          }
        }
        str += "]";
        pl->notify(str);
      }
      to_play_ = player;
      callback_ = [this](int idx) {
        if (std::getenv("YGOENV_QUERY_DEBUG")) {
          fmt::print(stderr, "[adbg] announce_number respond idx={}\n", idx);
        }
        YGO_SetResponsei(pduel_, idx);
      };
    } else if (msg_ == MSG_ANNOUNCE_ATTRIB) {
      auto player = read_u8();
      int count = read_u8();
      auto flag = read_u32();

      int n_attrs = 7;

      std::vector<uint8_t> attrs;
      for (int i = 0; i < n_attrs; i++) {
        if (flag & (1 << i)) {
          attrs.push_back(i + 1);
        }
      }

      if (count != 1) {
        throw std::runtime_error("Announce attrib count " +
                                 std::to_string(count) + " not implemented");
      }

      if (verbose_) {
        auto pl = players_[player];
        pl->notify("Select " + std::to_string(count) +
                   " attributes separated by spaces:");
        for (int i = 0; i < attrs.size(); i++) {
          pl->notify(std::to_string(attrs[i]) + ": " +
                     attribute2str.at(1 << (attrs[i] - 1)));
        }
      }

      auto combs = combinations(attrs.size(), count);
      for (const auto &comb : combs) {
        std::string option = "";
        for (int j = 0; j < count; ++j) {
          option += std::to_string(attrs[comb[j]]);
          if (j < count - 1) {
            option += " ";
          }
        }
        // count == 1 is enforced above, so the typed action carries the one
        // announced attribute bit.
        push_action(LegalAction::attribute(1 << (attrs[comb[0]] - 1)), option);
      }

      to_play_ = player;
      callback_ = [this](int idx) {
        const auto &option = options_[idx];
        uint32_t resp = 0;
        int i = 0;
        while (i < option.size()) {
          resp |= 1 << (option[i] - '1');
          i += 2;
        }
        YGO_SetResponsei(pduel_, resp);
      };

    } else if (msg_ == MSG_ANNOUNCE_RACE) {
      auto player = read_u8();
      int count = read_u8();
      uint64_t available = read_u64();
      if (count != 1) {
        throw std::runtime_error("Announce race count " +
                                 std::to_string(count) + " not implemented");
      }
      for (int i = 0; i < 64; ++i) {
        if (available & (1ULL << i)) {
          LegalAction la;
          la.race_ = 1ULL << i;
          push_action(la, std::string(1, static_cast<char>('0' + i)));
        }
      }
      if (verbose_) {
        players_[player]->notify("Announce a race");
      }
      to_play_ = player;
      callback_ = [this](int idx) {
        uint64_t resp = 1ULL << (options_[idx][0] - '0');
        memcpy(resp_buf_, &resp, 8);
        YGO_SetResponseb(pduel_, resp_buf_, 8);
      };
    } else if (msg_ == MSG_ANNOUNCE_CARD) {
      auto player = read_u8();
      int count = read_u8();
      std::vector<uint64_t> opcodes;
      for (int i = 0; i < count; ++i) {
        opcodes.push_back(read_u64());
      }
      std::vector<CardCode> candidates;
      for (const auto &[code, data] : cards_data_) {
        if (data.code != 0 && is_declarable(data, opcodes)) {
          candidates.push_back(code);
        }
      }
      if (candidates.empty()) {
        throw std::runtime_error("No declarable cards for announce card");
      }
      std::sort(candidates.begin(), candidates.end());
      for (const auto &code : candidates) {
        LegalAction la;
        la.cid_ = c_get_card_id_or_zero(code);
        push_action(la, std::to_string(code));
      }
      if (verbose_) {
        players_[player]->notify("Declare a card, " +
                                 std::to_string(candidates.size()) +
                                 " candidates");
      }
      to_play_ = player;
      callback_ = [this](int idx) {
        YGO_SetResponsei(pduel_,
                         static_cast<int32_t>(std::stoul(options_[idx])));
      };
    } else if (msg_ == MSG_SELECT_POSITION) {
      auto player = read_u8();
      auto code = read_u32();
      auto valid_pos = read_u8();

      if (verbose_) {
        auto pl = players_[player];
        auto card = c_get_card(code);
        pl->notify("Select position for " + card.name_ + ":");
      }

      std::vector<uint8_t> positions;
      int i = 1;
      for (auto pos : {POS_FACEUP_ATTACK, POS_FACEDOWN_ATTACK,
                       POS_FACEUP_DEFENSE, POS_FACEDOWN_DEFENSE}) {
        if (valid_pos & pos) {
          positions.push_back(pos);
          {
            LegalAction la;
            la.position_ = pos;
            la.cid_ = c_get_card_id_or_zero(code);
            push_action(la, std::to_string(i));
          }
          if (verbose_) {
            auto pl = players_[player];
            pl->notify(fmt::format("{}: {}", i, position_to_string(pos)));
          }
        }
        i++;
      }

      to_play_ = player;
      callback_ = [this](int idx) {
        uint8_t pos = options_[idx][0] - '1';
        YGO_SetResponsei(pduel_, 1 << pos);
      };
    } else {
      show_deck(0);
      show_deck(1);
      throw std::runtime_error(
        fmt::format("Unknown message {}, length {}, dp {}",
        msg_, dl_, dp_));
    }
  }

  int GetSuitableReturn(uint32_t maxseq, uint32_t size) {
    using nl8 = std::numeric_limits<uint8_t>;
    using nl16 = std::numeric_limits<uint16_t>;
    using nl32 = std::numeric_limits<uint32_t>;
    if(maxseq < nl8::max()) {
      if(maxseq >= size * nl8::digits)
        return 2;
    } else if(maxseq < nl16::max()) {
      if(maxseq >= size * nl16::digits)
        return 1;
    }
    else if(maxseq < nl32::max()) {
      if(maxseq >= size * nl32::digits)
        return 0;
    }
    return 3;
  }

  void _damage(uint8_t player, uint32_t amount) {
    lp_[player] -= amount;
    if (verbose_) {
      auto lp = players_[player];
      lp->notify(fmt::format("Your lp decreased by {}, now {}", amount, lp_[player]));
      players_[1 - player]->notify(fmt::format("{}'s lp decreased by {}, now {}",
                                   lp->nickname_, amount, lp_[player]));
    }
  }

  void _recover(uint8_t player, uint32_t amount) {
    lp_[player] += amount;
    if (verbose_) {
      auto lp = players_[player];
      lp->notify(fmt::format("Your lp increased by {}, now {}", amount, lp_[player]));
      players_[1 - player]->notify(fmt::format("{}'s lp increased by {}, now {}",
                                   lp->nickname_, amount, lp_[player]));
    }
  }

  void _duel_end(uint8_t player, uint8_t reason) {
    winner_ = player;
    win_reason_ = reason;

    std::unique_lock<std::shared_timed_mutex> ulock(duel_mtx);
    YGO_EndDuel(pduel_);
    ulock.unlock();

    duel_started_ = false;
  }
};

using EDOProEnvPool = AsyncEnvPool<EDOProEnv>;

} // namespace edopro

#endif // YGOENV_EDOPro_EDOPro_H_
