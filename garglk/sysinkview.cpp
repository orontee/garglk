#include "sysinkview.h"

#include "garglk.h"
#include "inkview.h"
#include <atomic>
#include <functional>
#include <iostream>
#include <ostream>
#include <string>
#include <thread>

static bool refresh_needed = true;

static constexpr int TICK_PERIOD_MILLIS = 10;
static std::atomic<bool> process_events(false);

static constexpr int MESSAGE_DELAY = 5000;

void glk_request_timer_events(glui32 ms)
{
  auto& app = garglk::App::get_app();
  app.window->start_timer(ms);
}

void gli_notification_waiting()
{
  auto *event_handler = GetEventHandler();
  SendEvent(event_handler, garglk::CustomEvent::gli_notification_waiting, 0, 0);
}

void garglk::winabort(const std::string &msg)
{
  std::cerr << "fatal: " << msg << std::endl;
  Message(ICON_ERROR, "Error", msg.c_str(), MESSAGE_DELAY);
  gli_exit(EXIT_FAILURE);
}

void garglk::winwarning(const std::string &title, const std::string &msg)
{
  std::cerr << "warning: " << msg << std::endl;
  Message(ICON_WARNING, title.c_str(), msg.c_str(), MESSAGE_DELAY);
}

void winexit()
{
  gli_exit(0);
}

std::string garglk::winopenfile(const char *prompt, FileFilter filter)
{
  return "/mnt/ext1/downloads/heroes.z5";
  // TODO
}

std::string garglk::winsavefile(const char *prompt, FileFilter filter)
{
  std::string filename = "/mnt/ext1/downloads/garglk_" + std::string{prompt};
  return filename;
  // TODO
}

void winclipstore(const std::vector<glui32> &text)
{
  // TODO
}

bool garglk::Window::_timed_out = false;
static constexpr char _timer_name[] = "gpp_timer";

garglk::Window::Window():
  _screen_image{NewBitmap8(ScreenWidth(), ScreenHeight())}
{}

void garglk::Window::start_timer(unsigned long ms) {
  ClearTimerByName(_timer_name);

  // SetHardTimer() takes int, so limit to avoid wrapping.
  if (ms > std::numeric_limits<int>::max()) {
    ms = std::numeric_limits<int>::max();
  }

  if (ms != 0) {
    SetHardTimer(_timer_name, []() {
      _timed_out = true;
    }, ms);
  }
}

int garglk::Window::handle_close_event() {
  gli_exit(0);

  return 1;
}

void garglk::Window::refresh() {
  if (!gli_drawselect) {
    gli_windows_redraw();
  } else {
    gli_drawselect = false;
  }

  auto* const event_handler = GetEventHandler();
  SendEvent(event_handler, EVT_REPAINT, 0, 0);

  refresh_needed = false;
}


int garglk::Window::handle_paint_event()
{
  const auto data_size = _screen_image->scanline * _screen_image->height;
  std::memcpy(this->_screen_image->data,
              gli_image_rgb.data(), data_size);

  DrawBitmap(0, 0, _screen_image.get());

  FullUpdate();

  return 1;
}

void gli_edit_config()
{
  // TODO
}

int garglk::Window::handle_key_event(int event_type, int key)
{
  refresh_needed = true;

  static const std::map<int, std::function<void()>> keys = {
    {IV_KEY_PREV,     []{ gli_input_handle_key(keycode_PageUp); }},
    {IV_KEY_NEXT,     []{ gli_input_handle_key(keycode_PageDown); }}
  };

  try {
    keys.at(key)();
    return 1;
  } catch (const std::out_of_range &) {
  }
  return 0;
}

int garglk::Window::handle_pointer_event(int event_type, int pointer_pos_x,
                                         int pointer_pos_y) {
  if (event_type == EVT_POINTERUP) {
    gli_input_handle_click(pointer_pos_x, pointer_pos_y);

    return 1;
  }
  // TODO add support to swipe
  return 0;
}

void wininit()
{
  InitInkview(TASK_SELFREGISTER);
  SetOrientation(0);
  TouchScreenEnable(true);
  SetGSensorEnabled(true);

  garglk::App::get_app();

  PrepareForLoop([](int event_type, int param_one, int param_two) {
    auto& app = garglk::App::get_app();
    return app.process_event(event_type, param_one, param_two);
  });
  // interpreter is responsible for periodically calling
  // ProcessEventLoop()...

  // ...the interpreter loop frequency may be two high to process
  // InkView events

  std::thread([]() {
    while (true) {
      std::this_thread::sleep_for(std::chrono::milliseconds(TICK_PERIOD_MILLIS));
      process_events.store(true, std::memory_order_relaxed);
    }
  })
    .detach();
}

void winopen()
{
  constexpr bool first_resize = true;
  gli_windows_size_change(ScreenWidth(), ScreenHeight(), first_resize);

  wintitle();
}

