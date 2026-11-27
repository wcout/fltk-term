/*

   Fully workable FLTK interactive terminal based on
   an improved 'Fl_Terminal' widget.

   (c) 2026 wcout

   It is running many applications like editors (vim) and
   tools like w3m or btop nearly perfectly.

   Initially written for Linux only, but later made
   cross platform using Google Gemini (and much rework!).

   Under Windows (tested only on WIN11), it uses a reader thread
   to fetch the output to the terminal without blocking UI.
   On Linux/macOs it uses the Fl::add_fd() callback mechanism.

*/
#include <FL/Fl_PTY_Terminal.H>
#include <string>
#include <sstream>
#include <fstream>
#include <ios>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_File_Chooser.H>

#if HAS_PSEUDO_CONSOLE

#include <FL/Fl_Preferences.H>
#include <FL/filename.H>
#include <FL/fl_utf8.h>
static const int BORDER = 0; // border between window and terminal widget
static int log_ = 0;
static int no_splash = 0;
static FILE *logfile = nullptr;
static int no_set_term = 0;
static int history_lines = -1; // -1: Fl_Terminal default (100)
static int columns = 80;
static int rows = 25;
static Fl_Color color = fl_rgb_color(FL_WHITE);
static int fontsize = 0;
static std::string fontname;
static int alt_colors = 0;
static std::string cmd; // initial command (none)
static std::string splash; // splash screen
static Fl_Preferences prefs(Fl_Preferences::USER_L, APPNAME, APPNAME);

void parse_command_line(int argc, char *argv[]) {
  auto geometry = [&](const std::string &s, int &W, int &H) ->void {
    size_t pos = s.find('x');
    if (pos == std::string::npos) {
      return;
    }
    int w = atoi(s.substr(0, pos).c_str());
    int h = atoi(s.substr(pos + 1).c_str());
    if (w > 10 && w <= 132 && h > 10 && h <= 50) {
      W = w;
      H = h;
    }
  };
  auto set_font =[&](const char *font) ->int {
    fontname = font;
    int fontsize = 0;
    size_t pos;
    if ((pos = fontname.find("::")) != std::string::npos) {
      fontsize = atoi(&fontname.c_str()[pos + 2]);
      fontname.erase(pos);
    }
    if (fontname.size()) {
      Fl::set_font(FL_COURIER, fontname.c_str());
    }
    return fontsize;
  };
  auto test_arg =[&](const char *arg, char v, int &var) -> bool {
    size_t n = 0;
    if (arg[0] == '-') {
      while (arg[n + 1] == v) {
        var++;
        n++;
      }
    }
    return n;
  };

  for (int i = 1; i < argc; i++) {
    if (argv[i][0] != '-') {
      cmd = argv[i];
    } else {
      if (std::string(argv[i]) == "--help") {
        fprintf(stderr, "Usage:\n"
                "-D ....... dark mode\n"
                "-F name .. use font (optional append ::size)\n"
                "-L name .. log to file\n"
                "-l[l] .... log input [and output]\n"
                "-c ....... use alternate color set (GTK style)\n"
                "-g WxH ... geometry width x height chars\n"
                "-h n ..... use n history lines\n"
                "-s ....... don't show splash screen\n"
                "-t ....... don't set TERM=xterm-256color\n");
        fprintf(stderr, "preferences file: %s\n", fl_getenv("FLTK_TERM_PREFS"));
        exit(0);
      }
      char o = argv[i][1];
      if (o == 'D') {
        color = 0x10101000; // dark mode
      }
      test_arg(argv[i], 'l', log_);
      test_arg(argv[i], 't', no_set_term);
      test_arg(argv[i], 's', no_splash);
      test_arg(argv[i], 'c', alt_colors);
      if (i + 1 < argc) { // options with parameter
        switch (o) {
          case 'h':
            if (argv[++i][0] >= '0' && argv[i][0] <= '9') {
              history_lines = atoi(argv[i]);
            }
            break;
          case 'g':
            geometry(argv[++i], columns, rows);
            break;
          case 'F':
            fontsize = set_font(argv[++i]);
            break;
          case 'L':
            logfile = fopen(argv[++i], "w");
            break;
        }
      }
    }
  }
} //parse_command_line

