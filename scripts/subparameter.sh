#!/bin/bash

# Test subparameter with ':' to set underline mode (curly line) in red color (58:2::255:0:0)
# => fltk-term draws only a normal underline, but parsing should produce no artifacts
printf "\x1b[4:3m\x1b[58:2::255:0:0mCurly red error\x1b[59m\x1b[4:0m\n"

printf "\x1b[4:3m\x1b[58:2::0:255:0mCurly green error\x1b[59m\x1b[4:0m\n"

printf "\x1b[4:3m\x1b[58:2::0:0:255mCurly blue error\x1b[59m\x1b[4:0m\n"

printf "\x1b[4:3m\x1b[58:2::0:0:0mCurly rgb black error\x1b[59m\x1b[4:0m\n"

printf "\x1b[4:3m\x1b[58:5:0mCurly indexed black error\x1b[59m\x1b[4:0m\n"

printf "\x1b[4:3m\x1b[58:5:1mCurly indexed red error\x1b[59m\x1b[4:0m\n"

printf "\x1b[4:3m\x1b[58:5:250mCurly gray error\x1b[59m\x1b[4:0m\n"

printf "\nERROR TESTING\n";

printf "\x1b[4:3m\x1b[58:5:-10mNegative  color index\x1b[59m\x1b[4:0m\n"

printf "\x1b[4:3m\x1b[58:5:270mInvalid color index\x1b[59m\x1b[4:0m\n"