void wintitle()
{
  auto& app = garglk::App::get_app();
  app.set_task_name();
}

void winrepaint(int x0, int y0, int x1, int y1)
{
  refresh_needed = true;
}

bool windark()
{
  const auto firmware_version = GetSoftwareVersion();
  const int major_version = 6;
  const int minor_version = 8;
  std::stringstream to_parse{firmware_version};
  std::string token;
  bool support_screen_inversion = false;

  if (std::getline(to_parse, token, '.')) {
    std::getline(to_parse, token, '.');
    try {
      if (std::stoi(token) >= major_version) {
        if (minor_version == 0) {
          support_screen_inversion = true;
        } else {
          std::getline(to_parse, token, '.');
          if (std::stoi(token) >= minor_version) {
            support_screen_inversion = true;
          }
        }
      }
    } catch (const std::invalid_argument &error) {
    }
  }

  if (support_screen_inversion) {
    const auto screenModeInverted = IvGetScreenModeInversion();
    return screenModeInverted;
  }
  return false;
}

nonstd::optional<std::string> garglk::winfontpath(const std::string &filename)
{
  return std::string{GAMEPATH} + "/garglk/" + filename;
}

std::vector<std::string> garglk::winappdata()
{
  std::vector<std::string> paths;
  paths.push_back(std::string{STATEPATH} + "/garglk");
  return paths;
}

nonstd::optional<std::string> garglk::winappdir()
{
  return std::string{GAMEPATH} + "/garglk";
}

bool garglk::winisfullscreen()
{
  return true;
}

void gli_tick()
{
  // Keep processing events even in the absence of calls to
  // glk_select(). Processing events is expensive, so should not be
  // done each tick (which generally happens each VM instruction).
  // Originally this waited at least 10ms between calls, but the mere
  // act of checking a timer each iteration was too expensive. Now a
  // separate thread sits and atomically updates "process_events"
  // every 10ms, since checking this atomic variable is much faster
  // than checking a timer.
  if (process_events.load(std::memory_order_relaxed)) {
    ProcessEventLoop();
    process_events.store(false, std::memory_order_relaxed);
  }
}

void gli_select(event_t *event, bool polled)
{
  gli_event_clearevent(event);

  ProcessEventLoop();

  gli_dispatch_event(event, polled);

  auto& app = garglk::App::get_app();
  if (refresh_needed) {
    app.window->refresh();
  }

  if (!polled) {
    while (event->type == evtype_None && !app.window->timed_out()) {
      if (refresh_needed) {
        app.window->refresh();
      }

      ProcessEventLoop();
      gli_dispatch_event(event, polled);
    }
  }

  if (event->type == evtype_None && app.window->timed_out()) {
    gli_event_store(evtype_Timer, nullptr, 0, 0);
    gli_dispatch_event(event, polled);
    app.window->reset_timeout();
  }

  process_events.store(false, std::memory_order_relaxed);
}

garglk::App& garglk::App::get_app() {
  static App app;
  return app;
}

garglk::App::App() {}

int garglk::App::process_event(int event_type, int param_one, int param_two) {
  if (event_type == EVT_INIT) {
    return 1;
  }
  if (event_type == EVT_SHOW or event_type == EVT_ACTIVATE) {
    this->window->handle_paint_event();
    return 1;
  }

  if (event_type == EVT_HIDE) {
    // TODO
    return 1;
  }

  if (event_type == EVT_EXIT) {
    return 0;
  }

  if (event_type == EVT_CUSTOM) {
    return this->handle_custom_event(param_one, param_two);
  }

  if (event_type == EVT_REPAINT) {
    return this->window->handle_paint_event();
  }

  if (ISPOINTEREVENT(event_type) || event_type == EVT_SCROLL) {
    return this->window->handle_pointer_event(event_type, param_one, param_two);
  }

  if (ISKEYEVENT(event_type)) {
    return this->window->handle_key_event(event_type, param_one);
  }
  // EVT_MTSYNC, EVT_CONFIGCHANGED, EVT_SCREEN_INVERSION_MODE_CHANGED
  return 0;
}

void garglk::App::set_task_name() {
  const auto task_identifier = GetCurrentTask();
  const auto task_info = GetTaskInfo(task_identifier);
  if (!task_info) {
    return;
  }

  compute_task_name();

  SetTaskParameters(task_identifier, task_info->appname,
                    _task_name.c_str(),
                    nullptr,
                    task_info->flags);
}

void garglk::App::compute_task_name() {
  _task_name = gli_story_title;

  if (_task_name.empty() and !gli_story_name.empty()) {
    _task_name = gli_story_name + " - " + gli_program_name;
  } else {
    _task_name = gli_program_name;
  }
}

int garglk::App::handle_custom_event(int param_one, int param_two) {
  if (param_one == garglk::CustomEvent::gli_notification_waiting) {
    // TODO what should be done?
    return 1;
  }
  return 0;
}
