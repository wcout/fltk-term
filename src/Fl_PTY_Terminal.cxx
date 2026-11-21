//
// Fl_PTY_Terminal - A pseudo terminal widget for Fast Light Tool Kit (FLTK).
//
// (c) 2026 wcout
//
#include <FL/Fl_PTY_Terminal.H>
#ifdef _WIN32
#include <wincon.h>
#include <io.h>
#include <fcntl.h>
#else
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <errno.h>
#endif
#include <FL/fl_draw.H> // fl_width()

////////////////////////////////////////////////////////////////////////////////////////////////
//  PTY terminal class with resize logic implementation
////////////////////////////////////////////////////////////////////////////////////////////////
Fl_PTY_Terminal::Fl_PTY_Terminal(int x, int y, int w, int h, const char* l/* = nullptr*/) :
  Fl_Terminal(x, y, w, h, l) {
  inited_ = init();
  if (inited_) {
#ifndef _WIN32
    Fl::add_fd(pty_master_fd_, FL_READ, pty_read_callback, this);
#endif
  }
  reset();
  callback([](Fl_Widget *wgt, void *d) {
    const char *buf = (const char *)d;
    int reason = Fl::callback_reason();
    Fl_PTY_Terminal *term = (Fl_PTY_Terminal *)wgt;
    term->notify(reason, buf ? buf : "");
  }, this);
}

void Fl_PTY_Terminal::reset() {
  // turn on "dynamic" colors for cursor and selection
  dynamic_cursor_color(true);
  dynamic_selection_color(true);
  set_cursor_style(1); // block blinking
}

Fl_PTY_Terminal::~Fl_PTY_Terminal() {
  Fl::remove_timeout(snap_to_grid_cb, this);
#ifdef _WIN32
#if HAS_PSEUDO_CONSOLE
  if (_hPC) {
    ClosePseudoConsole(_hPC);
  }
#endif // HAS_PSEUDO_CONSOLE
  if (_hWaitHandle != INVALID_HANDLE_VALUE) {
    CloseHandle(_hWaitHandle);
  }
  if (_hPipeInWrite != INVALID_HANDLE_VALUE) {
    CloseHandle(_hPipeInWrite);
  }
  if (_hPipeOutRead != INVALID_HANDLE_VALUE) {
    CloseHandle(_hPipeOutRead);
  }
#else
  if (pty_master_fd_ >= 0) {
    ::close(pty_master_fd_);
  }
#endif
}

void Fl_PTY_Terminal::dump(const char *buf, size_t n) {
  static const char BOLD[] = "\033[1m";
  static const char TEXT[] = "\033[90m";
  static const char HEX[] = "\033[31m";
  static const char CTRL[] = "\033[1m\033[32m";
  static const char OFF[] = "\033[0m";

  for (size_t i = 0; i < n; i++) {
    unsigned int c = (unsigned char)buf[i];
    if (c == 27) {
      fprintf(logfile_, "%s\\e%s", BOLD, OFF);
    } else if (c >= ' ' && c <= 127) {
      fprintf(logfile_, "%s%c%s", TEXT, c, OFF);
    } else {
      switch (c) {
        case '\r':
          fprintf(logfile_, "%s\\r%s", CTRL, OFF);
          break;
        case '\n':
          fprintf(logfile_, "%s\\n%s", CTRL, OFF);
          break;
        case '\a':
          fprintf(logfile_, "%s\\a%s", CTRL, OFF);
          break;
        case '\t':
          fprintf(logfile_, "%s\\t%s", CTRL, OFF);
          break;
        default:
          fprintf(logfile_, "%s\\x%02X%s", HEX, (unsigned char)c, OFF);
      }
    }
  }
  fprintf(logfile_, "\n");
}

void Fl_PTY_Terminal::write_pty(const char *buf, size_t len) {
  if (pty_master_fd_ != -1) {
#ifdef _WIN32
    DWORD bytes_written = 0;
    if (_hPipeInWrite != INVALID_HANDLE_VALUE) {
      WriteFile(_hPipeInWrite, buf, (DWORD)len, &bytes_written, nullptr);
    }
#else
    ssize_t bytes_written = ::write(pty_master_fd_, buf, len);
    if ((int)bytes_written != (int)len) {
      fprintf(stderr, "Write error PTY: %d != %d\n", (int)bytes_written, (int)len);
    }
#endif
  }
}

