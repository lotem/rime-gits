// Copyright RIME Developers

#include "plugins/rime/rime_engine_component.h"

#include <string>

#include "base/logging.h"
#include "base/scoped_ptr.h"
#include "base/stl_util.h"
#include "base/string_utils_win.h"
#include "common/app_utils.h"
#include "components/common/constants.h"
#include "components/common/file_utils.h"
#include "components/common/keystroke_util.h"
#include "ipc/constants.h"
#include "ipc/message_types.h"
#include "ipc/protos/ipc.pb.h"
#include "skin/skin_consts.h"

#include "plugins/rime/keysym.h"
#include "plugins/rime/rime_api.h"
#include "plugins/rime/rime_key_event.h"

using ime_goopy::AppUtils;
using ime_goopy::FileUtils;
using ime_goopy::Utf8ToWide;
using ime_goopy::WideToUtf8;

namespace {

// The message types Rime engine can consume.
const uint32 kConsumeMessages[] = {
  // Input context related messages.
  ipc::MSG_ATTACH_TO_INPUT_CONTEXT,
  ipc::MSG_COMPONENT_ACTIVATED,
  ipc::MSG_COMPONENT_DEACTIVATED,
  ipc::MSG_INPUT_CONTEXT_GOT_FOCUS,
  ipc::MSG_INPUT_CONTEXT_LOST_FOCUS,
  ipc::MSG_PROCESS_KEY_EVENT,
  // Composition related messages.
  ipc::MSG_CANCEL_COMPOSITION,
  ipc::MSG_COMPLETE_COMPOSITION,
  ipc::MSG_CANDIDATE_LIST_PAGE_DOWN,
  ipc::MSG_CANDIDATE_LIST_PAGE_UP,
  ipc::MSG_SELECT_CANDIDATE,
  // Status bar and context menu messages.
  ipc::MSG_DO_COMMAND,
};

const uint32 kProduceMessages[] = {
  ipc::MSG_INSERT_TEXT,
  ipc::MSG_REQUEST_CONSUMER,
  ipc::MSG_SET_CANDIDATE_LIST,
  ipc::MSG_SET_COMMAND_LIST,
  ipc::MSG_SET_COMPOSITION,
  ipc::MSG_SET_SELECTED_CANDIDATE,
  ipc::MSG_UPDATE_COMMANDS,
};

static const char* kRimeLanguages[] = {
  "zh-CN",
  "zh-HK",
  "zh-TW",
  NULL
};
static const char kRimeIcon[]  = "rime.png";
static const char kRimeOverIcon[]  = "rime_over.png";
static const char kResourcePackPathPattern[] = "/rime_[LANG].pak";

static const int kDefaultFontSize = 14;

static const char kTextFormat[] = "text";

static const int kTitleTextIndex = 0;
static const int kNormalStateIconIndex = 1;
static const int kDisabledStateIconIndex = 2;
static const int kDownStateIconIndex = 3;
static const int kOverStateIconIndex = 4;
static const int kStateIndices = 5;

static const int kChineseEnglishModeIndex = 0;
static const int kFullHalfWidthCharacterModeIndex = 2;
static const int kFullHalfWidthPunctuationModeIndex = 4;

static const char* kStateLabels[][kStateIndices] = {
  {
    "Chinese",
    ime_goopy::skin::kChineseModeIcon,
    ime_goopy::skin::kChineseModeDisabledIcon,
    ime_goopy::skin::kChineseModeDownIcon,
    ime_goopy::skin::kChineseModeOverIcon,
  },
  {
    "Abc",
    ime_goopy::skin::kEnglishModeIcon,
    ime_goopy::skin::kEnglishModeDisabledIcon,
    ime_goopy::skin::kEnglishModeDownIcon,
    ime_goopy::skin::kEnglishModeOverIcon,
  },
  {
    "Full-width char",
    ime_goopy::skin::kFullWidthCharacterModeIcon,
    ime_goopy::skin::kFullWidthCharacterModeDisabledIcon,
    ime_goopy::skin::kFullWidthCharacterModeDownIcon,
    ime_goopy::skin::kFullWidthCharacterModeOverIcon,
  },
  {
    "Half-width char",
    ime_goopy::skin::kHalfWidthCharacterModeIcon,
    ime_goopy::skin::kHalfWidthCharacterModeDisabledIcon,
    ime_goopy::skin::kHalfWidthCharacterModeDownIcon,
    ime_goopy::skin::kHalfWidthCharacterModeOverIcon,
  },
  {
    "Full-width punct",
    ime_goopy::skin::kFullWidthPunctuationModeIcon,
    ime_goopy::skin::kFullWidthPunctuationModeDisabledIcon,
    ime_goopy::skin::kFullWidthPunctuationModeDownIcon,
    ime_goopy::skin::kFullWidthPunctuationModeOverIcon,
  },
  {
    "Half-width punct",
    ime_goopy::skin::kHalfWidthPunctuationModeIcon,
    ime_goopy::skin::kHalfWidthPunctuationModeDisabledIcon,
    ime_goopy::skin::kHalfWidthPunctuationModeDownIcon,
    ime_goopy::skin::kHalfWidthPunctuationModeOverIcon,
  },
};

static const char kRimeAsciiModeOption[] = "ascii_mode";
static const char kRimeFullShapeOption[] = "full_shape";
static const char kRimeAsciiPunctuationOption[] = "ascii_punct";

typedef RimeApi* (*RimeApiProc)();

}  // namespace

