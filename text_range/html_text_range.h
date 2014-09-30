/*
  Copyright 2014 Google Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#ifndef GOOPY_TEXT_RANGE_HTML_TEXT_RANGE_H_
#define GOOPY_TEXT_RANGE_HTML_TEXT_RANGE_H_

#include <mshtml.h>
#include <windows.h>
#include <string>
#include "base/basictypes.h"
#include "base/callback.h"
#include "common/framework_interface.h"
#include "common/smart_com_ptr.h"

namespace ime_goopy {
// The class defines text range object for IE HTML control.
class HtmlTextRange : public TextRangeInterface {
 public:
  // Creates a range object from the selection of the window.
  // Returns NULL if the window isn't an HTML control.
  static HtmlTextRange *CreateFromSelection(
      Callback1<HtmlTextRange*> *on_reconvert, HWND handle);
  HtmlTextRange();
  HtmlTextRange(
      Callback1<HtmlTextRange*> *on_reconvert, HWND handle,
      IHTMLTxtRange *range);
  virtual wstring GetText();
  virtual void ShiftStart(int offset, int *actual_offset);
  virtual void ShiftEnd(int offset, int *actual_offset);
  virtual void CollapseToStart();
  virtual void CollapseToEnd();
  virtual bool IsEmpty();
  virtual bool IsContaining(TextRangeInterface *inner_range);
  virtual void Reconvert();
  virtual TextRangeInterface *Clone();
  void SetText(const wchar_t *text);
  void Select();
  POINT GetCompositionPos();
  HWND handle() const { return handle_; }
 private:
  Callback1<HtmlTextRange*> *on_reconvert_;
  HWND handle_;
  SmartComPtr<IHTMLTxtRange> range_;
};
}  // namespace ime_goopy
#endif  // GOOPY_TEXT_RANGE_HTML_TEXT_RANGE_H_
