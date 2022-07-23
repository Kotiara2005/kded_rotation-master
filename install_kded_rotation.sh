#!/bin/bash

DIR=$(pwd)
BUILD_DIR=$DIR/build

function prepare_application {
  rm -rf "$BUILD_DIR"
  mkdir -p "$BUILD_DIR"
}

function clean
{
  rm -rf "$BUILD_DIR"
  rm -f "$DIR/kded_rotation.conf"
}

function build_application {
  if cd "$BUILD_DIR"; then

    display=$(xrandr | grep primary | awk '{print $1}')
    if [ -z "$display" ]
    then
      echo "Installation failed, could not determine the main display,"
      echo "check if the xrandr utility is installed in the system and KDE is running under X11, not under Wayland."
      clean
      exit 1
    fi
    sensor_screen=$(xinput list | grep -i 'Touchscreen\|ELAN\|Pen\|Eraser\|wacom\|maXTouch\|eGalaxTouch\|IPTS')
    sensor_screen=$(sed -n 's/[^A-Za-z0-9]\+\(.*\)[[:space:]]\{2,\}id.*/\1/p' <<< $sensor_screen)
    if [ -z "$sensor_screen" ]
    then
      echo "Installation error, unable to detect touchscreen,"
      echo "check if xinput is installed on the system and KDE is running under X11, not Wayland."
      echo "You can continue with the installation but then you will not be able"
      echo "to use \"Coordinate Transformation Matrix\" to calibrate your touch screen."
      read -p "continue installation anyway? [y/n] " INSTALL_CHOICE
      echo ""
      if [[ $INSTALL_CHOICE =~ ^[Yy]$ ]]; then
        TOUCH_SCREEN="#TOUCH_SCREEN:Name of your device"
        MATRIX_NEED_TO_APPLY="MATRIX_NEED_TO_APPLY:0"
      else
        echo "Stop installation"
        clean
        exit 1
      fi
      else
        TOUCH_SCREEN="TOUCH_SCREEN:$sensor_screen"
        MATRIX_NEED_TO_APPLY="MATRIX_NEED_TO_APPLY:1  #If you are satisfied with the default matrix, set 0 otherwise 1"
      fi
      echo "DISPLAY:$display" 1>../kded_rotation.conf    #/etc/kded_rotation/kded_rotation.conf
      echo "$TOUCH_SCREEN" 1>>../kded_rotation.conf
      echo "TIME_BEFORE_START:1.5  #seconds before screen rotation, floating point number" 1>>../kded_rotation.conf
      echo "TIME_DELAY:1.0  #seconds before applying the matrix, floating point number" 1>>../kded_rotation.conf
      echo "$MATRIX_NEED_TO_APPLY" 1>>../kded_rotation.conf
      echo "SCREEN_ROTATOR_ON:1 #If you want automatic screen rotation set to 1 otherwise 0" 1>>../kded_rotation.conf
      echo "ROTATE_QUIETLY:0 #If you want to be warned before the screen is rotated set to 1 otherwise 0" 1>>../kded_rotation.conf
      echo "#After some update of KDE possible the value of all matrices should match Matrix_normal" 1>>../kded_rotation.conf
      echo "#because kde itself changes coordinates only for the graphical environment only" 1>>../kded_rotation.conf
      echo "Matrix_normal:1 0 0 0 1 0 0 0 1" 1>>../kded_rotation.conf
      echo "Matrix_inverted:-1 0 1 0 -1 1 0 0 1" 1>>../kded_rotation.conf
      echo "Matrix_left:0 -1 1 1 0 0 0 0 1" 1>>../kded_rotation.conf
      echo "Matrix_right:0 1 0 -1 0 1 0 0 1" 1>>../kded_rotation.conf

    cmake ../. \
      -DCMAKE_INSTALL_PREFIX=/usr \
      -DCMAKE_BUILD_TYPE=Release \
      -DLIB_INSTALL_DIR=lib \
      -DLIBEXEC_INSTALL_DIR=lib \
      -DKDE_INSTALL_USE_QT_SYS_PATHS=ON

      make
  fi
}

function install_application {
  if cd "$BUILD_DIR"; then
    read -p "Run sudo make install? [y/n] " INSTALL_CHOICE
    echo ""
    if [[ $INSTALL_CHOICE =~ ^[Yy]$ ]]; then
      sudo make install
      sudo chmod +x /usr/bin/orientation-helper

    fi
  fi
}

function main {
  prepare_application
  build_application
  install_application
}

main

exit 0
