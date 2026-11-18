/*

  A simple POC of putting Fl_PTY_Terminals into a tabbed interface.

  Add tabs with ALT-'t' or the '+' tab, remove tab entering 'exit'.
  Switch between tabs with ALT-'0'... ALT-'9'.

  Bonus: Shift+Click on any colored character to 'edit' it's color.
  ALT-'w' to write the changed palette out to 'palette.h'.

  (c) 2026 wcout

*/

// TODO: show tabs only after second term?
// TODO: color of tabs background change?
// TODO: Title of tabs (from window title?)

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Tabs.H>
#include <FL/fl_draw.H>
#include <FL/fl_ask.H>
#include <FL/filename.H>
#include <FL/Fl_Color_Chooser.H>
#include <FL/Fl_Preferences.H>
#include <FL/filename.H>
#include <cstdio>
#include <algorithm>
#include <ios>
#include <fstream>

#include <FL/Fl_PTY_Terminal.H>

static constexpr int MaxTabs = 9;
static inline Fl_Color xterm_rgb_color(uchar r, uchar g, uchar b) {
  return (Fl_Color)(((((r << 8) | g) << 8) | b) << 8);
}

namespace Args {
  static std::string cmd;
  static int no_set_term = 0;
  static int history_lines = -1; // -1: Fl_Terminal default (100)
  static int COLS = 100;
  static int ROWS = 30;
  static Fl_Color color = fl_rgb_color(FL_WHITE);
  static int fontsize = 0;
  static std::string fontname;
};

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
    Args::fontname = font;
    int fontsize = 0;
    size_t pos;
    if ((pos = Args::fontname.find("::")) != std::string::npos) {
      fontsize = atoi(&Args::fontname.c_str()[pos + 2]);
      Args::fontname.erase(pos);
    }
    if (Args::fontname.size()) {
      Fl::set_font(FL_COURIER, Args::fontname.c_str());
    }
    return fontsize;
  };
  for (int i = 1; i < argc; i++) {
    if (argv[i][0] != '-') {
      Args::cmd = argv[i];
    } else {
      if (std::string(argv[i]) == "--help") {
        fprintf(stderr, "Usage:\n"
                "-D ....... dark mode\n"
                "-F name .. use font (optional append ::size)\n"
                "-g WxH ... geometry width x height chars\n"
                "-h n ..... use n history lines\n"
                "-t ....... don't set TERM=xterm-256color\n");
        fprintf(stderr, "preferences file: %s\n", fl_getenv("FLTK_TERM_TABBED_PREFS"));
        exit(0);
      }
      char o = argv[i][1];
      if (o == 'D') {
        Args::color = 0x10101000; // dark mode
      } else if (o == 't') {
        Args::no_set_term = true;
      }
      if (i + 1 < argc) { // options with parameter
        switch (o) {
          case 'h':
            if (argv[++i][0] >= '0' && argv[i][0] <= '9') {
              Args::history_lines = atoi(argv[i]);
            }
            break;
          case 'g':
            geometry(argv[++i], Args::COLS, Args::ROWS);
            break;
          case 'F':
            Args::fontsize = set_font(argv[++i]);
            break;
        }
      }
    }
  }
} //parse_command_line

class FltkTerm : public Fl_PTY_Terminal {
public:
  FltkTerm(int x, int y, int w, int h, const char *l = 0) : Fl_PTY_Terminal(x, y, w, h, l) {
    if (!inited()) {
      fprintf(stderr, "FltkTerm: Failed to initialize PTY backend\n");
      return;
    }
    box(FL_FLAT_BOX);
    margin(2);
    margin_top(6);
    snap(true); // allow 'snap to character grid'
    if (Args::fontsize > 0) {
      textsize(Args::fontsize);
    }
    scrollbar->slider(FL_FLAT_BOX);
    hscrollbar_style(Fl_Terminal::SCROLLBAR_OFF);
    scrollbar_size(6);
    scrollbar->color(FL_WHITE, FL_GRAY);// set custom handle colors
  }
  bool alternate_buffer() const  { return Fl_Terminal::alternate_buffer(); }
  bool scroll_region_set() const { return Fl_Terminal::scroll_region_set(); }

  int lowest_free_child_id(Fl_Tabs *tabs) {
    std::vector<std::string> ids = {"1", "2", "3", "4", "5", "6", "7", "8", "9" };
    for (int i = 0; i < tabs->children() - 1; i++) {
      std::string id = std::string((const char *)tabs->child(i)->user_data());
      auto ele = std::find(ids.begin(), ids.end(), id);
      if (ele != ids.end()) ids.erase(ele);
    }
    if (ids.size()) return atoi(ids[0].c_str());
    return 0;
  }

