#!/bin/sh

printf "Cursor color red\n"
printf "\e]12;rgb:ff/00/00\a"
sleep 4
printf "== Cursor style (syntax without space):\n"
printf "Block no blinking\n"
printf "\e[2q"
sleep 4
printf "Vertical line, blinking:\n"
printf "\e[5q"
sleep 4
printf "== Cursor style (syntax with space):\n"
printf "Block no blinking\n"
printf "\e[2 q"
sleep 4
printf "Vertical line, blinking:\n"
printf "\e[5 q"
sleep 4
printf "Horizontal line, blinking:\n"
printf "\e[3 q"
sleep 4
printf "Default cursor (syntax with SP):\n"
printf "\e[ q"
printf "Default cursor (syntax without SP):\n"
printf "\e[0 q"
printf "Cursor color green\n"
printf "\e]12;#00ff00\a"
sleep 4
printf "Cursor color black\n"
printf "\e]12;#000000\a"
