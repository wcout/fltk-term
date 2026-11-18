#!/bin/bash

# put cursor to last screen line
lines=$(tput lines)
printf "\e[%d;1H\n" "$lines"
tput cnorm

for i in {1..1000000}; do printf "\e[48;2;$((RANDOM%256));$((RANDOM%256));$((RANDOM%256))m%s\e[0m" " "; sleep 0.02; done