  static FltkTerm* new_terminal_tab(Fl_Tabs *tabs) {
    int rx, ry, rw, rh;
    tabs->client_area(rx, ry, rw, rh);
    FltkTerm *term = new FltkTerm(tabs->x() + rx, tabs->y() + ry, rw, rh);
    term->Fl_Terminal::color(Args::color);
    if (Args::history_lines >= 0) {
      term->Fl_Terminal::history_lines(Args::history_lines);
    }
    tabs->insert(*term, tabs->value());
    static const std::string ids[] = { "1", "2", "3", "4", "5", "6", "7", "8", "9" };
    const std::string &id = ids[term->lowest_free_child_id(tabs) - 1];
    term->copy_label((std::string("FltkTerm ") + id).c_str());
    term->user_data((void*)id.c_str());
    tabs->value(term);
    tabs->value(tabs->children() - 2); // highlight new tab
    tabs->value()->take_focus();
    term->size(rw, rh);
    if (Args::cmd.size()) {
      // delay command to let terminal initialize completely before
      Fl::add_timeout(0.2, [](void *d) {
      Fl_PTY_Terminal *term = (Fl_PTY_Terminal *)d;
      term->command(Args::cmd.c_str());
      term->command("\n");        // that works for cmd.exe
      }, term);
    }
    return term;
  }

  bool set_default_palette(const std::vector<Fl_Color> &palette) {
    std::vector<Fl_Color> pal = palette;
    if (pal.size() != 8 && pal.size() != 16 && pal.size() != 32) {
      fprintf(stderr, "palette has wrong size: %d\n", (int)pal.size());
      return false;
    }
    for (size_t i = 0; i < pal.size(); i++) {
      uchar r, g, b;
      Fl::get_color(pal[i], r, g, b);
      pal[i] = xterm_rgb_color(r, g, b);
    }
    if (pal.size() == 8) {
       pal.resize(16, (Fl_Color)0);
       for (size_t i = 0; i < 8; i++) {
         pal[i + 8] = fl_color_average(pal[i], 0xffffff00, 0.5);
       }
    }
    for (auto c : pal) {
      if (c == 0x0 || c > 0xff) ;
      else {
        fprintf(stderr, "palette color 0x%08X is invalid!\n", c);
        return false;
      }
    }
    for (size_t i = 0; i < 16; i++) {
      xterm_fg_color(i, pal[i]);
      if (pal.size() == 16)
        xterm_bg_color(i, pal[i]);
      else
        xterm_bg_color(i, pal[i + 16]);
    }
    redraw();
    return true;
  }

  void choose_color(bool bg = false) {
    uchar r, g, b;
    Fl::get_color(color_, r, g, b);
    char msg[100];
    snprintf(msg, sizeof(msg), "Change %s for index %d", (bg ? "bgcolor" : "fgcolor"), index_);
    int ret = fl_color_chooser(msg, r, g, b, 2);
    if (ret == 1) {
      if (bg ) xterm_bg_color(index_, xterm_rgb_color(r, g, b));
      else     xterm_fg_color(index_, xterm_rgb_color(r, g, b));
      redraw();
    }
  }