const std::string create_splash(Fl_PTY_Terminal &term) {
  std::ostringstream os;
  std::string pad = std::string(std::max((term.display_columns() - 80) / 2, 0), ' ');
  os << "\x1b[?1049h";                    // use alternate buffer
  os << "\x1b[2J\x1b[H\x1b[?25l\x1b[?7l"; // cls+home, cursor off, wrap off
  os << "\n";
  os << "\x1b[34m";
  os << pad << "    ███████╗██╗     ████████╗██╗  ██╗    ████████╗███████╗██████╗ ███╗   ███╗\n";
  os << pad << "    ██╔════╝██║     ╚══██╔══╝██║ ██╔╝    ╚══██╔══╝██╔════╝██╔══██╗████╗ ████║\n";
  os << pad << "    █████╗  ██║        ██║   █████╔╝        ██║   █████╗  ██████╔╝██╔████╔██║\n";
  os << pad << "    ██╔══╝  ██║        ██║   ██╔═██╗        ██║   ██╔══╝  ██╔══██╗██║╚██╔╝██║\n";
  os << pad << "    ██║     ███████╗   ██║   ██║  ██╗       ██║   ███████╗██║  ██║██║ ╚═╝ ██║\n";
  os << pad << "    ╚═╝     ╚══════╝   ╚═╝   ╚═╝  ╚═╝       ╚═╝   ╚══════╝╚═╝  ╚═╝╚═╝     ╚═╝\n";
  os << "\x1b[32m\n";
  os << "\x1b[K\n";
  os << pad << "          ╔═╗    ╔═╗      ╔═══╗╔═══╗╔═══╗╔═══╗                       ╔╗\x1b[K\n";
  os << pad << "         ╔╝╔╝    ╚╗╚╗     ║╔═╗║║╔═╗║║╔═╗║║╔══╝                      ╔╝╚╗\x1b[K\n";
  os << pad << "        ╔╝╔╝ ╔══╗ ╚╗╚╗    ╚╝╔╝║║║ ║║╚╝╔╝║║╚══╗    ╔╗╔╗╔╗╔══╗╔══╗╔╗╔╗╚╗╔╝\x1b[K\n";
  os << pad << "        ║║║  ║╔═╝  ║║║    ╔═╝╔╝║║ ║║╔═╝╔╝║╔═╗║    ║╚╝╚╝║║╔═╝║╔╗║║║║║ ║║\x1b[K\n";
  os << pad << "        ║║║  ║╚═╗  ║║║    ║║╚═╗║╚═╝║║║╚═╗║╚═╝║    ╚╗╔╗╔╝║╚═╗║╚╝║║╚╝║ ║╚╗\x1b[K\n";
  os << pad << "        ╚╗╚╗ ╚══╝ ╔╝╔╝    ╚═══╝╚═══╝╚═══╝╚═══╝     ╚╝╚╝ ╚══╝╚══╝╚══╝ ╚═╝\x1b[K\n";
  os << pad << "         ╚╗╚╗    ╔╝╔╝\x1b[K\n";
  os << pad << "          ╚═╝    ╚═╝\x1b[K\n";
  os << "\x1b[0m";
  os << "\n";
  os << "\n";
  os << pad << "=== 1. System Colors (Indices 0 - 15) ===\n";
  os << pad << "Standard Colors:\n";
  os << pad << "[48;5;0m   0 [0m[48;5;1m   1 [0m[48;5;2m   2 [0m[48;5;3m   3 [0m[48;5;4m   4 [0m[48;5;5m   5 [0m[48;5;6m   6 [0m[48;5;7m   7 [0m\n";
  os << pad << "Bright Colors:\n";
  os << pad << "[48;5;8m   8 [0m[48;5;9m   9 [0m[48;5;10m  10 [0m[48;5;11m  11 [0m[48;5;12m  12 [0m[48;5;13m  13 [0m[48;5;14m  14 [0m[48;5;15m  15 [0m";
  os << "\x1b[0m";
  os << "\x1b[?2026h";  // sync on
  return os.str();
}

#include <FL/Fl_Scrollbar.H>
#include <FL/Fl_Slider.H>
#include <FL/Fl_Group.H>

