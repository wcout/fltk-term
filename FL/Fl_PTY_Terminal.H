//
// Fl_PTY_Terminal - A pseudo terminal widget for Fast Light Tool Kit (FLTK).
//
// (c) 2026 wcout
//
#ifndef Fl_PTY_Terminal_H
#define Fl_PTY_Terminal_H

#include <FL/Fl_Terminal.H>
// OS-specific includes & globals
#ifdef _WIN32
#include <windows.h>
#if defined(NTDDI_VERSION) && (NTDDI_VERSION >= 0x0A000006)
// WINAPI CreatePseudoConsole() exists from Windows 10/RS5 (10/2018) or higher
#define HAS_PSEUDO_CONSOLE 1
#else // NTDDI_VERSION not defined or too low
#define HAS_PSEUDO_CONSOLE 0
DECLARE_HANDLE(HPCON);
#pragma message("WARNING: PTY functionality not available (needs WIN10 >= RS5)")
#endif
#else // Linux, macOS
#define HAS_PSEUDO_CONSOLE 1
#include <stdlib.h>
#endif

// PTY terminal class with resize logic
class Fl_PTY_Terminal : public Fl_Terminal {
public:
  enum Fl_PTY_Terminal_Callback_Reason {
    FL_REASON_PTY_STOPPED = FL_REASON_TERM_USER,
    FL_REASON_RIGHT_MOUSE
  };
  Fl_PTY_Terminal(int x, int y, int w, int h, const char* l = nullptr);
  ~Fl_PTY_Terminal();
#ifndef _WIN32
private:
  void onRead(int fd_, void *data_);
  static void pty_read_callback(int fd_, void *data);
#else
  static DWORD WINAPI ChildReaderThread(LPVOID lpParam);
  static VOID CALLBACK WaitCallback(PVOID lpParameter, BOOLEAN TimerOrWaitFired);
#endif
  int handle_dnd(int event);
  int handle_paste(const std::string &text);
  void snap_to_grid(void);
protected:
  void write_pty(const char *buf_, size_t len);
  bool init();
  void quit();
  void reset() override;
  void set_pty_size();
  virtual void notify(int reason, const std::string &buf = "") {}
public:
  void dump(const char *buf, size_t n);
  void input(const std::string& buffer);
  void send_pty(const char *buf_) override;
  int handle(int event) override;
  void toggle_default_colors();
  void resize(int X, int Y, int W, int H) override;
  bool search(const char *needle, int &row, int &col, bool down = true);
  void command(const char *cmd_);
  void logging(int level, FILE *logfile = stderr) {
    log_ = level;
    logfile_ = logfile;
  }
  bool inited() const { return inited_; }
  /**
    Flag that toggles 'snap to character grid' after a resize
    Initially false, because application must be prepared for that.
  */
  void snap(bool v) { snap_ = v; }
  static void snap_to_grid_cb(void *d);
private:
  int    pty_master_fd_ = -1;
  bool   inited_        = false;
  int    log_           = 0;
  FILE * logfile_       = stderr;
  bool   snap_          = false;
#ifdef _WIN32
  HANDLE _hPipeInWrite = INVALID_HANDLE_VALUE;
  HANDLE _hPipeOutRead = INVALID_HANDLE_VALUE;
  HANDLE _hWaitHandle  = INVALID_HANDLE_VALUE;
  HPCON _hPC = nullptr;
public:
  HANDLE readPipe() const {
    return _hPipeOutRead;
  }
#endif
};
#endif // #ifndef Fl_PTY_Terminal_H