  int handle(int e) override {
    if (e == FL_PUSH && Fl::event_state() & FL_SHIFT) {
      // Shift-Click to select a character for color edit
      int grow, gcol;
      bool gcr;
      if (xy_to_glob_rowcol(Fl::event_x(), Fl::event_y(), grow, gcol, gcr) == 1) {
        const Utf8Char *u8c = u8c_ring_row(grow) + gcol;
        int index;
        if ((index = u8c->fgcolor_index()) >= 0 && Fl::event_button() == FL_LEFT_MOUSE) {
          color_ = u8c->fgcolor();
          index_ = index;
          Fl::add_timeout(0.,[](void *d) { ((FltkTerm *)d)->choose_color(); }, this);
          return 1;
        } else if ((index = u8c->bgcolor_index()) >= 0 && Fl::event_button() == FL_RIGHT_MOUSE) {
          color_ = u8c->bgcolor();
          index_ = index;
          Fl::add_timeout(0.,[](void *d) { ((FltkTerm *)d)->choose_color(true); }, this);
          return 1;
        } else fl_beep();
      }
    }
    if (e == FL_KEYDOWN && Fl::event_state() & FL_ALT) {
      Fl_Tabs *tabs = (Fl_Tabs *)window()->child(0);
      if (Fl::event_key() == 't') {
        // remove ALT-t keystroke from Fl_PTY_Terminal to create new tab
        if (tabs->children() <= MaxTabs) {
          tabs->value(tabs->children() - 1);
          FltkTerm::new_terminal_tab(tabs);
        }
        return 1;
      } else if (Fl::event_key() >= '1' && Fl::event_key() <= '9') {
        // remove ALT-0 - ALT -9 keystroke from Fl_PTY_Terminal to switch to tab
        const char key = (const char)Fl::event_key();
        std::string id = std::string(&key, 1);
        for (int i = 0; i < tabs->children() - 1; i++) {
          if (std::string((const char *)tabs->child(i)->user_data()) == id) {
            // activate tab with 'id'
            tabs->value(tabs->child(i));
            tabs->value()->take_focus();
            break;
          }
        }
        return 1;
      } else if (Fl::event_key() == 'w') {
        std::fstream f("palette.h", std::ios::out);
        f << "static const std::vector<Fl_Color> palette = {\n";
        for (int i = 0; i < 16; i++) {
          Fl_Color c = xterm_fg_color(i);
          f << "0x" << std::hex << c << ",\n";
        }
        for (int i = 0; i < 16; i++) {
          Fl_Color c = xterm_bg_color(i);
          f << "0x" << std::hex << c << ",\n";
        }
        f << "};\n";
        f.close();
        Fl::add_timeout(0.,[](void *d) {
          std::string msg = fl_getcwd_str();
          fl_alert("%s", (msg + "/palette.h written").c_str());
        }, this);
        return 1;
      }
    }
    return Fl_PTY_Terminal::handle(e);
  }

  void notify(int reason, const std::string &buf) override {
//    fprintf(stderr, "notify %d\n", reason);
    if (!window()) return;                         // not every FltkTerm may have a window
    switch (reason) {
      case Fl_Terminal::FL_REASON_TITLE_CHANGE: {  // window title has changed
        if (window_title_ != buf) {
          window_title_ = buf;
          window()->copy_label(window_title_.c_str());
        }
        return;
      }
      case Fl_Terminal::FL_REASON_COLOR_CHANGE:    // background color changed
        scrollbar->color(color());
//        window()->color(color());
//        window()->redraw();
        return;
      case Fl_Terminal::FL_REASON_SIZE_CHANGE: {   // terminal size changed by reset
        // terminal has now snapped to grid and is w() x h() size.
        // terminal is in tab client rect
        Fl_Tabs *tabs = (Fl_Tabs *)parent();
        int rx, ry, rw, rh;
        tabs->client_area(rx, ry, rw, rh);
        int W = rw;
        int H = rh + ry;
        if (W == window()->w() && H == window()->h()) {
          return;
        }
        window()->size(W, H);
        return;
      }
      case Fl_PTY_Terminal::FL_REASON_PTY_STOPPED: // bash quitted
        Fl_Tabs *tabs = (Fl_Tabs *)parent();
        if (tabs->children() == 2) { // last terminal?
          tabs->window()->hide();    // dont't remove last terminal (needed for prefernces)
          return;
        }
        tabs->remove(this);          // unlink terminal from tab
        delete this;                 // and delete it
        //fprintf(stderr, "after close: %d\n", tabs->children());
        if (tabs->children() >= 2) {
          tabs->value(tabs->children() - 2);
          tabs->value()->take_focus();
        }
        return;
    }
  }
  const std::string &window_title() const { return window_title_; }
private:
	std::string window_title_;
   int index_;
   Fl_Color color_;
};

#if HAS_PSEUDO_CONSOLE

