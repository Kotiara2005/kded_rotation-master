# kded_rotation

Copyright 2016 Sebastian Krzyszkowiak <dos@dosowisko.net>

Edited in 2022 by Golubov Konstantin <Kotiara2005@ukr.net>

KDED module for handling automatic screen rotation on X11, with visual feedback before orientation change happens. Some assembly might be required.

# Installation

Run `./install_kded_rotation.sh` and install missing dependencies as needed. 

You'll most likely need  `xrandr`, `xinput`, `libQt5Core-devel`, `qt5-qtbase-devel`, `cmake-utils`, `extra-cmake-modules`, `iio-sensor-proxy`, `qt5-qtsensors` and `kf5-kded-devel`. Depending on your distribution, these packages might have different names. CMake will tell you which packages it is missing.

# Usage
During installation, the configuration file “/etc/kded_rotation/kded_rotation.conf” is automatically created, common to the entire system, and when the service is started, configuration files are created individually for each user, they are located in the “User Home Folder/.config/kded_rotation.conf” and have priority over the “/etc/kded_rotation/kded_rotation.conf”.

If you need to calibrate your touch screen, then make sure you have the `xinput` utility installed on your system and the option “MATRIX_NEED_TO_APPLY:1” in the configuration file, and also set the "Coordinate Transformation Matrix" you need. If your touch screen does not require calibration, set “MATRIX_NEED_TO_APPLY:0” this will allow, albeit slightly, to save system resources, for tablets this may well be a tangible savings.

In KDE 5, after the next update, KDE began to apply "Coordinate Transformation Matrix 1 0 0 0 1 0 0 0 1" for any screen position after rotation, to replace it with a custom one, a "TIME_DELAY" delay in seconds was added in the configuration file to wait for the application of the standard matrix and only after that apply a custom one, the duration of the delay may need to be adjusted depending on the power of your device and, accordingly, the time it spends on all procedures associated with screen rotation. Also after this KDE update all "Coordinate Transformation Matrix" should be the same and match "Matrix_normal".

Also, the service has an interface for communicating with it via Dbus, this can be done through the qdbus or qdbus-qt5 utility, possibly qdbus-qt6 if you have qt6 installed in my distribution, it is included in the libqt5-qdbus package. Type in the command line "qdbus org.kde.screen.rotation /org/kde/screen/rotation org.kde.screen.rotation.function argument".
qdbus org.kde.screen.rotation /org/kde/screen/rotation org.kde.screen.rotation.set_auto_rotate_state 1 enables auto screen rotation.
qdbus org.kde.screen.rotation /org/kde/screen/rotation org.kde.screen.rotation.set_auto_rotate_state 0 disables auto screen rotation.
Functions:

"set_orientation argument" sets the screen position manual to argument 1 TopUp, 2 TopDown, 3 LeftUp, 4 RightUp.

"get_orientation" Returns the current screen position. First character "O" auto screen rotation enabled "F" auto screen rotation disabled, second character means screen position "T"= TopUp, "D"= TopDown,
"R"=RightUp,"L"= LeftUp, the third character is always "0".

"set_auto_rotate_state argument" Argument is 1 auto rotate enable, argument is 0 disable.

"get_auto_rotate_state" Returns the current auto rotation state, 1 enabled, 0 disabled.

"reconfig" Causes the service to reread the configuration files and apply the new settings without restarting the service.

"set_matrix_apply_needed argument" Sets whether to apply a custom "Coordinate Transformation Matrix". If the argument is 1 matrix apply if 0 is not applied.

"get_matrix_apply_needed" Returns 1 if a custom "Coordinate Transformation Matrix" is applied and 0 if not.

"set_rotate_quietly argument" Argument 1 sets visual feedback, argument 0 removes visual feedback.

"get_rotate_quietly" Returns 1 if visual feedback is enabled and 0 if visual feedback is disabled.

This interface is convenient to use together with the plasma widget "Configurable button".
To enable, set
qdbus org.kde.screen.rotation /org/kde/screen/rotation org.kde.screen.rotation.set_auto_rotate_state 1; sleep2; exit 0
To turn off
qdbus org.kde.screen.rotation /org/kde/screen/rotation org.kde.screen.rotation.set_auto_rotate_state 0; sleep1; exit 0
To check the state when the widget starts
service=$(qdbus org.kde.screen.rotation /org/kde/screen/rotation org.kde.screen.rotation.get_auto_rotate_state); if [[ $service -eq 1 ]] ; then exit 0; else exit 1; fi
Also in the folder there are icons that you can replace the standard icons for the on and off button, they are made by me completely and you can use them as you wish.
