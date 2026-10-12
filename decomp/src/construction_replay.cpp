#include "comet/arena_construction_config.hpp"
#include "comet/native_construction_scene.hpp"
#include "comet/native_session.hpp"
#include <charconv>
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
        for (auto index : tick.production_reservations)
          std::cout << "unit-production reserved factory=" << index << '\n';
        for (auto index : tick.factory_selections)
          std::cout << "factory selected entity=" << index << '\n';
        for (auto index : tick.ready_entities)
          std::cout << "ready entity=" << index << '\n';
      } else if (w[0] == "expect" && w.size() == 3) {
        const auto expected = number<unsigned>(w[2]);
        std::size_t actual;
        if (w[1] == "allocated")
          actual = config.capacity - state().storage().free_indices.size();
        else if (w[1] == "ready") {
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