#ifdef _WIN32
/*static*/
VOID CALLBACK Fl_PTY_Terminal::WaitCallback(PVOID lpParameter, BOOLEAN TimerOrWaitFired) {
  Fl_PTY_Terminal *term = (Fl_PTY_Terminal *)lpParameter;
  term->input("");
}
#endif

bool Fl_PTY_Terminal::init() {
  // Initialize backend
#ifdef _WIN32
  auto enable_VT_mode =[&](void) -> bool {
    // Enable VT mode (Color)
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) {
      return false;
    }

    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) {
      return false;
    }

    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
    return true;
  };

  enable_VT_mode();

  // WINDOWS ConPTY
  HANDLE hPipeInRead = INVALID_HANDLE_VALUE;
  HANDLE hPipeOutWrite = INVALID_HANDLE_VALUE;
  SECURITY_ATTRIBUTES sa = { sizeof(sa), nullptr, TRUE };

  if (!CreatePipe(&hPipeInRead, &_hPipeInWrite, &sa, 0) ||
      !CreatePipe(&_hPipeOutRead, &hPipeOutWrite, &sa, 0)) {
    return false;
  }

  SetHandleInformation(_hPipeInWrite, HANDLE_FLAG_INHERIT, 0);
  SetHandleInformation(_hPipeOutRead, HANDLE_FLAG_INHERIT, 0);

#if HAS_PSEUDO_CONSOLE
  COORD size = { 80, 25 };
  HRESULT hr = CreatePseudoConsole(size, hPipeInRead, hPipeOutWrite, 0, &_hPC);
  CloseHandle(hPipeInRead);
  CloseHandle(hPipeOutWrite);

  if (FAILED(hr)) {
    return false;
  }

  STARTUPINFOEXW siEx = { 0 };
  siEx.StartupInfo.cb = sizeof(STARTUPINFOEXW);
  SIZE_T sizeList = 0;
  InitializeProcThreadAttributeList(nullptr, 1, 0, &sizeList);
  siEx.lpAttributeList = (PPROC_THREAD_ATTRIBUTE_LIST)HeapAlloc(GetProcessHeap(), 0, sizeList);
  InitializeProcThreadAttributeList(siEx.lpAttributeList, 1, 0, &sizeList);

  UpdateProcThreadAttribute(siEx.lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE, _hPC, sizeof(HPCON), nullptr, nullptr);
  siEx.StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
  siEx.StartupInfo.wShowWindow = SW_HIDE;

  PROCESS_INFORMATION pi = { 0 };
  wchar_t cmdPath[] = L"C:\\Windows\\System32\\cmd.exe";

  // For Windows PowerShell
//  wchar_t cmdPath[] = L"powershell.exe -NoLogo -NoExit -Command chcp 65001";

  // Or for PowerShell 7+
//  wchar_t cmdPath[] = L"pwsh.exe -NoLogo -NoExit";

  BOOL success = CreateProcessW(nullptr, cmdPath, nullptr, nullptr, TRUE,
                                EXTENDED_STARTUPINFO_PRESENT, nullptr, nullptr,
                                &siEx.StartupInfo, &pi);
  if (!success) {
    return false;
  }

  RegisterWaitForSingleObject(&_hWaitHandle, pi.hProcess, WaitCallback, this, INFINITE, WT_EXECUTEONLYONCE);

  CloseHandle(pi.hThread);

  pty_master_fd_ = 1; // dummy, for code unification

  // Spawn background reader thread
  CreateThread(NULL, 0, ChildReaderThread, this, 0, NULL);

  return true;
#else // HAS_PSEUDO_CONSOLE
  return false;
#endif

#else
  // Use POSIX PTY (macOS/Linux)
  pty_master_fd_ = ::posix_openpt(O_RDWR | O_CLOEXEC | O_NONBLOCK);
  if (pty_master_fd_ < 0) {
    return false;
  }

  if (::grantpt(pty_master_fd_) < 0 || ::unlockpt(pty_master_fd_) < 0) {
    ::close(pty_master_fd_);
    return false;
  }

  char* slave_name = ::ptsname(pty_master_fd_);
  if (!slave_name) {
    ::close(pty_master_fd_);
    return false;
  }

  pid_t pid = ::fork();
  if (pid < 0) {
    ::close(pty_master_fd_);
    return false;
  }

  if (pid == 0) {
    ::setsid();
    int slave_fd = ::open(slave_name, O_RDWR);
    if (slave_fd < 0) {
      std::exit(1);
    }
    ::close(pty_master_fd_);

#ifdef TIOCSCTTY
    ::ioctl(slave_fd, TIOCSCTTY, 0);
#endif

    ::dup2(slave_fd, STDIN_FILENO);
    ::dup2(slave_fd, STDOUT_FILENO);
    ::dup2(slave_fd, STDERR_FILENO);
    ::close(slave_fd);

#ifdef __APPLE__
    ::execl("/bin/zsh", "zsh", "--login", nullptr);
#else
    ::execl("/bin/bash", "bash", "--login", nullptr);
#endif
    std::exit(1);
  }
  return true;
