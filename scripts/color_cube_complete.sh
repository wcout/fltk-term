#!/bin/sh

echo "   --- xterm 256-Color Full Palette Test ---"
echo

# ==============================================================================
# 1. STANDARD & BRIGHT SYSTEM COLORS
# ==============================================================================
echo "=== 1. System Colors (Indices 0 - 15) ==="
echo "Standard Colors:"
for idx in $(seq 0 7); do
    printf "\033[48;5;${idx}m %3d \033[0m" "$idx"
done
echo
echo "Bright Colors:"
for idx in $(seq 8 15); do
    printf "\033[48;5;${idx}m %3d \033[0m" "$idx"
done
echo "\n"

# ==============================================================================
# 2. 6x6x6 COLOR CUBE
# ==============================================================================
echo "=== 2. 6x6x6 Color Cube (Indices 16 - 231) ==="
echo

for r in $(seq 0 5); do
    echo "Red Level: $r (index = 16 + 36*$r + 6*g + b)"
    
    for g in $(seq 0 5); do
        for b in $(seq 0 5); do
            index=$((16 + 36*r + 6*g + b))
            printf "\033[48;5;${index}m %3d \033[0m" "$index"
        done
        echo
    done
    echo
done

# ==============================================================================
# 3. GRAYSCALE RAMP
# ==============================================================================
echo "=== 3. Grayscale Ramp (Indices 232 - 255) ==="
echo

for idx in $(seq 232 255); do
    printf "\033[48;5;${idx}m %3d \033[0m" "$idx"
    
    # Split into two rows of 12 for clean alignment
    if [ $(( (idx - 231) % 12 )) -eq 0 ]; then
        echo
    fi
done
echo

