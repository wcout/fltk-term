#!/usr/bin/env bash

# ANSI Escape Sequences
printf -v c_clear "\e[2J\e[H"
printf -v c_hide_cursor "\e[?25l"
printf -v c_show_cursor "\e[?25h"
printf -v c_sync_start "\e[?2026h"
printf -v c_sync_end "\e[?2026l"

# Get terminal dimensions
COLS=$(tput cols 2>/dev/null || echo 80)
ROWS=$(tput lines 2>/dev/null || echo 24)

# Generate a row of colored text to slow down the rendering slightly
make_row() {
    local row_num=$1
    local row=""
    for ((c=1; c<=COLS; c++)); do
        # Create a changing color pattern
        local color=$(( (row_num + c) % 6 + 31 ))
        row+="\e[${color}m#\e[0m"
    done
    echo -e "$row"
}

echo "Pre-generating frame data..."
mapfile -t FRAME < <(for ((r=1; r<ROWS; r++)); do make_row "$r"; done)

# --- Test 1: WITHOUT Synchronized Output ---
echo -e "${c_clear}--- TEST 1: WITHOUT Synchronized Mode ---"
echo "Watch how the screen fills up line-by-line (or flashes)..."
sleep 2

echo -e "$c_hide_cursor"
for row in "${FRAME[@]}"; do
    echo -e "$row"
    sleep 0.02 # Artificial delay to simulate heavy CLI rendering
done
echo -e "$c_show_cursor"

sleep 3

# --- Test 2: WITH Synchronized Output ---
echo -e "${c_clear}--- TEST 2: WITH Synchronized Mode ---"
echo "If supported, the frame below will appear INSTANTLY all at once."
sleep 2

# 1. Start synchronization (terminal freezes updates)
echo -e "$c_sync_start$c_hide_cursor"

for row in "${FRAME[@]}"; do
    echo -e "$row"
    sleep 0.02 # Same delay, but terminal should buffer it hidden
done

# 2. End synchronization (terminal flushes everything onto the screen at once)
echo -e "$c_sync_end$c_show_cursor"

echo -e "\n\nTest finished."