#endif
}

void Fl_PTY_Terminal::quit() {
  append("\n--- shell process stopped ---\n");
  do_callback(this, nullptr, (Fl_Callback_Reason)FL_REASON_PTY_STOPPED);
}

void Fl_PTY_Terminal::input(const std::string& buffer) {
  if (buffer.empty()) {
    quit();
    return;
  }
  size_t bytes_read = buffer.size();
  const char *buf = buffer.c_str();
  if (bytes_read > 0) {
    if (log_ > 1) {
      fprintf(logfile_, "Send %d bytes to terminal:\n", (int)bytes_read);
      dump(buf, bytes_read);
    }
    append_utf8(buf, bytes_read);
  }
}

#ifdef _WIN32
// Background thread loop for reading stdout
/*static*/
DWORD WINAPI Fl_PTY_Terminal::ChildReaderThread(LPVOID lpParam) {
  // Structure to pass data from background thread to FLTK main thread
  struct MessageData {
    Fl_PTY_Terminal *term;
    std::string text;
  };
  Fl_PTY_Terminal *term = reinterpret_cast<Fl_PTY_Terminal *>(lpParam);
  HANDLE hReadPipe = term->readPipe();
  char buffer[4096];
  DWORD bytesRead = 0;

  // Blocks efficiently in kernel mode when no data is available
  while (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
    buffer[bytesRead] = '\0';

    MessageData* msg = new MessageData{ term, std::string(buffer, bytesRead) };

    // Notify FLTK main thread to run fltk_update_callback
    Fl::awake([](void *data) {
      MessageData* msg = reinterpret_cast<MessageData*>(data);
      msg->term->input(msg->text);
      delete msg;
    }, msg);
  }

  // Clean up handle and notify GUI of termination
  CloseHandle(hReadPipe);
  MessageData* msg = new MessageData{ term, "" };
  Fl::awake([](void *data) {
    MessageData* msg = reinterpret_cast<MessageData*>(data);
    msg->term->input(msg->text);
    delete msg;
  }, msg);

  return 0;
}
#else
void Fl_PTY_Terminal::onRead(int fd_, void *data_) {
  char buffer[4096];
  bool process_alive = true;
  int bytes_read = 0;

  // Under macOS/Linux we use POSIX read()
  ssize_t posix_bytes = ::read(fd_, buffer, sizeof(buffer) - 1);
  if (posix_bytes > 0) {
    bytes_read = (int)posix_bytes;
  } else if (posix_bytes == 0 || (posix_bytes < 0 && errno != EAGAIN)) {
    process_alive = false;
  }

  if (bytes_read > 0) {
    buffer[bytes_read] = '\0';
    input(std::string(buffer, bytes_read));
  }

  if (!process_alive) {
    Fl::remove_fd(fd_);
    input("");
  }
}

// Linux pty fd callback
/*static*/
void Fl_PTY_Terminal::pty_read_callback(int fd, void *data) {
  Fl_PTY_Terminal* terminal = static_cast<Fl_PTY_Terminal*>(data);
  terminal->onRead(fd, data);
}
#endif

void Fl_PTY_Terminal::send_pty(const char *buf) {
  int len = strlen(buf);
  if (log_) {
    fprintf(logfile_, "Terminal answer (%d bytes): ", len);
    dump(buf, len);
  }
  write_pty(buf, len);
}

int Fl_PTY_Terminal::handle_paste(const std::string &text) {
  if (text.size() && !alternate_buffer()) {
    if (bracketed_paste()) {
      write_pty("\x1b[200~", 6);
    }
    write_pty(text.c_str(), text.size());
    if (bracketed_paste()) {
      write_pty("\x1b[201~", 6);
    }
    return 1;
  }
  return 0;
}

