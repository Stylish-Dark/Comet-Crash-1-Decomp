#include "comet/native_mission_playback.hpp"
#include <utility>

namespace comet::decomp {
NativeMissionPlayback::NativeMissionPlayback(NativeConstructionScene& scene)
    : scene_(scene), initial_(std::make_unique<NativeConstructionScene>(scene)) {
  synchronize();
}
bool NativeMissionPlayback::finished() const {
  // A defeated perspective alone does not end a match with >2 teams.
  for (std::size_t owner=0; owner<scene_.simulation().accounts().size(); ++owner) {
    const auto outcome = evaluate_native_mission(scene_.simulation(),owner);
    if (outcome==NativeMissionOutcome::Victory || outcome==NativeMissionOutcome::Draw)
      return true;
  }
  return false;
}
void NativeMissionPlayback::synchronize() {
  if (!finished()) return;
  pause();
  for (std::size_t owner=0; owner<scene_.simulation().accounts().size(); ++owner) {
    const auto control = scene_.simulation().ground_deployment()[owner];
    if (control) scene_.set_ground_deployment(owner,{control->kind,0});
  }
}
void NativeMissionPlayback::toggle_playing() {
  synchronize();
  if (!finished()) playing_ = !playing_;
}
void NativeMissionPlayback::advance(float elapsed) {
  synchronize();
  if (finished()) return;
  scene_.advance(scene_.simulation().clock()+elapsed);
  synchronize();
}
void NativeMissionPlayback::restart() {
  // Copy before mutation. Allocation failure leaves the current match intact;
  // heap storage also avoids placing the large bank/cell state on the stack.
  auto restored = std::make_unique<NativeConstructionScene>(*initial_);
  scene_ = std::move(*restored);
  pause();
  synchronize();
}
} // namespace comet::decomp
