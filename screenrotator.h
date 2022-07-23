#ifndef SCREENROTATOR_H
#define SCREENROTATOR_H

#include <QObject>
#include <QOrientationSensor>
#include <QTimer>
#include <QProcess>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QString>
#include <QStringList>
#include <QFile>
#include <QDir>
#include <QTextStream>

//#define _DIAGNOSTIC

class ScreenRotator : public QObject
{
    Q_OBJECT
public:
    explicit       ScreenRotator(QObject *parent = nullptr);
                   ~ScreenRotator();

    int            set_orientation(const int &msg);
    QString        get_orientation();
    void           set_sensor_state(const bool &msg);
    const bool&    get_sensor_state();
    QString        reconfig();
    void           set_matrix_apply_needed(const bool matrix_needed);
    const bool&    get_matrix_apply_needed();
    void           set_rotate_quietly(bool rotate_quietly);
    const bool&    get_rotate_quietly();
#ifdef _DIAGNOSTIC
    QString        get_diagnostic();
#endif

private:
    QOrientationSensor *sensor;
    QTimer* timer_before_start;
    QProcess* helper;
    QOrientationReading::Orientation currentOrientation;
    int progress;
    int orientation;
    bool swch;
    bool matrix_needed;
    bool rotate_quietly;
    int time_before_start;
    int time_delay;
    QString display;
    QString touch_screen;
    QStringList matrixs[4];
    QStringList xrandr_args;
    QStringList xinput_args;
#ifdef _DIAGNOSTIC
    QString diagnostic_constructor;
    QString diagnostic_updateOrientation;
    QString diagnostic_update_xinput;
    QString diagnostic_read_config;
    QString diagnostic_validate_config;
    QString diagnostic_sensor_satate;
    QString save_too_config_state;
#endif
private:
    QString read_config(int& answer);
    QString validate_config(int& answer);
    QString save_to_config(int& answer);
    bool    create_config_file();
    void    sensor_state();
public slots:
    void updateOrientation();
    void startProgress();
    void updateProgress();
    void update_xinput();
};


#endif // SCREENROTATOR_H