int main(int argc, char** argv) {
  char buf[FL_PATH_MAX];
  prefs.filename(buf, sizeof(buf) - 1);
  std::string prefs_env = "FLTK_TERM_TABBED_PREFS=" + (std::string)buf;
  fl_putenv(prefs_env.c_str());
  prefs.get("columns", Args::COLS, 100);
  prefs.get("rows", Args::ROWS, 30);
  prefs.get("fontname", Args::fontname, "");
  prefs.get("fontsize", Args::fontsize, 0);
  prefs.get("history-lines", Args::history_lines, -1);
  parse_command_line(argc, argv);
#ifdef _WIN32
  Fl::lock();
#else
  setenv("HISTCONTROL", "ignorespace", 1); // don't put splash commands in history
  if (!Args::no_set_term) {
    setenv("TERM", "xterm-256color", 1);
  }
#endif
  // 1) determine tabs ry
  int rx, ry, rw, rh;
  Fl_Tabs *tabs = new Fl_Tabs(0, 0, 100, 100);
  fl_font(tabs->labelfont(), tabs->labelsize()); // kick loading font system
  tabs->client_area(rx, ry, rw, rh);
  delete tabs;

  // 2) determine terminal dimensions for COLS x ROWS
  int tw, th;
  FltkTerm *temp = new FltkTerm(0, 0, 100, 100);     // use a tempory object just to
  temp->char_to_pixel_dimension(Args::COLS, Args::ROWS, tw, th); // call 'char_to_pixel_dimension' on it
#ifndef _WIN32
  delete temp; // WIN32 does not like this! => disturbs cmd.exe initialization
#endif

  // 3) size window appropriate
  int W = tw;
  int H = th + ry;
  Fl_Window *win = new Fl_Double_Window(W, H);
  Fl_Group *resizable = nullptr;

  // 4) create first terminal and a '+' tab
  tabs = new Fl_Tabs(0, 0, win->w(), win->h());
  tabs->selection_color(FL_DARK3);
  tabs->labelcolor(FL_WHITE);
  tabs->box(FL_FLAT_BOX);
  win->color(FL_WHITE);
  tabs->client_area(rx, ry, rw, rh);
  Fl_Group *add = new Fl_Group(tabs->x() + rx, tabs->y() + ry, rw, rh, "+");
  add->user_data((void*)"+");
  add->selection_color(FL_YELLOW);
  add->end();
  FltkTerm *term = FltkTerm::new_terminal_tab(tabs);
#if 0
#include "palette.h"
#else
  static const std::vector<Fl_Color> palette = {
    // fltk
    FL_BLACK, fl_darker(FL_RED), fl_darker(FL_GREEN), fl_darker(FL_YELLOW),
    0x12488800, fl_darker(FL_MAGENTA), fl_darker(FL_CYAN), 0xdddddd00
  };
#endif
  term->set_default_palette(palette);
  term->toggle_default_colors();
  resizable = add;
  tabs->end();
  tabs->resizable(resizable);
  win->resizable(tabs);
  win->xclass(APPNAME);
  win->end();
  win->show();
  tabs->callback([](Fl_Widget *wgt, void *d) {
    Fl_Tabs *tabs = (Fl_Tabs *)wgt;
    Fl_Window *win  = (Fl_Window *)d;
    std::string id = tabs->value()->user_data() ? (const char *)tabs->value()->user_data() : "";
    if (id == "+") {
      if (tabs->children() > MaxTabs) {
        tabs->value(tabs->children() - 2);
        tabs->value()->take_focus();
        return;
      }
      Fl::add_timeout(0.,[](void *d) {
        Fl_Tabs *tabs = (Fl_Tabs *)d;
        FltkTerm::new_terminal_tab(tabs);
      }, tabs);
      return;
    }
    FltkTerm *term = (FltkTerm *)tabs->value();
    win->copy_label(term->window_title().c_str());
    term->take_focus();
  }, win);
  win->callback([](Fl_Widget *wgt, void *d) { // window close callback
    // poor mans check, if terminals might run a process currently
    Fl_Window *win  = (Fl_Window *)wgt;
    Fl_Tabs *tabs = (Fl_Tabs *)d;
    for (int i = 0; i < tabs->children() - 1; i++) {
      FltkTerm *term = (FltkTerm *)tabs->child(i);
      // if a terminal application uses the alternate buffer or a scroll region
      // the terminal is definitely not idling currenty...
      if (term->alternate_buffer() || term->scroll_region_set()) {
        fl_alert("There are open processes.\n");
        return; // don't close window
      }
    }
    // close window
    win->hide();
  }, tabs);

  int ret = Fl::run();

  // access first terminal (which is guaranteed to be left)
  term = (FltkTerm *)tabs->child(0);
  //fprintf(stderr, "%d x %d\n", term->display_columns(), term->display_rows());
  prefs.set("columns", term->display_columns());
  prefs.set("rows", term->display_rows());
  prefs.set("fontname", Args::fontname.c_str());
  prefs.set("fontsize", Args::fontsize);
  prefs.set("history-lines", Args::history_lines);

  delete win; // check delete
  return ret;
}
#else // HAS_PSEUDO_CONSOLE
#include <FL/fl_ask.H>
int main() {
  fl_message_title("fltk_term");
  fl_alert("WIN32 version not supported!\n(need WIN10/6 or higher)");
}
#endif
