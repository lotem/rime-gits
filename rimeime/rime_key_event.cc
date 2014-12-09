#include "rimeime/rime_key_event.h"

#include <windows.h>
#include "components/common/keystroke_util.h"
#include "ipc/constants.h"
#include "ipc/protos/ipc.pb.h"
#include "rimeime/keysym.h"

using ime_goopy::components::KeyStroke;
using ime_goopy::components::KeyStrokeUtil;

namespace rimeime {

static int TranslateModifiers(const KeyStroke& key) {
  int result = 0;
  if (key.IsShifted()) {
    result |= SHIFT_MASK;
  }
  if (key.IsCaplocked()) {
    result |= LOCK_MASK;
  }
  if (key.IsCtrled()) {
    result |= CONTROL_MASK;
  }
  if (key.IsMenued()) {
    result |= ALT_MASK;
  }
  if (key.IsUp()) {
    result |= RELEASE_MASK;
  }
  if (key.IsCaptital() && key.IsDown()) {
    // NOTE: rime assumes XK_Caps_Lock to be sent before modifier changes,
    // while VK_CAPITAL has the modifier changed already.
    // so it is necessary to revert LOCK_MASK.
    result ^= LOCK_MASK;
  }
  return result;
}

static Keycode TranslateKeyCode(const KeyStroke& key) {
  switch (key.vkey()) {
  case VK_BACK: return BackSpace;
  case VK_TAB: return Tab;
  case VK_CLEAR: return Clear;
  case VK_RETURN: return Return;

  case VK_SHIFT:
  {
    if (key.IsRightShift())
      return Shift_R;
    else
      return Shift_L;
  }
  case VK_CONTROL:
  {
    if (key.IsRightControl())
      return Control_R;
    else
      return Control_L;
  }
  case VK_MENU: return Alt_L;
  case VK_PAUSE: return Pause;
  case VK_CAPITAL: return Caps_Lock;

  case VK_KANA: return Hiragana_Katakana;
  //case VK_JUNJA: return 0;
  //case VK_FINAL: return 0;
  case VK_KANJI: return Kanji;

  case VK_ESCAPE: return Escape;

  //case VK_CONVERT: return 0;
  //case VK_NONCONVERT: return 0;
  //case VK_ACCEPT: return 0;
  //case VK_MODECHANGE: return 0;

  case VK_SPACE: return space;
  case VK_PRIOR: return Prior;
  case VK_NEXT: return Next;
  case VK_END: return End;
  case VK_HOME: return Home;
  case VK_LEFT: return Left;
  case VK_UP: return Up;
  case VK_RIGHT: return Right;
  case VK_DOWN: return Down;
  case VK_SELECT: return Select;
  case VK_PRINT: return Print;
  case VK_EXECUTE: return Execute;
  //case VK_SNAPSHOT: return 0;
  case VK_INSERT: return Insert;
  case VK_DELETE: return Delete;
  case VK_HELP: return Help;

  case VK_LWIN: return Meta_L;
  case VK_RWIN: return Meta_R;
  //case VK_APPS: return 0;
  //case VK_SLEEP: return 0;
  case VK_NUMPAD0: return KP_0;
  case VK_NUMPAD1: return KP_1;
  case VK_NUMPAD2: return KP_2;
  case VK_NUMPAD3: return KP_3;
  case VK_NUMPAD4: return KP_4;
  case VK_NUMPAD5: return KP_5;
  case VK_NUMPAD6: return KP_6;
  case VK_NUMPAD7: return KP_7;
  case VK_NUMPAD8: return KP_8;
  case VK_NUMPAD9: return KP_9;
  case VK_MULTIPLY: return KP_Multiply;
  case VK_ADD: return KP_Add;
  case VK_SEPARATOR: return KP_Separator;
  case VK_SUBTRACT: return KP_Subtract;
  case VK_DECIMAL: return KP_Decimal;
  case VK_DIVIDE: return KP_Divide;
  case VK_F1: return F1;
  case VK_F2: return F2;
  case VK_F3: return F3;
  case VK_F4: return F4;
  case VK_F5: return F5;
  case VK_F6: return F6;
  case VK_F7: return F7;
  case VK_F8: return F8;
  case VK_F9: return F9;
  case VK_F10: return F10;
  case VK_F11: return F11;
  case VK_F12: return F12;
  case VK_F13: return F13;
  case VK_F14: return F14;
  case VK_F15: return F15;
  case VK_F16: return F16;
  case VK_F17: return F17;
  case VK_F18: return F18;
  case VK_F19: return F19;
  case VK_F20: return F20;
  case VK_F21: return F21;
  case VK_F22: return F22;
  case VK_F23: return F23;
  case VK_F24: return F24;

  case VK_NUMLOCK: return Num_Lock;
  case VK_SCROLL: return Scroll_Lock;

  case VK_LSHIFT: return Shift_L;
  case VK_RSHIFT: return Shift_R;
  case VK_LCONTROL: return Control_L;
  case VK_RCONTROL: return Control_R;
  case VK_LMENU: return Alt_L;
  case VK_RMENU: return Alt_R;
  }
  if (key.IsAscii() && key.IsVisible()) {
    return (Keycode)key.AsciiValue();
  }
  if (key.IsCtrled()) {
    uint8 key_state[256] = {0};
    KeyStroke bare_key = KeyStroke(key.vkey(), key_state, key.IsDown());
    if (bare_key.IsAscii() && bare_key.IsVisible()) {
      return (Keycode)bare_key.AsciiValue();
    }
  }
  return Null;
}

RimeKeyEvent::RimeKeyEvent(const ipc::proto::KeyEvent& key) {
  KeyStroke key_stroke = KeyStrokeUtil::ConstructKeyStroke(key);
  modifiers = TranslateModifiers(key_stroke);
  keycode = TranslateKeyCode(key_stroke);
}

}  // namespace rimeime
