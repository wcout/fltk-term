#!/bin/sh

if [ "$#" -ne 0 ]; then
    echo "Error: This script does not accept any arguments." >&2
    exit 1
fi

# local user install under linux

BINARY_PATH=~/bin
ICON_PATH=~/.local/share/icons/hicolor/scalable/apps
DESKTOP_PATH=~/.local/share/applications

install_app()
{
  APPLICATION=$1

  # copy executable
  mkdir -p $BINARY_PATH
  cp -v build/bin/$APPLICATION $BINARY_PATH/.

  # copy svg icon
  mkdir -p $ICON_PATH
  cp -v data/$APPLICATION.svg $ICON_PATH/.

  # copy desktop file
  cp -v data/$APPLICATION.desktop $DESKTOP_PATH/$APPLICATION.desktop
  chmod 0777 $DESKTOP_PATH/$APPLICATION.desktop
}

install_app "fltk-term"
install_app "fltk-term-tabbed"
