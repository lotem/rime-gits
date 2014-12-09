// Copyleft RIME Developers.
// Author: Chen Gong <chen.sst@gmail.com>

#include "base/logging.h"
#include "base/scoped_ptr.h"
#include "components/plugin_wrapper/plugin_definition.h"
#include "rimeime/rime_engine_component.h"

using rimeime::RimeApiWrapper;
using rimeime::RimeEngineComponent;

int GetAvailableComponentInfos(ipc::proto::MessagePayload* payload) {
  DCHECK(payload);
  payload->Clear();
  scoped_ptr<RimeEngineComponent> component(new RimeEngineComponent);
  component->GetInfo(payload->add_component_info());
  return payload->component_info_size();
}

ipc::Component* CreateComponent(const char* id) {
  std::string string_id = id;
  if (string_id == rimeime::kRimeEngineComponentStringId) {
    return new RimeEngineComponent(RimeApiWrapper::GetInstance());
  }
  return NULL;
}
