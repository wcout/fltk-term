#!/bin/bash

# Test subparameter with ':' to set underline mode (curly line) in red color (58:2::255:0:0)
# => fltk-term draws only a normal underline, but parsing should produce no artifacts

printf "\x1b[4m\e[38;5;130mNormal underline classic style\x1b[0m\x1b[4mNormal underline classic style\x1b[0m\n"

printf "\x1b[4:3m\x1b[58:2::255:0:0mCurly red text\x1b[59m\x1b[4:0m\n"

printf "\x1b[4:3m\x1b[58:2::0:255:0mCurly green text\x1b[59m\x1b[4:0m\n"

printf "\x1b[4:3m\x1b[58:2::0:0:255mCurly blue text\x1b[59m\x1b[4:0m\n"

printf "\x1b[4:3m\x1b[58:2::0:0:0mCurly rgb black text\x1b[59m\x1b[4:0m\n"

printf "\x1b[4:3m\x1b[58:5:0mCurly indexed black text\x1b[59m\x1b[4:0m\n"

printf "\x1b[4:3m\x1b[58:5:1mCurly indexed red text\x1b[59m\x1b[4:0m\n"

printf "\x1b[4:3m\x1b[58:5:0mCurly gray text\x1b[59m\x1b[4:0m\n"

printf "\x1b[4:1m\x1b[58:5:0mBlack normal underlined text\x1b[59m\x1b[4:0m\n"

printf "\x1b[4:2m\x1b[58:5:1mRed double underlined text\x1b[59m\x1b[4:0m\n"

printf "\x1b[4:4m\x1b[58:2::0:0:255mBlue Dotted text\x1b[59m\x1b[4:0m\n"

printf "\x1b[4:5m\x1b[58:2::0:255:0mGreen dashed text\x1b[59m\x1b[4:0m\n"

printf "\x1b[21mDouble underline classic style\x1b[0m\n"

printf "\x1b[4mNormal underline classic style\x1b[0m\n"

printf "\nERROR TESTING\n";

printf "\x1b[4:3m\x1b[58:5:-10mNegative  color index\x1b[59m\x1b[4:0m\n"

printf "\x1b[4:3m\x1b[58:5:270mInvalid color index\x1b[59m\x1b[4:0m\n"