/**
  A scrollbar that changes the color of its handle
  during dragging to a user definable color.
*/
class MyScrollbar : public Fl_Scrollbar {
static const Fl_Color NoColor = (Fl_Color)-1;
typedef Fl_Scrollbar Inherited;
public:
  MyScrollbar(int x, int y, int w, int h, const char *L = nullptr) :
    Inherited(x, y, w, h, L),
    dragging_(false),
    within_(false),
    slider_selection_color_(NoColor),
    selection_color_(NoColor),
    slider_hoover_color_(NoColor) {
    slider_hoover_color_ = fl_darker(Fl_Widget::color());
    slider(FL_FLAT_BOX);
  }
  int handle(int e) override {
    switch (e) {
      case FL_PUSH:
        dragging_ = true;
        if (slider_selection_color_ != NoColor) {
           selection_color(slider_selection_color_);
        }
        break;
      case FL_LEAVE:
        if (within_) { // FL_LEAVE also from leaving window
          if (slider_hoover_color_ != NoColor) {
            selection_color(selection_color_);
          }
          redraw();
        }
        within_ = false;
        break;
      case FL_RELEASE:
        dragging_ = false;
        if (slider_selection_color_ != NoColor && selection_color_ != NoColor) {
          selection_color(slider_hoover_color_);
        }
        redraw();
        break;
      case FL_ENTER:
        within_ = true;
        if (slider_hoover_color_ != NoColor) {
          selection_color_ = selection_color();
          selection_color(slider_hoover_color_);
          redraw();
        }
        break;
    }
    return Inherited::handle(e);
  }
  void color(Fl_Color color, Fl_Color slider_color, Fl_Color slider_selection_color) {
    Fl_Widget::color(color, slider_color);
    slider_selection_color_ = slider_selection_color;
    slider_hoover_color_ = fl_darker(selection_color());
  }
  static void replace_scrollbar(Fl_Scrollbar **pscrollbar) {
    Fl_Scrollbar *s = *pscrollbar;
    Fl_Group *g = s->parent();
    MyScrollbar *cs = new MyScrollbar(s->x(), s->y(), s->w(), s->h());
    cs->type(s->type());                    // copy all relevant values
    cs->value(s->value());
    cs->linesize(s->linesize());
    cs->callback(s->callback(), s->user_data());
    *pscrollbar = cs;
    g->add(cs);                             // add custom scrollbar to original widget
    delete s;                               // delete current scrollbar
  }
private:
  bool dragging_;
  bool within_;
  Fl_Color slider_selection_color_;
  Fl_Color selection_color_;
  Fl_Color slider_hoover_color_;
};

class MyTerminal : public Fl_PTY_Terminal {
public:
  MyTerminal(int x, int y, int w, int h, const char *l = 0) : Fl_PTY_Terminal(x, y, w, h, l) {}
  void search_dialog();
  bool is_selection() const { return Fl_Terminal::is_selection(); }
  bool alternate_buffer() const { return Fl_Terminal::alternate_buffer(); }
};

void MyTerminal::search_dialog() {
  static const int W = 370;
  static const int H = 100;
  Fl_Double_Window *win = new Fl_Double_Window(W, H, "Search history");
  Fl_Input *input = new Fl_Input(60, 10, 300, 26, "Search:");
  Fl_Button *up   = new Fl_Button(60, 60, 130, 30, "@8->");
  Fl_Button *down = new Fl_Button(230, 60, 130, 30, "@2->");
  input->tooltip("Give a search string - Enter searches upwards");
  up->selection_color(FL_SELECTION_COLOR);
  down->selection_color(FL_SELECTION_COLOR);
  Fl_Box *not_found = new Fl_Box(60, 35, 300, 26, "Not found!");
  not_found->labelcolor(FL_RED);
  not_found->hide();
  not_found->align(FL_ALIGN_CENTER|FL_ALIGN_INSIDE);
  input->take_focus();
  input->when(FL_WHEN_ENTER_KEY|FL_WHEN_NOT_CHANGED);
  static int row, col;
  row = -1;

  auto static do_search =[](Fl_Widget *wgt, void *d, bool direction) {
    MyTerminal *term = static_cast<MyTerminal *>(d);
    Fl_Input *input = (Fl_Input *)(wgt->window()->child(0));
    if (!input->value()[0]) return;
    ((Fl_Button *)wgt)->value(1);
    wgt->redraw();
    Fl::wait();
    term->search(input->value(), row, col, direction) ?
      input->window()->child(3)->hide() : input->window()->child(3)->show();
    ((Fl_Button *)wgt)->value(0);
  };

  input->callback([](Fl_Widget *wgt, void *d) {
    MyTerminal *term = static_cast<MyTerminal *>(d);
    Fl_Button *up = (Fl_Button *)(wgt->window()->child(1));
    up->do_callback(up, term); // Enter acts like 'up' button
  }, this);
  up->callback([](Fl_Widget *wgt, void *d) {
    do_search(wgt, d, false);
  }, this);
  down->callback([](Fl_Widget *wgt, void *d) {
    do_search(wgt, d, true);
  }, this);
  win->end();
  win->set_modal();
  win->show();
  while (win->shown()) {
    Fl::wait();
  }
  delete win;
  if (alternate_buffer()) clear_mouse_selection();
}

