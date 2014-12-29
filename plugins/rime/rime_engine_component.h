// Copyleft RIME Developers.
// Author: Chen Gong <chen.sst@gmail.com>

// This file defines an IME component class which wraps
// Rime Input Method Engine.

#ifndef GOOPY_PLUGINS_RIME_RIME_ENGINE_COMPONENT_H_
#define GOOPY_PLUGINS_RIME_RIME_ENGINE_COMPONENT_H_

#include "rime_api.h"

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "ipc/component_base.h"

namespace ipc {
namespace proto {
class ComponentInfo;
class Message;
}  // namespace proto
}  // namespace ipc

namespace plugins {
namespace rime {

static const char kRimeEngineComponentStringId[] =
    "io.github.rimeime.rime_engine";

class RimeApiWrapper : public RimeApi {
 public:
  ~RimeApiWrapper();
  static shared_ptr<RimeApiWrapper> GetInstance();

 private:
  RimeApiWrapper();
  HMODULE handle_;
  static weak_ptr<RimeApiWrapper> instance_;
};

class RimeEngineComponent : public ipc::ComponentBase {
 public:
  RimeEngineComponent(shared_ptr<RimeApiWrapper> api = nullptr);
  virtual ~RimeEngineComponent();

  // Overridden interface ipc::Component.
  virtual void GetInfo(ipc::proto::ComponentInfo* info) OVERRIDE;
  virtual void Handle(ipc::proto::Message* message) OVERRIDE;

 private:
  // Handles input context related messages.
  void OnMsgAttachToInputContext(ipc::proto::Message* message);
  void OnMsgComponentActivated(ipc::proto::Message* message);
  void OnMsgComponentDeactivated(ipc::proto::Message* message);
  void OnMsgInputContextGotFocus(ipc::proto::Message* message);
  void OnMsgInputContextLostFocus(ipc::proto::Message* message);
  void OnMsgProcessKey(ipc::proto::Message* message);
  void OnMsgSelectCandidate(ipc::proto::Message* message);
  void OnMsgCandidateListPageDown(ipc::proto::Message* message);
  void OnMsgCandidateListPageUp(ipc::proto::Message* message);
  void OnMsgDoCommand(ipc::proto::Message* message);
  void Update(uint32 icid);
  void Clear(uint32 icid);
  void CommitText(uint32 icid);
  void UpdateContext(uint32 icid);
  void UpdateStatus(uint32 icid);
  bool SetComposition(const RimeContext& context, uint32 icid);
  bool SetCandidateList(const RimeContext& context, uint32 icid);
  bool SetStatus(const RimeStatus& status, uint32 icid);

  shared_ptr<RimeApiWrapper> rime_;
  RimeSessionId session_;
  int current_icid_;
  RimeStatus current_status_;

  DISALLOW_COPY_AND_ASSIGN(RimeEngineComponent);
};

}  // namespace rime
}  // namespace plugins

#endif  // GOOPY_PLUGINS_RIME_RIME_ENGINE_COMPONENT_H_
