#!/bin/sh

clear
printf "The following text should appear after 5 seconds!\n"
sleep 1

printf "\e[?2026h"
printf "\n***SYNCED***\n"
sleep 4
printf "\e[?2026l"