int Fl_PTY_Terminal::handle_dnd(int event) {
  switch (event) {
    case FL_DND_ENTER:
    case FL_DND_DRAG:
    case FL_DND_RELEASE:
      return Fl::event_inside(this);
    case FL_PASTE:
      return handle_paste(std::string(Fl::event_text(), Fl::event_length()));
  }
  return 0;
}

// Catch Keyboard strokes
int Fl_PTY_Terminal::handle(int event) {
  auto backspace = [&](void) {
    // Send backspace (use DEL instead BS, which seems better accepted)
    char bs = '\x7f';
    write_pty(&bs, 1);
  };

  if (event == FL_KEYDOWN && pty_master_fd_ != -1) {
    int key = Fl::event_key();
    int state = Fl::event_state();

    int del;
    if (Fl::compose(del)) {
       if (del) {
         backspace();
       }
       int len = Fl::event_length();
       const char *text = Fl::event_text();
       // Fl_Terminal can't handle character combining via 'Combining Accute Accent' (0xCC81)
       if (len && (!(len == 3 && (uchar)text[1] == 0xCC && (uchar)text[2] == 0x81))) {
         write_pty(text, len);
        redraw_rate(1./10);
       }
       return 1;
    }

    // Scroll terminal view to bottom (so input will be seen)
    scrollbar->value(0);

    // Catch TAB key for auto completion
    if (key == FL_Tab) {
      char tab_char = '\t';
      write_pty(&tab_char, 1);
      return 1; // 1 signals to FLTK: event processed (prevents focus change)
    }
    // Catch Backspace key (\x08 does not always work)
    if (key == FL_BackSpace) {
      backspace();
      return 1; // 1 signals to FLTK: event processed (prevents focus change)
    }

    // Catch Strg (Ctrl) key combinations (e.g. Strg+C)
    if (state & FL_CTRL) {
      // Keep FLTK zoom keys functional
      if (key == '+' || key == '-' || key == '0') {
        return Fl_Terminal::handle(event);
      }
      if ((key == 'c' || key == 'C') && Fl_Terminal::is_selection()) {
        return Fl_Terminal::handle(event);
      } else if ((key == 'a' || key == 'A') && !alternate_buffer()) {
        return Fl_Terminal::handle(event);
      } else if ((key == 'v' || key == 'V') && !alternate_buffer()) {
        if (Fl::clipboard_contains(Fl::clipboard_plain_text)) {
          Fl::paste(*this, 1, Fl::clipboard_plain_text);
          return 1;
        }
      }

      // In ASCII Strg+A = 1, Strg+B = 2, Strg+C = 3, ...
      if (key >= 'a' && key <= 'z') {
        char ctrl_char = key - 'a' + 1;
        write_pty(&ctrl_char, 1);
        return 1;
      }
      // Catch uppercase letters, if Capslock/Shift is active
      else if (key >= 'A' && key <= 'Z') {
        char ctrl_char = key - 'A' + 1;
        write_pty(&ctrl_char, 1);
        return 1;
      }
    }

    // Arrow keys for command history (up/down) and Cursor navigation
    if (key == FL_Up) {
      write_pty((DECCKM() ? "\033OA" : "\033[A"), 3); // ANSI Escape-Code for Arror up
      redraw_rate(1./30);
      return 1;
    }
    if (key == FL_Down) {
      write_pty((DECCKM() ? "\033OB" : "\033[B"), 3); // ANSI Escape-Code for Arrow down
      redraw_rate(1./30);
      return 1;
    }
    if (key == FL_Right) {
      write_pty((DECCKM() ? "\033OC" :"\033[C"), 3); // ANSI Escape-Code for Arrow right
      redraw_rate(1./30);
      return 1;
    }
    if (key == FL_Left) {
      write_pty((DECCKM() ? "\033OD" : "\033[D"), 3); // ANSI Escape-Code for Arrow left
      redraw_rate(1./30);
      return 1;
    }
    if (key == FL_Delete) {
      write_pty("\033[3~", 4); // ANSI Escape-Code for Delete key
      return 1;
    }
    if (key == FL_Insert) {
      write_pty("\033[2~", 4); // ANSI Escape-Code for Insert key
      return 1;
    }
    if (key == FL_Home) {
      if (state & FL_CTRL) {
        write_pty("\033[1;5H", 6);  // ANSI Escape-Code for Home key
      } else {
        write_pty("\033[H", 3);  // ANSI Escape-Code for Home key
      }
      return 1;
    }
    if (key == FL_End) {
      if (state & FL_CTRL) {
        write_pty("\033[1;5F", 6); // ANSI Escape-Code for End key
      } else {
        write_pty("\033[F", 3); // ANSI Escape-Code for End key
      }
      return 1;
    }
    if (key >= FL_F + 1 && key <= FL_F + 5) {
      char buf[7];
      snprintf(buf, sizeof(buf), "\033[%d~", key - FL_F - 1 + 11);
      write_pty(buf, 5); // ANSI Escape-Code for F1-F5
      return 1;
    }
    if (key >= FL_F + 6 && key <= FL_F + 10) {
      char buf[7];
      snprintf(buf, sizeof(buf), "\033[%d~", key - FL_F - 6 + 17);
      write_pty(buf, 5); // ANSI Escape-Code for F1-F10
      return 1;
    }
    if (key == FL_F + 11) {
      // toggle logging
      log_++;
      log_ %= 3;
      fprintf(logfile_, "logging: %d\n", log_);
      return 1;
    }
    if (key == FL_F + 12) {
      if (state & (FL_CTRL|FL_ALT)) {
         // Ctrl/ALt-F12 switches the standard color palette
         toggle_default_colors();
         return 1;
      }
      // toggle background color
      static std::vector<Fl_Color> bg_colors = {
        0xffffff00, 0x00000000,                         // pure white/black
        0x1f000000, 0x00001f00, 0x001f0000, 0x2f1f0000, // dark red/dark blue/dark green/dark orange
        0x08080800, 0x14141400, 0x1f1f1f00, 0x24242400, // decreasing dark gray levels
        0xcccccc00, 0xdddddd00, 0xe0e0e000, 0xf0f0f000  // increasing light gray levels
      };
      static size_t current_bg_index = 0;
      state & FL_SHIFT ? current_bg_index-- : current_bg_index++;
      if ((int)current_bg_index < 0) {
        current_bg_index = bg_colors.size() - 1;
      }
      if (current_bg_index >= bg_colors.size()) {
        current_bg_index = 0;
      }
      color(bg_colors[current_bg_index]);
      return 1;
    }
    if (key == FL_Page_Up) {
      write_pty("\033[5~", 4); // ANSI Escape-Code for Page up
      return 1;
    }
    if (key == FL_Page_Down) {
      write_pty("\033[6~", 4); // ANSI Escape-Code for Page down
      return 1;
    }

    // Handle ALT keys
    if (key >='a' && key <= 'z' && (state & FL_ALT)) {
      char buf[4];
      snprintf(buf, sizeof(buf), "\033%c", key);
      write_pty(buf, 2);
      return 1;
    }

    // Standard textinput (normal chars and Enter)
    const char* text = Fl::event_text();
    int len = Fl::event_length();
    if (len > 0) {
      write_pty(text, len);
      redraw_rate(1./10);
      return 1;
    }
  }
  if (handle_dnd(event)) {
    return 1;
  }

  return Fl_Terminal::handle(event);
}

