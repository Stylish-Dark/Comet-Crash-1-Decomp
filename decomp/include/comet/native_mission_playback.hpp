#pragma once
#include "comet/native_construction_scene.hpp"
#include "comet/native_mission_outcome.hpp"
#include <memory>

namespace comet::decomp {
// Native preview lifecycle, not recovered original campaign rules. The launch
// checkpoint includes both simulation banks, RNG, queues and scene projection.
class NativeMissionPlayback {
  NativeConstructionScene& scene_;
  std::unique_ptr<const NativeConstructionScene> initial_;
  bool playing_ = false;
public:
  explicit NativeMissionPlayback(NativeConstructionScene&);
  bool finished() const;
  bool playing() const { return playing_; }
  void pause() { playing_ = false; }
  void toggle_playing();
  void synchronize();
  void advance(float elapsed);
  // Restores the state supplied at controller creation; always returns paused.
  void restart();
};
} // namespace comet::decomp