namespace plugins {
namespace rime {

using ipc::proto::Message;

weak_ptr<RimeApiWrapper> RimeApiWrapper::instance_;

RimeApiWrapper::RimeApiWrapper() {
  RimeApi* api = (RimeApi*)this;
  RIME_STRUCT_INIT(RimeApi, *api);
  RIME_STRUCT_CLEAR(*api);

  // Load rime.dll and dependencies from plugin path.
  std::wstring env_path;
  wchar_t path[MAX_PATH] = {0};
  if (GetEnvironmentVariable(L"PATH", path, MAX_PATH)) {
    env_path = path;
    env_path += L";";
  }
  env_path += Utf8ToWide(FileUtils::GetSystemPluginPath());
  SetEnvironmentVariable(L"PATH", env_path.c_str());
  handle_ = ::LoadLibrary(L"rime.dll");
  if (!handle_) {
    return;
  }
  RimeApiProc proc = (RimeApiProc)::GetProcAddress(handle_, "rime_get_api");
  if (!proc) {
    return;
  }
  *api = *proc();

  // opencc config uses relative paths "Rime/opencc/..."
  SetCurrentDirectory(AppUtils::GetSystemDataFilePath(L"").c_str());
  std::string shared_data_dir(WideToUtf8(AppUtils::GetSystemDataFilePath(L"Rime")));
  std::string user_data_dir(WideToUtf8(AppUtils::GetUserDataFilePath(L"Rime")));
  RIME_STRUCT(RimeTraits, traits);
  traits.shared_data_dir = shared_data_dir.c_str();
  traits.user_data_dir = user_data_dir.c_str();
  traits.distribution_name = "Rime with Google Input Tools";
  traits.distribution_code_name = "rime-gits";
  traits.distribution_version = "0.9";
  traits.app_name = "gits";
  setup(&traits);
  initialize(&traits);
  start_maintenance(/*full_check = */False);
}

RimeApiWrapper::~RimeApiWrapper() {
  if (finalize) {
    finalize();
  }
  if (handle_) {
    ::FreeLibrary(handle_);
    handle_ = NULL;
  }
}

shared_ptr<RimeApiWrapper> RimeApiWrapper::GetInstance() {
  shared_ptr<RimeApiWrapper> api = instance_.lock();
  if (!api) {
    api.reset(new RimeApiWrapper);
    instance_ = api;
  }
  return api;
}

RimeEngineComponent::RimeEngineComponent(shared_ptr<RimeApiWrapper> api)
    : rime_(api), session_(0), current_icid_(0) {
  if (rime_) {
    session_ = rime_->create_session();
  }
  RIME_STRUCT_INIT(RimeStatus, current_status_);
  RIME_STRUCT_CLEAR(current_status_);
  current_status_.is_disabled = true;
}

RimeEngineComponent::~RimeEngineComponent() {
  if (rime_) {
    rime_->destroy_session(session_);
    session_ = 0;
  }
}

void RimeEngineComponent::GetInfo(ipc::proto::ComponentInfo* info) {
  info->set_string_id(kRimeEngineComponentStringId);
  for (const char** language = kRimeLanguages; *language; ++language) {
    info->add_language(*language);
  }
  for (int i = 0; i < arraysize(kConsumeMessages); ++i)
    info->add_consume_message(kConsumeMessages[i]);
  for (int i = 0; i < arraysize(kProduceMessages); ++i)
    info->add_produce_message(kProduceMessages[i]);
  GetSubComponentsInfo(info);
  std::string dir = FileUtils::GetDataPathForComponent(
      kRimeEngineComponentStringId);
  std::string filename = dir + "/" + kRimeOverIcon;
  ipc::proto::IconGroup icon;
  if (!FileUtils::ReadFileContent(filename,
                                  icon.mutable_over()->mutable_data())) {
    icon.clear_over();
  }
  filename = dir + "/" + kRimeIcon;
  if (FileUtils::ReadFileContent(filename,
                                 icon.mutable_normal()->mutable_data())) {
    info->mutable_icon()->CopyFrom(icon);
  }
  std::string name = "Rime";
  info->set_name(name.c_str());
}

void RimeEngineComponent::Handle(Message* message) {
  DCHECK(id());
  // This message is consumed by sub components.
  if (HandleMessageBySubComponents(message))
    return;
  scoped_ptr<Message> mptr(message);

  switch (mptr->type()) {
    case ipc::MSG_ATTACH_TO_INPUT_CONTEXT:
      OnMsgAttachToInputContext(mptr.release());
      break;
    case ipc::MSG_COMPONENT_ACTIVATED:
      OnMsgComponentActivated(mptr.release());
      break;
    case ipc::MSG_COMPONENT_DEACTIVATED:
      OnMsgComponentDeactivated(mptr.release());
      break;
    case ipc::MSG_INPUT_CONTEXT_GOT_FOCUS:
      OnMsgInputContextGotFocus(mptr.release());
      break;
    case ipc::MSG_INPUT_CONTEXT_LOST_FOCUS:
      OnMsgInputContextLostFocus(mptr.release());
      break;
    case ipc::MSG_PROCESS_KEY_EVENT:
      OnMsgProcessKey(mptr.release());
      break;
    case ipc::MSG_SELECT_CANDIDATE:
      OnMsgSelectCandidate(mptr.release());
      break;
    case ipc::MSG_CANDIDATE_LIST_PAGE_DOWN:
      OnMsgCandidateListPageDown(mptr.release());
      break;
    case ipc::MSG_CANDIDATE_LIST_PAGE_UP:
      OnMsgCandidateListPageUp(mptr.release());
      break;
    case ipc::MSG_CANCEL_COMPOSITION:
      rime_->clear_composition(session_);
      // ReplyTrue in case the source component uses SendWithReply.
      ReplyTrue(mptr.release());
      break;
    case ipc::MSG_COMPLETE_COMPOSITION:
      rime_->commit_composition(session_);
      ReplyTrue(mptr.release());
      break;
    case ipc::MSG_DO_COMMAND:
      OnMsgDoCommand(mptr.release());
      break;
    default: {
      DLOG(ERROR) << "Unexpected message received:"
                  << "type = " << mptr->type()
                  << "icid = " << mptr->icid();
      ReplyError(mptr.release(), ipc::proto::Error::INVALID_MESSAGE,
                 "unknown type");
      break;
    }
  }
}

void RimeEngineComponent::OnMsgAttachToInputContext(Message* message) {
  scoped_ptr<Message> mptr(message);
  DCHECK_EQ(mptr->reply_mode(), Message::NEED_REPLY);
  uint32 icid = mptr->icid();
  ReplyTrue(mptr.release());

  mptr.reset(NewMessage(ipc::MSG_REQUEST_CONSUMER, icid, true));
  for (int i = 0; i < arraysize(kProduceMessages); ++i) {
    mptr->mutable_payload()->add_uint32(kProduceMessages[i]);
  }

  Message* reply = NULL;
  if (!SendWithReply(mptr.release(), -1, &reply)) {
    DLOG(ERROR) << "SendWithReply failed with type = MSG_REQUEST_CONSUMER"
        << " icid = " << icid;
    return;
  }
  delete reply;
}

void RimeEngineComponent::OnMsgComponentActivated(Message* message) {
  scoped_ptr<Message> mptr(message);
  uint32 icid = mptr->icid();
  ReplyTrue(mptr.release());
}

void RimeEngineComponent::OnMsgComponentDeactivated(Message* message) {
  scoped_ptr<Message> mptr(message);
  uint32 icid = mptr->icid();

  for (size_t i = 0; i < mptr->payload().uint32_size(); ++i) {
    if (mptr->payload().uint32(i) == ipc::MSG_PROCESS_KEY_EVENT) {
      Clear(icid);
    }
  }

  ReplyTrue(mptr.release());
}

void RimeEngineComponent::Clear(uint32 icid) {
  RIME_STRUCT(RimeContext, context);
  SetComposition(context, icid);
  SetCandidateList(context, icid);

  RIME_STRUCT(RimeStatus, status);
  status.is_disabled = true;
  SetStatus(status, icid);
}

void RimeEngineComponent::OnMsgInputContextGotFocus(Message* message) {
  uint32 icid = message->icid();
  ReplyTrue(message);

  UpdateStatus(icid);
}

void RimeEngineComponent::OnMsgInputContextLostFocus(Message* message) {
  ReplyTrue(message);
}

void RimeEngineComponent::OnMsgProcessKey(Message* message) {
  scoped_ptr<Message> mptr(message);
  uint32 icid = mptr->icid();
  if (!mptr->has_payload() || !mptr->payload().has_key_event()) {
    ReplyError(mptr.release(), ipc::proto::Error::INVALID_PAYLOAD,
               "missing key event.");
    return;
  }
  const ipc::proto::KeyEvent& key = mptr->payload().key_event();
  RimeKeyEvent rime_key(key);

  if (!rime_->find_session(session_)) {
    session_ = rime_->create_session();
  }
  Bool handled = rime_->process_key(session_,
                                    rime_key.keycode,
                                    rime_key.modifiers);
  Update(icid);

  if (handled) {
    ReplyTrue(mptr.release());
  }
  else {
    ReplyFalse(mptr.release());
  }
}

void RimeEngineComponent::OnMsgSelectCandidate(Message* message) {
  scoped_ptr<Message> mptr(message);
  uint32 icid = mptr->icid();
  uint32 selected_candidate_index = mptr->payload().uint32(1);
  rime_->select_candidate(session_, selected_candidate_index);
  Update(icid);
  ReplyTrue(mptr.release());
}

void RimeEngineComponent::OnMsgCandidateListPageDown(Message* message) {
  scoped_ptr<Message> mptr(message);
  uint32 icid = mptr->icid();
  rime_->process_key(session_, Page_Down, 0);
  Update(icid);
  ReplyTrue(mptr.release());
}

void RimeEngineComponent::OnMsgCandidateListPageUp(Message* message) {
  scoped_ptr<Message> mptr(message);
  uint32 icid = mptr->icid();
  rime_->process_key(session_, Page_Up, 0);
  Update(icid);
  ReplyTrue(mptr.release());
}

void RimeEngineComponent::OnMsgDoCommand(Message* message) {
  scoped_ptr<Message> mptr(message);
  uint32 icid = mptr->icid();
  const std::string& command = mptr->payload().string(0);
  if (command == ime_goopy::skin::kChineseEnglishModeButton) {
    // Toggle ascii_mode.
    if (!rime_->process_key(session_, Eisu_toggle, 0)) {
      Bool ascii_mode = rime_->get_option(session_, kRimeAsciiModeOption);
      rime_->set_option(session_, kRimeAsciiModeOption, !ascii_mode);
    }
  }
  else if (command == ime_goopy::skin::kFullHalfWidthCharacterModeButton) {
    // Toggle full_shape.
    Bool full_shape = rime_->get_option(session_, kRimeFullShapeOption);
    rime_->set_option(session_, kRimeFullShapeOption, !full_shape);
  }
  else if (command == ime_goopy::skin::kFullHalfWidthPunctuationModeButton) {
    // Toggle ascii_punct.
    Bool ascii_punct = rime_->get_option(session_, kRimeAsciiPunctuationOption);
    rime_->set_option(session_, kRimeAsciiPunctuationOption, !ascii_punct);
  }

  Update(icid);
  ReplyTrue(mptr.release());
}

void RimeEngineComponent::Update(uint32 icid) {
  CommitText(icid);
  UpdateContext(icid);
  UpdateStatus(icid);
}

void RimeEngineComponent::CommitText(uint32 icid) {
  RIME_STRUCT(RimeCommit, commit);
  if (rime_->get_commit(session_, &commit)) {
    scoped_ptr<Message> commit_message(
        NewMessage(ipc::MSG_INSERT_TEXT, icid, false));
    commit_message->mutable_payload()->mutable_composition()->
        mutable_text()->set_text(commit.text);
    if (!Send(commit_message.release(), NULL)) {
      DLOG(ERROR) << "Send message type = MSG_INSERT_TEXT failed";
    }
  }
}

void RimeEngineComponent::UpdateContext(uint32 icid) {
  RIME_STRUCT(RimeContext, context);
  if (rime_->get_context(session_, &context)) {
    if (!SetComposition(context, icid)) {
      DLOG(ERROR) << "Set composition failed, id: " << id();
    }
    if (!SetCandidateList(context, icid)) {
      DLOG(ERROR) << "Set candidate list failed, id: " << id();
    }
  }
}

void RimeEngineComponent::UpdateStatus(uint32 icid) {
  RIME_STRUCT(RimeStatus, status);
  if (!rime_->get_status(session_, &status)) {
    status.is_disabled = true;
  }
  if (!SetStatus(status, icid)) {
    DLOG(ERROR) << "Set status failed, id: " << id();
  }
}

static void set_state_label(ipc::proto::Command* command, int mode) {
  command->mutable_title()->set_text(kStateLabels[mode][kTitleTextIndex]);
  ipc::proto::IconGroup* icon_group = command->mutable_state_icon();
  icon_group->mutable_normal()->set_data(
      kStateLabels[mode][kNormalStateIconIndex]);
  icon_group->mutable_normal()->set_format(kTextFormat);
  icon_group->mutable_disabled()->set_data(
      kStateLabels[mode][kDisabledStateIconIndex]);
  icon_group->mutable_disabled()->set_format(kTextFormat);
  icon_group->mutable_down()->set_data(
      kStateLabels[mode][kDownStateIconIndex]);
  icon_group->mutable_down()->set_format(kTextFormat);
  icon_group->mutable_over()->set_data(
      kStateLabels[mode][kOverStateIconIndex]);
  icon_group->mutable_over()->set_format(kTextFormat);
}

bool RimeEngineComponent::SetStatus(const RimeStatus& status, uint32 icid) {
  if (icid == current_icid_ &&
      status.is_disabled == current_status_.is_disabled &&
      status.is_ascii_mode == current_status_.is_ascii_mode &&
      status.is_full_shape == current_status_.is_full_shape &&
      status.is_ascii_punct == current_status_.is_ascii_punct) {
    return true;
  }
  bool update = !(status.is_disabled || current_status_.is_disabled);
  current_icid_ = icid;
  current_status_ = status;

  int type = update ? ipc::MSG_UPDATE_COMMANDS
                    : ipc::MSG_SET_COMMAND_LIST;
  scoped_ptr<Message> mptr(NewMessage(type, icid, false));
  ipc::proto::CommandList* command_list =
      mptr->mutable_payload()->add_command_list();
  ipc::proto::Command* command = NULL;

  bool enabled = !status.is_disabled;

  // TODO(chen): refer to the definition of switches in schema.
  const bool supports_ascii_mode = enabled;
  const bool supports_full_shape = enabled;
  const bool supports_ascii_punctuation = enabled;

  if (supports_ascii_mode) {
    bool is_english_mode = status.is_ascii_mode;
    command = command_list->add_command();
    command->set_id(ime_goopy::skin::kChineseEnglishModeButton);
    set_state_label(command, kChineseEnglishModeIndex + is_english_mode);
  }

  if (supports_full_shape) {
    bool is_half_width_character = !status.is_full_shape;
    command = command_list->add_command();
    command->set_id(ime_goopy::skin::kFullHalfWidthCharacterModeButton);
    set_state_label(command,
        kFullHalfWidthCharacterModeIndex + is_half_width_character);
  }

  if (supports_ascii_punctuation) {
    bool is_half_width_punctuation = status.is_ascii_punct;
    command = command_list->add_command();
    command->set_id(ime_goopy::skin::kFullHalfWidthPunctuationModeButton);
    set_state_label(command,
        kFullHalfWidthPunctuationModeIndex + is_half_width_punctuation);
  }

  return Send(mptr.release(), NULL);
}

bool RimeEngineComponent::SetComposition(const RimeContext& context,
                                         uint32 icid) {
  scoped_ptr<Message> mptr(
      NewMessage(ipc::MSG_SET_COMPOSITION, icid, false));
  ipc::proto::Composition* ipc_composition =
      mptr->mutable_payload()->mutable_composition();
  ipc::proto::Text* text = ipc_composition->mutable_text();
  if (context.composition.length == 0) {
    text->set_text("");
    return Send(mptr.release(), NULL);
  }
  std::string preedit(context.composition.preedit);
  text->set_text(preedit);
  ipc::proto::Attribute* attribute = text->add_attribute();
  attribute->set_start(0);
  attribute->set_end(Utf8ToWide(preedit).length());
  attribute->set_type(ipc::proto::Attribute::FONT_SIZE);
  attribute->set_float_value(kDefaultFontSize);
  ipc::proto::Range* selection = ipc_composition->mutable_selection();
  int cursor_pos = Utf8ToWide(
          preedit.substr(0, context.composition.cursor_pos)).length();
  selection->set_start(cursor_pos);
  selection->set_end(cursor_pos);
  if (context.commit_text_preview) {
    ipc::proto::Text* inline_text = ipc_composition->mutable_inline_text();
    inline_text->set_text(context.commit_text_preview);
    ipc::proto::Range* inline_selection =
        ipc_composition->mutable_inline_selection();
    inline_selection->set_start(0);
    inline_selection->set_end(Utf8ToWide(context.commit_text_preview).length());
  }
  return Send(mptr.release(), NULL);
}

bool RimeEngineComponent::SetCandidateList(const RimeContext& context,
                                           uint32 icid) {
  scoped_ptr<Message> mptr(
      NewMessage(ipc::MSG_SET_CANDIDATE_LIST, icid, false));
  ipc::proto::CandidateList* candidate_list =
      mptr->mutable_payload()->mutable_candidate_list();
  candidate_list->set_id(0);
  if (context.menu.num_candidates == 0) {
    return Send(mptr.release(), NULL);
  }
  candidate_list->set_page_width(1);
  candidate_list->set_page_height(context.menu.page_size);
  candidate_list->set_selected_candidate(
      context.menu.highlighted_candidate_index);
  int page_start = context.menu.page_size * context.menu.page_no;
  candidate_list->set_page_start(page_start);
  int total_candidates = page_start + context.menu.num_candidates +
      !context.menu.is_last_page;
  candidate_list->set_total_candidates(total_candidates);
  candidate_list->set_show_page_flip_buttons(true);
  candidate_list->set_visible(true);
  for (const char* k = (context.menu.select_keys ? context.menu.select_keys
                                                 : "1234567890"); *k; ++k) {
    candidate_list->add_selection_key((uint32)(*k));
  }
  for (int i = 0; i < context.menu.num_candidates; ++i) {
    ipc::proto::Candidate* candidate = candidate_list->add_candidate();
    ipc::proto::Text* text = candidate->mutable_text();
    std::string candidate_text(context.menu.candidates[i].text);
    if (context.menu.candidates[i].comment) {
      std::string comment(context.menu.candidates[i].comment);
      candidate_text += " ";
      candidate_text += comment;
      int candidate_text_length = Utf8ToWide(candidate_text).length();
      int comment_length = Utf8ToWide(comment).length();
      ipc::proto::Attribute* attribute = text->add_attribute();
      attribute->set_start(candidate_text_length - comment_length);
      attribute->set_end(candidate_text_length);
      attribute->set_type(ipc::proto::Attribute::FOREGROUND);
      attribute->mutable_color_value()->set_red(0);
      attribute->mutable_color_value()->set_green(0);
      attribute->mutable_color_value()->set_blue(1);
    }
    text->set_text(candidate_text);
    ipc::proto::Attribute* attribute = text->add_attribute();
    attribute->set_start(0);
    attribute->set_end(Utf8ToWide(candidate_text).length());
    attribute->set_type(ipc::proto::Attribute::FONT_SIZE);
    attribute->set_float_value(kDefaultFontSize);
  }
  return Send(mptr.release(), NULL);
}

}  // namespace rime
}  // namespace plugins
