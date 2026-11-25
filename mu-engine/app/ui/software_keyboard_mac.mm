// The Switch build's software_keyboard (Melee-NX-NetPlay frontend/src/software_keyboard.cpp, the console's keyboard) on the
// Mac: a small text prompt on the main thread for the feature modules that ask for a line of text (the Replay library's
// search). Called from a worker thread; the game waits while the prompt is up, as the console's keyboard takes the screen.
// SPDX-License-Identifier: GPL-3.0-or-later
#import <AppKit/AppKit.h>
#include <cstdlib>
#include <string>

namespace fe {
bool software_keyboard(const char* header, const char* guide, std::string& text) {
  if (const char* given = std::getenv("MU_TEST_KEYBOARD")) { text = given; return true; }   // tests: no prompt, this answer
  __block bool ok = false;
  __block std::string result = text;
  const std::string title = header ? header : "", note = guide ? guide : "";
  void (^ask)(void) = ^{
    NSAlert* alert = [[NSAlert alloc] init];
    alert.messageText = [NSString stringWithUTF8String:title.c_str()];
    alert.informativeText = [NSString stringWithUTF8String:note.c_str()];
    [alert addButtonWithTitle:@"OK"];
    [alert addButtonWithTitle:@"Cancel"];
    NSTextField* field = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 280, 24)];
    field.stringValue = [NSString stringWithUTF8String:result.c_str()] ?: @"";
    alert.accessoryView = field;
    alert.window.initialFirstResponder = field;
    if ([alert runModal] == NSAlertFirstButtonReturn) { ok = true; result = field.stringValue.UTF8String ?: ""; }
  };
  if (NSThread.isMainThread) ask(); else dispatch_sync(dispatch_get_main_queue(), ask);
  if (ok) text = result;
  return ok;
}
}  // namespace fe
