#include "comet/arena_construction_config.hpp"
#include "comet/native_construction_scene.hpp"
#include "comet/native_session.hpp"
#include <charconv>
#include <bit>
#include <cmath>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
using namespace comet::decomp;
namespace {
template <class T> T number(const std::string &s) {
  T v{};
  auto r = std::from_chars(s.data(), s.data() + s.size(), v);
  if (r.ec != std::errc{} || r.ptr != s.data() + s.size())
    throw std::invalid_argument("invalid number");
  return v;
}
unsigned byte(const std::string &s) {
  auto v = number<unsigned>(s);
  if (v > 255)
    throw std::invalid_argument("byte out of range");
  return v;
}
void completion(const ArenaConstructionSimulationStep &s) {
  constexpr const char *names[] = {
      "idle",           "invalid",      "route-rejected", "constructed",
      "release-queued", "release-full", "released"};
  std::cout << "step " << names[static_cast<unsigned>(s.status)];
  if (s.entity_index)
    std::cout << " entity=" << *s.entity_index;
  std::cout << '\n';
}
} // namespace
int main(int argc, char **argv) {
  std::size_t line = 0;
  try {
    std::filesystem::path maps;
    unsigned level = 0;
    for (int i = 1; i < argc; ++i) {
      std::string arg = argv[i];
      if ((arg == "--map" || arg == "--level") && i + 1 < argc) {
        if (arg == "--map")
          maps = argv[++i];
        else
          level = number<unsigned>(argv[++i]);
      } else
        throw std::invalid_argument("usage: comet_construction_replay [--map "
                                    "directory --level id] < scenario");
    }
    auto config = read_arena_construction_config(std::cin, line);
    std::unique_ptr<ArenaConstructionSimulation> simulation;
    std::unique_ptr<NativeConstructionScene> scene;
    if (maps.empty())
      simulation = std::make_unique<ArenaConstructionSimulation>(config);
    else {
      NativeSession session;
      NativeSessionConfig c;
      c.data_root = maps;
      c.arena.player_count = config.players.size();
      c.arena.secondary_player = config.players.size() > 1 ? 1 : -1;
      auto result = initialize_native_session(c, level, session);
      if (!result)
        throw std::invalid_argument(result.detail);
      scene = std::make_unique<NativeConstructionScene>(session.arena, config);
      std::cout << "map level=" << level << " live=" << scene->imports().live
                << " display-only=" << scene->imports().display_only << '\n';
    }
    auto state = [&]() -> const ArenaConstructionSimulation & {
      return scene ? scene->simulation() : *simulation;
    };
    auto enqueue = [&](ArenaConstructionRequest r) {
      return scene ? scene->enqueue(r) : simulation->enqueue(r);
    };
    auto step = [&]() { return scene ? scene->step() : simulation->step(); };
    std::string text;
    while (std::getline(std::cin, text)) {
      ++line;
      if (text.size() > 4096)
        throw std::invalid_argument("line too long");
      text = text.substr(0, text.find('#'));
      std::istringstream input(text);
      std::vector<std::string> w;
      for (std::string t; input >> t;)
        w.push_back(t);
      if (w.empty())
        continue;
      if (w[0] == "queue" && w.size() == 5) {
        ArenaConstructionRequest r{static_cast<std::uint8_t>(byte(w[1])),
                                   static_cast<std::uint8_t>(byte(w[2])),
                                   static_cast<std::uint8_t>(byte(w[3])),
                                   static_cast<std::uint8_t>(byte(w[4]))};
        auto result = enqueue(r);
        constexpr const char *names[] = {"queued",
                                         "invalid",
                                         "occupied",
                                         "cell-full",
                                         "no-capacity",
                                         "duplicate",
                                         "insufficient-resources",
                                         "queue-full"};
        std::cout << "queue " << names[static_cast<unsigned>(result)] << '\n';
      } else if (w[0] == "spawn" && w.size() == 5) {
        const auto owner = byte(w[1]), kind = byte(w[2]), x = byte(w[3]),
                   z = byte(w[4]);
        if (owner >= state().accounts().size() || x >= 24 || z >= 24)
          throw std::invalid_argument("invalid spawn request");
        const auto result = scene ? scene->spawn_reserved_ground_unit(
                                        owner, kind,
                                        {static_cast<std::uint8_t>(x),
                                         static_cast<std::uint8_t>(z)})
                                  : simulation->spawn_reserved_ground_unit(
                                        owner, kind, {x + .5f, 0, z + .5f, 0});
        constexpr const char *names[] = {"spawned", "no-order", "invalid",
                                         "capacity-rejected",
                                         "allocation-failed"};
        std::cout << "spawn " << names[static_cast<unsigned>(result.status)];
        if (result.entity_index)
          std::cout << " entity=" << *result.entity_index;
        std::cout << '\n';
        if (result.status == ArenaUnitSpawnStatus::Invalid)
          throw std::invalid_argument("unsupported spawn request");
      } else if(w[0]=="deploy" && w.size()==4){
        const auto owner=number<unsigned>(w[1]);const auto kind=byte(w[2]);
        const ArenaGroundDeploymentControl control{static_cast<std::uint8_t>(kind),number<float>(w[3])};
        if(!(scene?scene->set_ground_deployment(owner,control):simulation->set_ground_deployment(owner,control)))
          throw std::invalid_argument("invalid ground deployment control");
        std::cout<<"deploy owner="<<owner<<" kind="<<kind<<" strength="<<control.strength<<'\n';
      } else if (w[0] == "health-delta" && w.size()==3) {
        const auto index=number<unsigned>(w[1]);const float delta=number<float>(w[2]);
        if(index>65535 || !(scene?scene->queue_ground_health_delta(static_cast<std::uint16_t>(index),delta):
            simulation->queue_ground_health_delta(static_cast<std::uint16_t>(index),delta)))
          throw std::invalid_argument("health delta requires a live ground unit and finite sum");
        std::cout<<"health-delta queued entity="<<index<<" delta="<<delta<<'\n';
      } else if (w[0] == "expect-unit-health" && w.size()==3) {
        const auto index=number<unsigned>(w[1]);const auto expected=number<float>(w[2]);
        const auto& bank=state().storage().banks[state().storage().read_bank];
        if(index>=bank.size() || !std::isfinite(expected) ||
           !(bank[index][15]==40 || bank[index][15]==41 || (bank[index][15]>=43 && bank[index][15]<=47) || bank[index][15]==62))
          throw std::invalid_argument("invalid health expectation");
        unsigned word=0;for(unsigned b=0;b<4;++b)word=(word<<8)|bank[index][0x38+b];
        if(std::bit_cast<float>(word)!=expected)throw std::runtime_error("unit health expectation failed");
        std::cout<<"expect-unit-health passed\n";
      } else if (w[0] == "expect-capture-ready" && w.size()==3) {
        const auto index=number<unsigned>(w[1]),ready=number<unsigned>(w[2]);
        const auto& bank=state().storage().banks[state().storage().read_bank];
        if(index>=bank.size() || ready>1 || bank[index][15]!=45)
          throw std::invalid_argument("invalid converter readiness expectation");
        if(bank[index][0xbb]!=ready)throw std::runtime_error("converter readiness expectation failed");
        std::cout<<"expect-capture-ready passed\n";
      } else if (w[0] == "capture-ready" && w.size()==3) {
        const auto index=number<unsigned>(w[1]),ready=number<unsigned>(w[2]);
        if(index>65535||ready>1)throw std::invalid_argument("invalid capture readiness");
        const bool changed=scene?scene->set_ground_capture_ready(static_cast<std::uint16_t>(index),ready!=0):
            simulation->set_ground_capture_ready(static_cast<std::uint16_t>(index),ready!=0);
        if(!changed)throw std::invalid_argument("readiness requires a live kind-45 unit");
        std::cout<<"capture-ready "<<index<<' '<<ready<<'\n';
      } else if (w[0] == "expect-base-counter" && w.size()==3) {
        const auto owner=number<unsigned>(w[1]),expected=number<unsigned>(w[2]);
        const auto actual=state().base_counter(owner);
        if(!actual || expected>32767)throw std::invalid_argument("invalid base counter expectation");
        if(static_cast<unsigned>(*actual)!=expected)throw std::runtime_error("base counter expectation failed");
        std::cout<<"expect-base-counter passed\n";
      } else if (w[0] == "expect-unit-owner" && w.size()==3) {
        const auto index=number<unsigned>(w[1]),owner=number<unsigned>(w[2]);
        const auto &bank=state().storage().banks[state().storage().read_bank];
        if(index>=bank.size()||owner>=state().accounts().size()||bank[index][15]<40||bank[index][15]>48)
          throw std::invalid_argument("invalid unit owner expectation");
        if((bank[index][0x33]>>5)!=owner)throw std::runtime_error("unit owner expectation failed");
        std::cout<<"expect-unit-owner passed\n";
      } else if (w[0] == "contacts" && w.size()==2) {
        const auto flag=number<unsigned>(w[1]);
        if(flag>1)throw std::invalid_argument("invalid contacts flag");
        if(scene)scene->set_ground_contacts(flag!=0);else simulation->set_ground_contacts(flag!=0);
        std::cout<<"contacts "<<flag<<'\n';
      } else if (w[0] == "dynamics" && w.size() == 2) {
        const auto flag = number<unsigned>(w[1]);
        if (flag > 1) throw std::invalid_argument("invalid dynamics flag");
        if (scene) scene->set_ground_dynamics(flag != 0);
        else simulation->set_ground_dynamics(flag != 0);
        std::cout << "dynamics " << flag << '\n';
      } else if (w[0] == "transport" && w.size() == 2) {
        const auto flag = number<unsigned>(w[1]);
        if (flag > 1) throw std::invalid_argument("invalid transport flag");
        if (scene) scene->set_ground_transport(flag != 0);
        else simulation->set_ground_transport(flag != 0);
        std::cout << "transport " << flag << '\n';
      } else if (w[0] == "expect-unit-separated" && w.size()==4) {
        const auto first=number<unsigned>(w[1]),second=number<unsigned>(w[2]);
        const double minimum=number<double>(w[3]);const auto &storage=state().storage();
        const auto &bank=storage.banks[storage.read_bank];
        if(first==second||first>=bank.size()||second>=bank.size()||!std::isfinite(minimum)||minimum<0)
          throw std::invalid_argument("invalid separation expectation");
        auto coordinate=[](const ArenaEntitySlot &s,unsigned offset){unsigned bits=0;for(unsigned i=0;i<4;++i)bits=(bits<<8)|s[offset+i];return std::bit_cast<float>(bits);};
        double squared=0;
        for(auto index:{first,second})if(bank[index][15]<40||bank[index][15]>48)
          throw std::invalid_argument("separation requires live units");
        for(unsigned axis=0;axis<3;++axis){const double delta=double(coordinate(bank[first],4*axis))-coordinate(bank[second],4*axis);squared+=delta*delta;}
        if(!std::isfinite(squared)||std::sqrt(squared)<minimum)throw std::runtime_error("unit separation expectation failed");
        std::cout<<"expect-unit-separated passed\n";
      } else if (w[0] == "expect-unit-near" && w.size() == 5) {
        const auto index=number<unsigned>(w[1]);
        const float x=number<float>(w[2]),z=number<float>(w[3]),tolerance=number<float>(w[4]);
        const auto &storage=state().storage();
        if(index>=storage.banks[storage.read_bank].size() || !std::isfinite(x) ||
           !std::isfinite(z) || !std::isfinite(tolerance) || tolerance<0)
          throw std::invalid_argument("invalid unit position expectation");
        const auto &slot=storage.banks[storage.read_bank][index];
        auto scalar=[&](unsigned off){unsigned word=0;for(unsigned i=0;i<4;++i)word=(word<<8)|slot[off+i];return std::bit_cast<float>(word);};
        const float actual_x=scalar(0),actual_z=scalar(8);
        if(slot[15]<40 || slot[15]>48 || !std::isfinite(actual_x) || !std::isfinite(actual_z) ||
           std::abs(actual_x-x)>tolerance || std::abs(actual_z-z)>tolerance)
          throw std::runtime_error("unit position expectation failed");
        std::cout << "expect-unit-near passed x=" << actual_x << " z=" << actual_z << '\n';
      } else if (w[0] == "expect-unit-cell" && w.size() == 4) {
        const auto index = number<unsigned>(w[1]), x = number<unsigned>(w[2]), z = number<unsigned>(w[3]);
        const auto &storage = state().storage();
        if (index >= storage.banks[0].size() || x >= storage.width || z >= storage.height)
          throw std::invalid_argument("invalid unit cell expectation");
        const auto &slot = storage.banks[storage.read_bank][index];
        if ((slot[0xc8]*256u+slot[0xc9])!=x || (slot[0xca]*256u+slot[0xcb])!=z || slot[15]<40 || slot[15]>48)
          throw std::runtime_error("unit cell expectation failed");
        for (const auto &bank : storage.cells) {
          unsigned count=0;
          for (const auto &entry : bank[z*24+x])
            count += ((unsigned(entry[0])<<8)|entry[1])==index;
          if (count!=1) throw std::runtime_error("unit cell membership failed");
        }
        std::cout << "expect-unit-cell passed\n";
      } else if (w[0] == "expect-live-unit" && w.size() == 4) {
        const auto index = number<unsigned>(w[1]), kind = byte(w[2]),
                   target = byte(w[3]);
        const auto &bank = state().storage().banks[state().storage().read_bank];
        if (index >= bank.size() || bank[index][15] != kind ||
            ((bank[index][0x33] >> 2) & 7) != 5 || bank[index][0x7e] != target)
          throw std::runtime_error("live unit expectation failed");
        std::cout << "expect-live-unit passed\n";
      } else if (w[0] == "factory" && w.size() == 3) {
        const auto index = number<unsigned>(w[1]);
        if (index > 65535)
          throw std::invalid_argument("factory index out of range");
        auto result =
            scene ? scene->select_factory(static_cast<std::uint16_t>(index),
                                          byte(w[2]))
                  : simulation->select_factory(
                        static_cast<std::uint16_t>(index), byte(w[2]));
        constexpr const char *names[] = {"changed", "unchanged", "unsupported",
                                         "no-target", "invalid"};
        std::cout << "factory " << names[static_cast<unsigned>(result)] << '\n';
        if (result != ArenaFactorySelection::Changed &&
            result != ArenaFactorySelection::Unchanged)
          throw std::runtime_error("factory selection failed");
      } else if (w[0] == "expect-factory-ground-unit" && w.size() == 4) {
        const auto index = number<unsigned>(w[1]), target = byte(w[2]);
        if (index > 65535)
          throw std::invalid_argument("invalid factory index");
        constexpr const char *names[] = {"unsupported", "no-target", "blocked",
                                         "bound"};
        const auto unit = state().factory_ground_unit_template(index);
        if (!unit || unit->slot[0x7e] != target ||
            w[3] != names[static_cast<unsigned>(unit->binding)])
          throw std::runtime_error("ground unit expectation failed");
        std::cout << "expect-factory-ground-unit passed\n";
      } else if (w[0] == "expect-factory-unit" && w.size() == 4) {
        const auto index = number<unsigned>(w[1]),
                   kind = number<unsigned>(w[2]),
                   model = number<unsigned>(w[3]);
        if (index > 65535 || kind < 40 || kind > 48 || model > 39)
          throw std::invalid_argument("invalid factory unit expectation");
        const auto unit = state().factory_unit_template(index);
        if (!unit || (*unit)[15] != kind || (*unit)[0x6f] != model)
          throw std::runtime_error("factory unit template expectation failed");
        std::cout << "expect-factory-unit passed\n";
      } else if (w[0] == "expect-unit" && w.size() == 4) {
        const auto owner = number<unsigned>(w[1]),
                   kind = number<unsigned>(w[2]),
                   expected = number<unsigned>(w[3]);
        if (owner >= state().accounts().size() || kind < 40 || kind > 48)
          throw std::invalid_argument("invalid unit expectation");
        if (state().unit_quotas()[owner].by_type[kind - 40] != expected)
          throw std::runtime_error("unit reservation expectation failed");
        std::cout << "expect-unit passed\n";
      } else if (w[0] == "expect-mode" && w.size() == 3) {
        const auto index = number<unsigned>(w[1]);
        const auto mode = number<unsigned>(w[2]);
        const auto &bank = state().storage().banks[state().storage().read_bank];
        if (index >= bank.size() ||
            (bank[index][15] != 26 && bank[index][15] != 28) ||
            bank[index][0xaa] != mode)
          throw std::runtime_error("factory mode expectation failed");
        std::cout << "expect-mode passed\n";
      } else if (w[0] == "step" && w.size() == 1)
        completion(step());
      else if (w[0] == "drain" && w.size() == 1) {
        while (state().pending())
          completion(step());
      } else if (w[0] == "event" && w.size() == 7) {
        auto owner = byte(w[1]);
        ArenaEvent e{number<float>(w[2]), static_cast<std::uint8_t>(byte(w[5])),
                     static_cast<std::uint8_t>(byte(w[6])),
                     static_cast<std::uint8_t>(byte(w[3])),
                     static_cast<std::uint8_t>(byte(w[4]))};
        auto result = scene ? scene->enqueue_event(owner, e)
                            : simulation->enqueue_event(owner, e);
        if (result == ArenaTimedSubmit::Invalid)
          throw std::invalid_argument("invalid event");
        std::cout << "event "
                  << (result == ArenaTimedSubmit::Scheduled ? "scheduled"
                                                            : "full")
                  << '\n';
      } else if (w[0] == "advance" && w.size() == 2) {
        auto tick = scene ? scene->advance(number<float>(w[1]))
                          : simulation->advance(number<float>(w[1]));
        constexpr const char *names[] = {"idle", "submitted", "retried",
                                         "unsupported", "backpressure"};
        std::cout << "clock " << state().clock();
        for (unsigned i = 0; i < state().players().size(); ++i)
          std::cout << " player" << i << '='
                    << names[static_cast<unsigned>(tick.players[i])];
        std::cout << '\n';
        completion(tick.construction);
        for(const auto &attempt:tick.ground_deployments){
          constexpr const char *status[]={"spawned","no-order","invalid","capacity-rejected","allocation-failed"};
          std::cout<<"base-deployment "<<status[static_cast<unsigned>(attempt.result.status)]<<" owner="<<unsigned(attempt.owner)<<" kind="<<unsigned(attempt.kind);
          if(attempt.result.entity_index)std::cout<<" entity="<<*attempt.result.entity_index;
          std::cout<<'\n';
        }
        for(auto index:tick.spawned_beams)std::cout<<"tesla-beam entity="<<index<<'\n';
        for(auto index:tick.ground_slams)std::cout<<"ground-slam entity="<<index<<'\n';
        for(auto index:tick.depleted_units)std::cout<<"ground-depleted entity="<<index<<'\n';
        for(auto index:tick.retired_entities)std::cout<<"entity-retired entity="<<index<<'\n';
        for(const auto &impact:tick.base_impacts)
          std::cout<<"base-impact entity="<<impact.entity_index<<" base="<<unsigned(impact.base)<<" owner="<<unsigned(impact.owner)<<'\n';
        for (auto index : tick.transported_units)
          std::cout << "unit-transport entity=" << index << '\n';
        for (auto index : tick.production_reservations)
          std::cout << "unit-production reserved factory=" << index << '\n';
        for (auto index : tick.factory_selections)
          std::cout << "factory selected entity=" << index << '\n';
        for (auto index : tick.ready_entities)
          std::cout << "ready entity=" << index << '\n';
      } else if (w[0] == "expect" && w.size() == 3) {
        const auto expected = number<unsigned>(w[2]);
        std::size_t actual;
        if(w[1]=="tesla-beams"){
          actual=0;for(const auto &slot:state().storage().banks[state().storage().read_bank])actual+=slot[15]==5;
        }else if (w[1] == "allocated")
          actual = config.capacity - state().storage().free_indices.size();
        else if(w[1]=="damaged-ground-units"){
          actual=0;
          for(const auto &slot:state().storage().banks[state().storage().read_bank]){
            if(!(slot[15]==40 || slot[15]==41 || (slot[15]>=43 && slot[15]<=47)))continue;
            auto value=[&](unsigned offset){unsigned word=0;for(unsigned b=0;b<4;++b)word=(word<<8)|slot[offset+b];return std::bit_cast<float>(word);};
            actual+=value(0x38)<value(0x64);
          }
        }else if(w[1]=="corpses"){
          actual=0;for(const auto &slot:state().storage().banks[state().storage().read_bank])actual+=slot[15]==62;
        } else if (w[1] == "live-units") {
          actual = 0;
          for (const auto &slot :
               state().storage().banks[state().storage().read_bank])
            actual += slot[15] >= 40 && slot[15] <= 48;
        } else if (w[1].starts_with("unit-active")) {
          const auto owner = number<unsigned>(w[1].substr(11));
          if (owner >= state().accounts().size())
            throw std::invalid_argument("invalid unit expectation player");
          actual = state().unit_quotas()[owner].active;
        } else if (w[1] == "ready") {
          actual = 0;
          for (const auto &slot :
               state().storage().banks[state().storage().read_bank])
            actual += arena_structure_ready(slot);
        } else if (w[1] == "pending")
          actual = state().pending();
        else if (w[1] == "display" && scene)
          actual = scene->arena().entities.size();
        else if (w[1].starts_with("unit-pending")) {
          const auto owner = number<unsigned>(w[1].substr(12));
          if (owner >= state().accounts().size())
            throw std::invalid_argument("invalid unit expectation player");
          actual = state().unit_quotas()[owner].pending;
        } else if (w[1].starts_with("events")) {
          auto owner = number<unsigned>(w[1].substr(6));
          if (owner >= state().players().size())
            throw std::invalid_argument("invalid expectation player");
          actual = state().players()[owner].events.size();
        } else if (w[1].starts_with("resource")) {
          auto owner = number<unsigned>(w[1].substr(8));
          if (owner >= state().accounts().size())
            throw std::invalid_argument("invalid expectation player");
          actual = expected;
          if (state().accounts()[owner].resource !=
              static_cast<float>(expected))
            throw std::runtime_error("resource expectation failed");
        } else
          throw std::invalid_argument("unknown expectation");
        if (actual != expected)
          throw std::runtime_error("expectation failed: " + w[1] +
                                   " expected " + w[2] + " actual " +
                                   std::to_string(actual));
        std::cout << "expect " << w[1] << " passed\n";
      } else
        throw std::invalid_argument("invalid scenario command");
    }
    std::cout << "allocated "
              << config.capacity - state().storage().free_indices.size()
              << " pending " << state().pending() << " clock "
              << state().clock() << '\n';
    for (unsigned i = 0; i < config.players.size(); ++i)
      std::cout << "player " << i << " resource "
                << state().accounts()[i].resource << " events "
                << state().players()[i].events.size() << '\n';
  } catch (const std::exception &e) {
    std::cerr << "line " << line << ": " << e.what() << '\n';
    return 1;
  }
}
