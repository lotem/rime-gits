#ifndef RIMEIME_RIME_KEY_EVENT_H_
#define RIMEIME_RIME_KEY_EVENT_H_

namespace ipc {
namespace proto {
class KeyEvent;
}  // namespace proto
}  // namespace ipc

namespace rimeime {

struct RimeKeyEvent {
  int keycode;
  int modifiers;

  RimeKeyEvent() : keycode(0), modifiers(0) {}
  RimeKeyEvent(const ipc::proto::KeyEvent& key);
};

}  // namespace rimeime

#endif  // RIMEIME_RIME_KEY_EVENT_H_