static void search_dialog_cb(Fl_Widget *wgt, void *d) {
  MyTerminal *term = static_cast<MyTerminal *>(d);
  term->search_dialog();
}

static void save_selection_cb(Fl_Widget *wgt, void *d) {
  MyTerminal *term = static_cast<MyTerminal *>(d);
  const char *name = fl_file_chooser("Save selection to:", "*.txt", "selection.txt", true);
  if (name) {
     const char *s = term->selection_text();
     std::string sel(s);
     free((void *)s);
     if (sel.size() && !std::all_of(sel.begin(), sel.end(),
       [](unsigned char c) {
         return std::isspace(c);
       })) {
       std::ofstream ofs(name, std::ios::out);
       ofs << sel;
     } else {
       fl_alert("Selection is empty!");
     }
  }
}

static void context_menu(MyTerminal *term) {
  Fl_Menu_Item menu[10];
  menu[0].text = 0;
  menu->add(" Search history...", 0, (Fl_Callback *)search_dialog_cb);
  if (term->is_selection() && !term->alternate_buffer()) {
    menu->add(" Save selection...", 0, (Fl_Callback *)save_selection_cb);
  }
  fl_cursor(FL_CURSOR_DEFAULT);
  const Fl_Menu_Item *m = menu->popup(Fl::event_x(), Fl::event_y());
  if (m) {
    m->do_callback(0, term);
  }
}

