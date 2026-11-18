# Changes to Fl\_Terminal\_Widget

- Dynamic default fg color (using fl_contrast to background color) *
    - required, when background changes, text became unreadable
    - test with 'fltk-term' by using `F12` to toggle the background

- Added support for xterm color mode '5' `<ESC>[38|48;5;index;m` (color cube) *
    - `vim` uses it
    - run `scripts/color_cube_complete.sh` (or cat `scripts/color_cube_complete.txt` to terminal)

- Added support for bright fg/background colors (`<ESC>[9x`, `<ESC>[10x`) *
    - run `scripts/bright_colors.sh` (or `cat scripts/bright_colors.txt` to terminal)

- Refactor ringbuffer to use std::vector<Utf8Char> instead malloc'ed array *
    - needed to implement alternate buffer

- Support for scroll region (top/bottom line) *
    - used by nearly all editors and tools (vim, htop, micro,..)

- Added alternate buffer support `<ESC>[?1049h/l` *
    - also needed for most fullscreen apps, like editors

- Added cursor on/off support *
    - e.g. `top` hides the cursor

- Added keyboard mode support (DECCKM) `<ESC>[?1h/l` (switch ANSI/VT100 mode to Application mode) *
    - needed to make cursor keys work for all apps: Editors enable it e.g. `<ESC>[A` -> `<ESC>0A`

- Added basic origin mode support *
    - (maybe incomplete, seems not widely used)

- Added "mouse tracking" `ESC[?1006h/l`: => applications (like editors) can use the mouse to select/copy/paste text *
    - full mouse support in editors (e.g. 'mcedit', 'micro'), tools like 'btop',...

- Added line graphics support *
    - see e.g. `nudoku` game

- Added support for `ESC[#d (VPA)` line pos absolute [row] *
    - needed for e.g. `htop`

- Added OSC (`<ESC>]val;BEL`) parsing *
    - needed to filter out many currently not handled sequences

- Added method to access EscSeq buffer !
    - needed for logging invalid sequences

- `\007` is not ignored, but used for fl_beep() or OSC parsing end marker !
    - like in other shells beeps, when cursor hits edges

- Support `<ESC>7` and `<ESC>8` als alternatives to `<ESC>[s` `<ESC>[u` (save/restore cursor) !
    - used by some tools 

- Some other ESC sequences parsed -
    - to ignore then properly e.g. `ESC=`, `ESC>`, `ESC)B`, `ESC(B`

- Make use of callback to "report" invalid sequences to application !
    - lets application log sequences, terminal does not handle or are invalid

- Bugfixes?: print_char() FIX cursor_right(): let cursor position overhang line,
  advance only just before next character is printed. -
    - because `micro` (editor) has a problem with the status line
    - otherwise problem with command line wrapping (long command vanishes
      beyond right margin,...)

- Support "set background" (OSC 11xx) and also reporting current background -

- Accept (but no support) private mode 12 "blinking cursor" -
   - **NOTE**: Is in some "conflict" with `\e[# q` (set cursor style).
      According to AI, overrules blinking, which is set through style.
      But in test with gnome terminal it has no effect and a blinking
      style stays, even when disabled afterwards with OSC 12.

- Added "synced buffer" support (private mode 2026) !
    - Try `scripts/sync_test.sh`

- Parse (but do not handle currently) soft reset command `ESC[!p` (DECSTR) !

- Support of cursor color via OSC 12 !

- Support of cursor styles also with blinking cursor !
    - Try `scripts/cursor_style.sh`

- Parse (but do not handle) "bracketed paste mode" (private mode 2004)
    - This is needed in `Fl_PTY_Terminal` to paste payload -

- Implement special BCE (backround color erase) mode to remove -
  "color bleeding" into auto-wrapped scrolled in new line.
    - Try `scripts/scroll_color.sh` and watch the line overflow without color bleeding (BCE)
    - Try the same with `xterm` and see how the color spills into the next line (xterm has no BCE)

- Implement auto_wrap OFF for cursor at end of line *

- Implement insert_mode `<ESC>[4h/l` partially *

- Implemented "dynamic cursor color" (cursor color changes with underlying character color)
    - optionally via API call (disabled by default) !

- Changed selection coloring (line Gnome terminal) !
    - Old implementation did not work well with dynamic screen backgrounds
    - optional via API call (disabled by default)

- Fix tabstops not set on column change *
    - also refactored to std::vector<bool>

- Fix background color in bold mode (dont't make it brighter); fixes `dialog` -

- Allow Cursor mode `ESC[5q`, which allows a space (SP) between 5 and 'q' `ESC[5 SP q` (WIN11 editor) -

- Fix FL_FOCUS: Don't test visible_focus(), always return 1 *
    - The original code returned 1 for the FL_FOCUS event (indicating "I want focus")
      only when visible_focus is set. In reality we always want focus, if we are asked for.
      Otherwise when visible focus is globally disabled with `Fl::visible_focus(0)` a terminal
      would never get the focus (and in case of interactive terminal would not be able to input).

- Implement "repeat last character" `ESC[##b` *

- Implement `ESC[#a`, `ESC#c`, which are commonly identical to `ESC[#C` and `ESC[#B` !

- added margin(value) comfort method to set all margins to same value !

- Change application cursor to "Insert" when hoovering above text !
    - Only when no application grabbed the screen
    - To indicate "selection is possible"

- Implemented "snap to character grid" feature (not enabled by default) *
    - After resizing (it´s container) it snaps/adjusts to full character edges

- Implement "query device attributes" and "device status" (just enough to make somen applications happy) -

- Implement "middle mouse paste" -

- Fix handling `\000` in data (stopped parsing) -

- Changed the way indexed colors 0..15 are stored in Utf8Char (as index, not rgb-value) !

- Support runtime change of palette !
    - test with `test/fltk-term-tabbed` (Shift-Click on a character to change it's palette color)


```

*) important changes IMHO

-) minor changes, nevertheless very usefull e.g. to 'ignore' ESC-sequences properly

!) "nice to have" changes

```
