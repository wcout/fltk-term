#!/bin/bash

export LANG=en_US.UTF-8

# Get the max iterations from the first argument, or default to 5000
MAX_ITERATIONS=${1:-5000}

clear
tput civis

# Restore normal screen behavior if the user aborts early with Ctrl+C
trap 'tput cnorm; clear; exit' INT TERM

chars=(
    "█" "▓" "▒" "░" "■" "□" "▲" "▼" "◀" "▶" "◆" "◇" "●" "○"
    "★" "☆" "✦" "✧" "♠" "♣" "♥" "♦" "♩" "♪" "♫" "♬" "⚛" "⚙"
    "➔" "➘" "➙" "➚" "➛" "➜" "➝" "➞" "➟" "➠" "➡" "➢" "➣" "➤"
    "α" "β" "γ" "δ" "ε" "ζ" "η" "θ" "ι" "κ" "λ" "μ" "ν" "ξ"
    "π" "ρ" "σ" "τ" "υ" "φ" "χ" "ψ" "ω" "∑" "∏" "√" "∞" "∩"
)
num_chars=${#chars[@]}

# Configuration for scrolling
SCROLL_THRESHOLD=300  # Scroll after printing this many characters
scroll_counter=0
total_counter=0

# Loop runs exactly N times
while [ "$total_counter" -lt "$MAX_ITERATIONS" ]; do
    lines=$(tput lines)
    cols=$(tput cols)

    # Scroll the screen if the scroll threshold is met
    if [ "$scroll_counter" -ge "$SCROLL_THRESHOLD" ]; then
        printf "\e[r\e[5S"
        scroll_counter=0
    fi

    # Pick random screen position
    x=$((RANDOM % cols + 1))
    y=$((RANDOM % lines + 1))

    color=$((1 + RANDOM % 255))
    char="${chars[RANDOM % num_chars]}"

    # Print the character
    printf "\e[%d;%dH\e[38;5;%dm%s\e[0m" "$y" "$x" "$color" "$char"

    ((scroll_counter++))
    ((total_counter++))
#    sleep 0.002
done

# --- Clean Up ---
# Move cursor to the very bottom line so the shell prompt returns cleanly
printf "\e[%d;1H\n" "$lines"
tput cnorm

