#ifndef SCREENROTATOR_DBUS_CONNECTOR_H
#define SCREENROTATOR_DBUS_CONNECTOR_H

#include <KDEDModule>
#include <KPluginFactory>
#include "screenrotator.h"



#define _SERVICE_NAME "org.kde.screen.rotation"

class ScreenRotator_DBus_connector : public KDEDModule
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", _SERVICE_NAME)

public:
    explicit ScreenRotator_DBus_connector(QObject *parent, const QVariantList &);
    ~ScreenRotator_DBus_connector();
private:
    ScreenRotator* p_screen_rotator;
#ifdef _DIAGNOSTIC
    QString diagnostic_constructor;
#endif
public slots:
    int     set_orientation(const int &msg);
    QString get_orientation();
    void    set_auto_rotate_state(const bool &msg);
    int     get_auto_rotate_state();
    QString reconfig();
    void    set_matrix_apply_needed(const bool &matrix_needed);
    bool    get_matrix_apply_needed();
    void    set_rotate_quietly(const bool &rotate_quietly);
    bool    get_rotate_quietly();
#ifdef _DIAGNOSTIC
    QString get_diagnostic();
#endif

};

#endif // SCREENROTATOR_DBUS_CONNECTOR_H
