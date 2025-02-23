#ifndef GARGLK_SYSINKVIEW_H
#define GARGLK_SYSINKVIEW_H

#include "glk.h"

#include <memory>
#include "inkview.h"

namespace garglk {

  enum CustomEvent {
    gli_notification_waiting,
  };

  class Window {
  public:
    Window();

    void start_timer(unsigned long);

    bool timed_out() const { return _timed_out; }

    void reset_timeout() { _timed_out = false; }

    void refresh();

    int handle_close_event();

    int handle_paint_event();

    int handle_pointer_event(int event_type, int pointer_pos_x,
                             int pointer_pos_y);

    int handle_key_event(int event_type, int key);

  private:
    std::unique_ptr<ibitmap> _screen_image;

    static bool _timed_out;
  };

  class App {
  public:
    App();

    int process_event(int event_type, int param_one, int param_two);

    void set_task_name();

    std::unique_ptr<Window> window;

    static App& get_app();

  private:
    std::string _task_name;

    void compute_task_name();

    int handle_custom_event(int param_one, int param_two);
  };

}

#endif
