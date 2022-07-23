#include "ScreenRotator_DBus_connector.h"

ScreenRotator_DBus_connector::ScreenRotator_DBus_connector(QObject *parent, const QVariantList &) : KDEDModule(parent)
{
    if(QDBusConnection::sessionBus().registerService(_SERVICE_NAME))
    {
#ifdef _DIAGNOSTIC
        this->diagnostic_constructor+="D-Bus service is registred\n";
        if(QDBusConnection::sessionBus().registerObject("/org/kde/screen/rotation", this, QDBusConnection::ExportNonScriptableSlots))
            this->diagnostic_constructor+="D-Bus Object is registred\n";
        else
            this->diagnostic_constructor+="D-Bus Object is not registred\n";
#else
        QDBusConnection::sessionBus().registerObject("/org/kde/screen/rotation", this, QDBusConnection::ExportNonScriptableSlots);
#endif
    }
#ifdef _DIAGNOSTIC
    else
        this->diagnostic_constructor+="D-Bus service is not registred\n";
#endif
    this->p_screen_rotator = new ScreenRotator(this);
}

ScreenRotator_DBus_connector::~ScreenRotator_DBus_connector()
{
     QDBusConnection::sessionBus().unregisterObject("/org/kde/screen/rotation", QDBusConnection::UnregisterNode);
     QDBusConnection::sessionBus().unregisterService(_SERVICE_NAME);
}

int ScreenRotator_DBus_connector::set_orientation(const int &msg)
{
    return this->p_screen_rotator->set_orientation(msg);
}
QString ScreenRotator_DBus_connector::get_orientation()
{
    return this->p_screen_rotator->get_orientation();
}
void ScreenRotator_DBus_connector::set_auto_rotate_state(const bool &msg)
{
    this->p_screen_rotator->set_sensor_state(msg);
}
int ScreenRotator_DBus_connector::get_auto_rotate_state()
{
    return this->p_screen_rotator->get_sensor_state();
}
QString ScreenRotator_DBus_connector::reconfig()
{
    return this->p_screen_rotator->reconfig();
}
void ScreenRotator_DBus_connector::set_matrix_apply_needed(const bool &matrix_needed)
{
    this->p_screen_rotator->set_matrix_apply_needed(matrix_needed);
}
bool ScreenRotator_DBus_connector::get_matrix_apply_needed()
{
    return this->p_screen_rotator->get_matrix_apply_needed();
}
void ScreenRotator_DBus_connector::set_rotate_quietly(const bool &rotate_quietly)
{
    this->p_screen_rotator->set_rotate_quietly(rotate_quietly);
}
bool ScreenRotator_DBus_connector::get_rotate_quietly()
{
    return this->p_screen_rotator->get_rotate_quietly();
}
#ifdef _DIAGNOSTIC
QString ScreenRotator_DBus_connector::get_diagnostic()
{
    QString str="DBus_connector\n\n" + this->diagnostic_constructor + "\n\nScreenRotator\n\n";
    str+=this->p_screen_rotator->get_diagnostic();
    return str;
}
#endif

K_PLUGIN_FACTORY(ScreenRotatorFactory,
                 registerPlugin<ScreenRotator_DBus_connector>();)
#include "ScreenRotator_DBus_connector.moc"