int main(int argc, char** argv) {
  char buf[FL_PATH_MAX];
  prefs.filename(buf, sizeof(buf) - 1);
  std::string prefs_env = "FLTK_TERM_PREFS=" + (std::string)buf;
  fl_putenv(prefs_env.c_str());
  prefs.get("columns", columns, 80);
  prefs.get("rows", rows, 25);
  int c;
  prefs.get("color", c, fl_rgb_color(FL_WHITE));
  color = (Fl_Color)c;
  prefs.get("alt_colors", alt_colors, 0);
  prefs.get("fontname", fontname, "");
  prefs.get("fontsize", fontsize, 0);
  prefs.get("history-lines", history_lines, -1);
  parse_command_line(argc, argv);
#ifdef _WIN32
  Fl::lock();
#else
  setenv("HISTCONTROL", "ignorespace", 1); // don't put splash commands in history
  if (!no_set_term) {
    setenv("TERM", "xterm-256color", 1);
  }
#endif

  // Replaces standard 3D up/down boxes with flat equivalents globally
  Fl::background(240, 240, 240);
  Fl::set_color(FL_SELECTION_COLOR, 0x72, 0x9f, 0xcf);
  Fl::set_boxtype(FL_UP_BOX, FL_BORDER_BOX);
  Fl::set_boxtype(FL_DOWN_BOX, FL_BORDER_BOX);
  Fl::set_boxtype(FL_THIN_UP_BOX, FL_BORDER_BOX);
  Fl::set_boxtype(FL_THIN_DOWN_BOX, FL_BORDER_BOX);

  // we must create terminal on heap, otherwise FLTK's auto-deleter chokes...
  MyTerminal *terminal = new MyTerminal(0, 0, 100, 100); // create smaller than will be later
  MyTerminal &term = *terminal;
  if (!term.inited()) {
    fprintf(stderr, "Failed to initialize PTY backend\n");
    return 1;
  }
  MyScrollbar::replace_scrollbar(&term.scrollbar);
  term.box(FL_FLAT_BOX);
  term.margin(2);
  term.snap(true); // allow 'snap to character grid'
  term.logging(log_, logfile ? logfile : stderr);
  term.textsize(fontsize > 0 ? fontsize : 18);
  term.hscrollbar_style(Fl_Terminal::SCROLLBAR_OFF);
  term.scrollbar_size(6);
  ((MyScrollbar*)term.scrollbar)->color(FL_WHITE, 0x99999900, FL_RED);// set custom handle colors
  if (alt_colors) term.toggle_default_colors(); // switch to GTK colors

  // calculate how the big the window must be for the terminal
  int term_w, term_h;
  term.char_to_pixel_dimension(columns, rows, term_w, term_h);

  // create window in just the right size
  Fl_Double_Window win(term_w + 2 * BORDER, term_h + 2 * BORDER, "FLTK Terminal");
  win.xclass(APPNAME);
  win.add(term); // now add the terminal to it
  win.resizable(term);

  // Send inital terminal size to PTY after window was shown
  term.resize(BORDER, BORDER, term_w, term_h); // resize to full window size
  win.end();
  win.show(/*argc, argv*/);

  ::printf("PTTY %d x %d\n", term.display_columns(), term.display_rows());

  term.callback([](Fl_Widget *wgt, void *d) {
    const char *buf = (const char *)d;
    Fl_PTY_Terminal *term = (Fl_PTY_Terminal *)wgt;
    int reason = Fl::callback_reason();
    switch (reason) {
      case Fl_Terminal::FL_REASON_TITLE_CHANGE: {  // window title has changed
        static std::string window_title;
        if (window_title != buf) {
          window_title = buf;
          term->window()->copy_label(window_title.c_str());
        }
        return;
      }
      case Fl_Terminal::FL_REASON_COLOR_CHANGE:    // background color changed
        term->scrollbar->color(term->color());
        term->window()->color(term->color());
        term->window()->redraw();
        return;
      case Fl_Terminal::FL_REASON_SIZE_CHANGE:     // terminal size changed by reset
        term->window()->size(term->w() + 2 * BORDER, term->h() + 2 * BORDER);
        return;
      case Fl_PTY_Terminal::FL_REASON_PTY_STOPPED: // bash quitted
        term->window()->hide();
        return;
      case Fl_PTY_Terminal::FL_REASON_RIGHT_MOUSE:
        Fl::add_timeout(0., [](void *d) {
          context_menu(static_cast<MyTerminal *>(d));
        }, term);
        return;
      default:                                     // ANSI command not implemented
        if (log_ && buf && reason >= Fl_Terminal::FL_REASON_NOT_IMPL) {
         fprintf(logfile ? logfile : stderr, "Fl_Terminal NOT IMPLEMENTED (%d): ", reason);
         term->dump(buf, strlen(buf));
        }
    }
  });

  term.color(color); // change color AFTER setting callback
  if (history_lines >= 0) {
    term.history_lines(history_lines);
  }
  if (cmd.empty() && !no_splash) {
    splash = create_splash(term);
  }
  if (cmd.size() || splash.size()) {
    // delay command to let terminal initialize completely before
    Fl::add_timeout(0.2, [](void *d) {
      Fl_PTY_Terminal *term = (Fl_PTY_Terminal *)d;
      cmd.size() ? term->command(cmd.c_str()) : term->append_utf8(splash.c_str());
      if (splash.size()) {
        bool wait = true;
        Fl::add_timeout(3., [](void *d) {
          bool *wait = (bool *)d;
          *wait = false;
        }, &wait);
        while (wait) Fl::check();
        // cleanup after splash      FIXME: how to better regain prompt on all platforms
        term->append_utf8("\033c"); // reset
#ifndef _WIN32
        term->command("clear");     // restore bash prompt
#else
        term->command("\n");        // that works for cmd.exe
#endif
      }
    }, terminal);
  }

  int result = Fl::run();

  // Clean-up
  if (logfile) {
    fclose(logfile);
  }
  prefs.set("columns", terminal->display_columns());
  prefs.set("rows", terminal->display_rows());
  prefs.set("color", (int)terminal->color());
  prefs.set("alt_colors", alt_colors);
  prefs.set("fontname", fontname.c_str());
  prefs.set("fontsize", fontsize);
  prefs.set("history-lines", history_lines);
  delete terminal;
  return result;
}

#else // HAS_PSEUDO_CONSOLE
#include <FL/fl_ask.H>
int main() {
  fl_message_title("fltk_term");
  fl_alert("WIN32 version not supported!\n(need WIN10/6 or higher)");
}
#endif