// Is called after a resize and tries to change the
// dimensions to full character sizes.
void Fl_PTY_Terminal::snap_to_grid(void) {
  int cols = w_to_col(scrn_.w());
  int rows = h_to_row(scrn_.h());
  resize_display_columns(cols);       // change columns to screen width
  default_tabstops();                 // reset tabstops to default 8 char
  int W, H;
  char_to_pixel_dimension(cols, rows, W, H); // align to character boundaries if on PTY
  set_pty_size();
  if (W != w() || H != h()) {
    Fl_Group::size(W, H);             // IMPORTANT: tell bash the new disp_rows(), disp_cols()!
    do_callback(this, nullptr, (Fl_Callback_Reason)Fl_Terminal::FL_REASON_SIZE_CHANGE);
  }
}

// trigger resize to character boundaries
/*static*/
void Fl_PTY_Terminal::snap_to_grid_cb(void *d) {
  Fl_PTY_Terminal *term = (Fl_PTY_Terminal *)d;
  term->snap_to_grid();
}

void Fl_PTY_Terminal::toggle_default_colors() {
  // NOTE: toggling colors will update the display immediately
  static bool saved_def = false;
  static std::vector<std::vector<Fl_Color>> palettes = {
    { // GTK v1
      // fg normal
      0x17142100, 0xc01c2800, 0x26a26900, 0xa2734c00, 0x12488b00, 0xa347ba00, 0x2aa1b300, 0xd0cfcc00,
      // fg bright
      0x5e5c6400, 0xf6615100, 0x33da7a00, 0xe9ad0c00, 0x2a7bde00, 0xc061cb00, 0x33c7de00, 0xffffff00,
      // bg normal
      0x17142100, 0xc01c2800, 0x26a26900, 0xa2734c00, 0x12488b00, 0xa347ba00, 0x2aa1b300, 0xd0cfcc00,
      // bg bright
      0x5e5c6400, 0xf6615100, 0x33da7a00, 0xe9ad0c00, 0x2a7bde00, 0xc061cb00, 0x33c7de00, 0xffffff00,
    },
    { // GTK v2
      // fg normal
      0x2e343600, 0xcc000000, 0x4e9a0600, 0xc4a00000, 0x3465a400, 0x75507b00, 0x06989a00, 0xd3d7cf00,
      // fg bright
      0x55575300, 0xdf292900, 0x8ae23400, 0xfce94f00, 0x729fcf00, 0xad7fa800, 0x34e2e200, 0xeeeeec00,
      // bg normal
      0x2e343600, 0xcc000000, 0x4e9a0600, 0xc4a00000, 0x3465a400, 0x75507b00, 0x06989a00, 0xd3d7cf00,
      // bg bright
      0x55575300, 0xdf292900, 0x8ae23400, 0xfce94f00, 0x729fcf00, 0xad7fa800, 0x34e2e200, 0xeeeeec00,
    }
  };
  if (!saved_def) {
    std::vector<Fl_Color> def_colrs;
    for (int i = 0; i < 16; i++)
      def_colrs.push_back(xterm_fg_color(i));
    for (int i = 0; i < 16; i++)
      def_colrs.push_back(xterm_bg_color(i));
    palettes.insert(palettes.begin(), def_colrs);
    saved_def = true;
  }
  static size_t curr_index = 0;
  curr_index++;
  if (curr_index >= palettes.size()) curr_index = 0;
  for (int i = 0; i < 16; i++) {
    xterm_fg_color(i, palettes[curr_index][i]);
    xterm_bg_color(i, palettes[curr_index][i + 16]);
  }
  redraw();
}

