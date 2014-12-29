#ifndef GOOPY_PLUGINS_RIME_RIME_KEY_EVENT_H_
#define GOOPY_PLUGINS_RIME_RIME_KEY_EVENT_H_

namespace ipc {
namespace proto {
class KeyEvent;
}  // namespace proto
}  // namespace ipc

namespace plugins {
namespace rime {

struct RimeKeyEvent {
  int keycode;
  int modifiers;

  RimeKeyEvent() : keycode(0), modifiers(0) {}
  RimeKeyEvent(const ipc::proto::KeyEvent& key);
};

}  // namespace rime
}  // namespace plugins

#endif  // GOOPY_PLUGINS_RIME_RIME_KEY_EVENT_H_
