#include "comet/arena_construction_config.hpp"
#include <charconv>
#include <cmath>
#include <sstream>
#include <stdexcept>
namespace comet::decomp {
namespace {
template <class T> T number(const std::string &s) {
  T v{};
  auto r = std::from_chars(s.data(), s.data() + s.size(), v);
  if (r.ec != std::errc{} || r.ptr != s.data() + s.size())
    throw std::invalid_argument("invalid number: " + s);
  return v;
}
unsigned bounded(const std::string &s, unsigned max) {
  auto v = number<unsigned>(s);
  if (v > max)
    throw std::invalid_argument("number out of range");
  return v;
}
float finite(const std::string &s) {
  auto f = number<float>(s);
  if (!std::isfinite(f))
    throw std::invalid_argument("nonfinite number");
  return f;
}
} // namespace
ArenaConstructionSimulationConfig
read_arena_construction_config(std::istream &in, std::size_t &line) {
  ArenaConstructionSimulationConfig c;
  bool tesla_seen=false, cannon_seen=false;
  bool world = false, production_seen = false, transport_seen = false, dynamics_seen = false, contacts_seen = false;
  std::array<bool, 4> quota_seen{}, unit_base_seen{}, unit_target_seen{};
  std::string text;
  while (std::getline(in, text)) {
    ++line;
    if (text.size() > 4096)
      throw std::invalid_argument("line too long");
    text = text.substr(0, text.find('#'));
    std::istringstream words(text);
    std::vector<std::string> w;
    for (std::string token; words >> token;)
      w.push_back(token);
    if (w.empty())
      continue;
    if (w[0] == "run" && w.size() == 1) {
      if (!world || c.players.empty())
        throw std::invalid_argument("world and players required");
      if(c.ground_contacts_enabled)c.ground_dynamics_enabled=true;
      return c;
    }
    if (w[0] == "world" && w.size() == 3 && !world && c.players.empty()) {
      c.capacity = bounded(w[1], 65536);
      c.seed = number<std::uint32_t>(w[2]);
      world = true;
    } else if (w[0] == "player" && w.size() == 7 && world &&
               c.players.size() < 4) {
      ArenaConstructionAccount a;
      a.route.base = {static_cast<std::uint8_t>(bounded(w[1], 23)),
                      static_cast<std::uint8_t>(bounded(w[2], 23))};
      a.route.team = bounded(w[3], 3);
      a.resource = finite(w[4]);
      a.resource_max = finite(w[5]);
      a.route.inactive = bounded(w[6], 1);
      if (a.resource < 0 || a.resource > a.resource_max)
        throw std::invalid_argument("invalid balance");
      c.players.push_back(a);
    } else if (w[0] == "cannon" && w.size()==2 && world) {
      if(cannon_seen)throw std::invalid_argument("duplicate cannon flag");
      cannon_seen=true;c.cannon_enabled=bounded(w[1],1);
    } else if (w[0] == "tesla" && w.size()==2 && world) {
      if(tesla_seen)throw std::invalid_argument("duplicate Tesla flag");
      tesla_seen=true;c.tesla_enabled=bounded(w[1],1);
    } else if (w[0] == "contacts" && w.size()==2 && world) {
      if(contacts_seen)throw std::invalid_argument("duplicate contacts flag");
      contacts_seen=true;c.ground_contacts_enabled=bounded(w[1],1);
    } else if (w[0] == "dynamics" && w.size() == 2 && world) {
      if (dynamics_seen) throw std::invalid_argument("duplicate dynamics flag");
      dynamics_seen = true;
      c.ground_dynamics_enabled = bounded(w[1], 1);
    } else if (w[0] == "transport" && w.size() == 2 && world) {
      if (transport_seen) throw std::invalid_argument("duplicate transport flag");
      transport_seen = true;
      c.ground_transport_enabled = bounded(w[1], 1);
    } else if (w[0] == "production" && w.size() == 2 && world) {
      if (production_seen)
        throw std::invalid_argument("duplicate production flag");
      production_seen = true;
      c.production_enabled = bounded(w[1], 1);
    } else if(w[0]=="deploy" && w.size()==4 && world){
      const auto owner=bounded(w[1],3),kind=bounded(w[2],255);
      const float strength=finite(w[3]);
      if(owner>=c.players.size() || c.ground_deployment[owner] ||
         !(kind==40 || kind==41 || (kind>=43 && kind<=47)) || strength<0 || strength>1 || std::fpclassify(strength)==FP_SUBNORMAL)
        throw std::invalid_argument("invalid or duplicate ground deployment");
      c.ground_deployment[owner]=ArenaGroundDeploymentControl{static_cast<std::uint8_t>(kind),strength};
    } else if (w[0] == "base-counter" && w.size()==3 && world) {
      const auto owner=bounded(w[1],3);
      if(owner>=c.players.size() || c.base_counters[owner])
        throw std::invalid_argument("invalid or duplicate base counter");
      c.base_counters[owner]=static_cast<std::int16_t>(bounded(w[2],32767));
    } else if (w[0] == "unit-base" && w.size() == 3 && world) {
      const auto owner = bounded(w[1], 3);
      if (owner >= c.players.size() || unit_base_seen[owner])
        throw std::invalid_argument("invalid or duplicate unit base");
      unit_base_seen[owner] = true;
      c.unit_base_available[owner] = bounded(w[2], 1);
    } else if (w[0] == "unit-target" && w.size() == 3 && world) {
      const auto owner = bounded(w[1], 3), target = bounded(w[2], 255);
      if (owner >= c.players.size() || unit_target_seen[owner] ||
          (target != 255 && (target >= c.players.size() || target == owner)))
        throw std::invalid_argument("invalid or duplicate unit target");
      unit_target_seen[owner] = true;
      c.unit_target_overrides[owner] = target;
    } else if (w[0] == "unit-quota" && w.size() == 4 && world) {
      const auto owner = bounded(w[1], 3);
      if (owner >= c.players.size())
        throw std::invalid_argument("quota player missing");
      if (quota_seen[owner])
        throw std::invalid_argument("duplicate production quota");
      quota_seen[owner] = true;
      c.unit_quotas[owner].active = bounded(w[2], 32767);
      c.unit_quotas[owner].capacity = bounded(w[3], 65535);
      if (static_cast<std::uint32_t>(c.unit_quotas[owner].active) >
          c.unit_quotas[owner].capacity)
        throw std::invalid_argument("invalid production quota");
    } else if (w[0] == "tuning" && w.size() == 7 && world) {
      auto op = bounded(w[1], 31);
      if (!arena_structure_initial_level(op) || c.tuning[op])
        throw std::invalid_argument("unsupported or duplicate tuning");
      ArenaStructureTableValues t;
      t.scalar_64 = number<std::int16_t>(w[2]);
      t.scalar_38_denominator = number<std::int16_t>(w[3]);
      t.first_byte = bounded(w[4], 255);
      t.second_byte = bounded(w[5], 255);
      t.scalar_84 = finite(w[6]);
      if (t.scalar_38_denominator <= 0)
        throw std::invalid_argument("nonpositive denominator");
      c.tuning[op] = t;
    } else
      throw std::invalid_argument("invalid construction configuration");
  }
  throw std::invalid_argument("missing run terminator");
}
} // namespace comet::decomp