// Catch GUI-Resize and pass it on to the Bash
void Fl_PTY_Terminal::resize(int X, int Y, int W, int H) {
  // First let the base class change the GUI-Widget size
  bool need_snap = w() != W || h() != H;
  Fl_Terminal::resize(X, Y, W, H);
  if (snap_ && need_snap && !window()->maximize_active()) { // 'snap to character grid' set?
    Fl::remove_timeout(snap_to_grid_cb, this);
    Fl::add_timeout(0.1, snap_to_grid_cb, this);
  }

  set_pty_size();
}

void Fl_PTY_Terminal::set_pty_size() {
  // Only change, when PTY is already active
  if (pty_master_fd_ != -1) {
    // Calculate columns and rows for current size
    int W = w();
    int H = h();
    int columns = display_columns();
    int rows    = display_rows();
    if (log_) {
      fprintf(logfile_, "resized to: %d x %d (%d x %d px)\n", columns, rows, W, H);
    }

    // Prevent invalid values by extreme minimize
    if (columns < 10) {
      columns = 10;
    }
    if (rows < 2) {
      rows = 2;
    }

#ifdef _WIN32
#if HAS_PSEUDO_CONSOLE
    if (_hPC != nullptr) {
      COORD size = { (short)columns, (short)rows };
      // inform Windows ConPTY
      ResizePseudoConsole(_hPC, size);
    }
#endif // HAS_PSEUDO_CONSOLE
#else
    if (pty_master_fd_ >= 0) {
      // Fill OS structure for terminal size
      struct winsize ws;
      ws.ws_col = static_cast<unsigned short>(columns);
      ws.ws_row = static_cast<unsigned short>(rows);
      ws.ws_xpixel = static_cast<unsigned short>(W);
      ws.ws_ypixel = static_cast<unsigned short>(H);

      // Signal to pseudoterminal (and shell) the new size
      ioctl(pty_master_fd_, TIOCSWINSZ, &ws);
    }
#endif
  }
}

void Fl_PTY_Terminal::command(const char *cmd) {
  write_pty(cmd, strlen(cmd));
  write_pty("\n", 1);
}
