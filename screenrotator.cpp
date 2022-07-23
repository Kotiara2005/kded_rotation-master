#include "screenrotator.h"

#define _NO_DISPLAY 1
#define _NO_TOUCH_SCREEN 2
#define _NO_NOT_CREETICAL_PARAM 4

ScreenRotator::ScreenRotator(QObject *parent) : QObject(parent)
{
#ifdef _DIAGNOSTIC
    this->diagnostic_constructor+="\nFunction ScreenRotator(QObject *parent) is runing\n";
#endif
    this->sensor = nullptr;
    this->timer_before_start = nullptr;
    this->helper = new QProcess();
    this->orientation=0;
    this->swch=false;
    this->rotate_quietly=false;
    this->matrix_needed=false;
    int config_answer=0;
    this->read_config(config_answer);
    if( -2 == config_answer )
        this->create_config_file();
    this->validate_config(config_answer);
#ifdef _DIAGNOSTIC
    this->diagnostic_constructor+="config_answer=" + QString::number(config_answer) + "\n";
    this->diagnostic_constructor+="_NO_DISPLAY=" + QString::number(config_answer & _NO_DISPLAY) + "\n";
    this->diagnostic_constructor+="_NO_TOUCH_SCREEN=" + QString::number(config_answer & _NO_TOUCH_SCREEN) + "\n";
#endif
    if( !( config_answer & _NO_DISPLAY ) && ( !( config_answer & _NO_TOUCH_SCREEN ) || !this->matrix_needed ) )
        this->sensor_state();
    if(this->swch)
        updateOrientation();
    if(this->matrix_needed)
    {
        this->currentOrientation = QOrientationReading::TopUp;
        QTimer::singleShot(5000, this, &ScreenRotator::update_xinput);
    }
    this->progress = -1;

}

ScreenRotator::~ScreenRotator()
{
    int i=0;
    this->save_to_config(i);
    if( this->swch )
    {
        this->sensor->stop();
        delete this->sensor;
    }
    if( !this->rotate_quietly )
        delete this->timer_before_start;
    delete this->helper;
}


void ScreenRotator::startProgress() {
    if ( ( this->progress == -1 || this->orientation>0 ) && !this->xrandr_args.isEmpty() )
    {
        this->timer_before_start->start(this->time_before_start);
        this->progress = 0;
    }
}

void ScreenRotator::updateProgress() {
    if ( this->xrandr_args.isEmpty() || ( !sensor->reading() && this->orientation<1 ) )
    {
        this->timer_before_start->stop();
        progress = -1;
        return;
    }
    if ((sensor->reading()->orientation() != this->currentOrientation && this->swch) || this->orientation>0)
    {
        progress++;

        QDBusMessage msg = QDBusMessage::createMethodCall(
            QStringLiteral("org.kde.plasmashell"),
            QStringLiteral("/org/kde/osdService"),
            QStringLiteral("org.kde.osdService"),
            QStringLiteral("mediaPlayerVolumeChanged")
        );

        msg.setArguments({progress, "screen", "view-refresh"});

        QDBusConnection::sessionBus().asyncCall(msg);

        if (progress == 100)
        {
            updateOrientation();
            this->timer_before_start->stop();
            progress = -1;
        }

    }
    else
    {
        this->timer_before_start->stop();
        progress = -1;
    }
}

void ScreenRotator::updateOrientation()
{
    #ifdef _DIAGNOSTIC
    this->diagnostic_updateOrientation.clear();
    this->diagnostic_updateOrientation="\nФункция updateOrientation запустилась\n";
    #endif
    if ( this->xrandr_args.isEmpty() || ( !sensor->reading() && this->orientation<1 ) ) return;
    if(this->orientation<5 && this->orientation>0)
    {
        #ifdef _DIAGNOSTIC
        this->diagnostic_updateOrientation+="this->currentOrientation = this->orientation\n";
        #endif
        this->currentOrientation = QOrientationReading::Orientation(this->orientation);
    }
    else
    {
        #ifdef _DIAGNOSTIC
        this->diagnostic_updateOrientation+="this->currentOrientation = sensor->reading()->orientation()\n";
        #endif
        this->currentOrientation = sensor->reading()->orientation();
    }
    QStringList tmp_xrandr=this->xrandr_args;
    switch (this->currentOrientation)
    {
        case QOrientationReading::TopUp:
            tmp_xrandr<<"normal";
            #ifdef _DIAGNOSTIC
            this->diagnostic_updateOrientation+="this->currentOrientation == QOrientationReading::TopUp\n";
            #endif
            break;
        case QOrientationReading::TopDown:
            tmp_xrandr<<"inverted";
            #ifdef _DIAGNOSTIC
            this->diagnostic_updateOrientation+="this->currentOrientation == QOrientationReading::TopDown\n";
            #endif
            break;
        case QOrientationReading::LeftUp:
            tmp_xrandr<<"left";
            #ifdef _DIAGNOSTIC
            this->diagnostic_updateOrientation+="this->currentOrientation == QOrientationReading::LeftUp\n";
            #endif
            break;
        case QOrientationReading::RightUp:
            tmp_xrandr<<"right";
            #ifdef _DIAGNOSTIC
            this->diagnostic_updateOrientation+="this->currentOrientation == QOrientationReading::RightUp\n";
            #endif
            break;
        default:
            #ifdef _DIAGNOSTIC
            this->diagnostic_updateOrientation+="currentOrientation имеет значение вне допустимого диапазона 1-4\n";
            this->diagnostic_updateOrientation+="currentOrientation=";
            this->diagnostic_updateOrientation+=QString::number(this->currentOrientation)+"\n";
            #endif
            return;
    }
    //////////////////////////////////////////////////////////////////////
    // Зпуск программ для переворота экрана и применения калибровочной матрици
    /////////////////////////////////////////////////////////////////////
    #ifdef _DIAGNOSTIC
    this->diagnostic_updateOrientation+="\nАргументы для xrandr: \n";
    this->diagnostic_updateOrientation+=tmp_xrandr.join(", ")+"\n";
    #endif
    this->helper->setProgram("xrandr");
    this->helper->setArguments(tmp_xrandr);
    this->helper->start();
    if(!this->helper->waitForFinished())
    {
        this->helper->kill();
        #ifdef _DIAGNOSTIC
        this->diagnostic_updateOrientation+="xrandr dont finished, killed manualy\n";
        #endif
    }
    #ifdef _DIAGNOSTIC
    this->diagnostic_updateOrientation+="Стандартные ошибки xrandr: \n";
    this->diagnostic_updateOrientation+=this->helper->readAllStandardError() + "\n";
    this->diagnostic_updateOrientation+="Стандартный вывод xrandr: \n";
    this->diagnostic_updateOrientation+=this->helper->readAllStandardOutput()+"\n";
    #endif
    this->orientation=0;
    if(this->matrix_needed)
        QTimer::singleShot(this->time_delay, this, &ScreenRotator::update_xinput);

}

void ScreenRotator::update_xinput()
{
#ifdef _DIAGNOSTIC
    this->diagnostic_update_xinput="\n\nFunction update_xinput() is started\n";
#endif
    QStringList tmp_xinput=this->xinput_args;
    switch (this->currentOrientation) {
        case QOrientationReading::TopUp:
#ifdef _DIAGNOSTIC
            this->diagnostic_update_xinput+="this->currentOrientation == QOrientationReading::TopUp\n";
#endif
            tmp_xinput+=this->matrixs[0];
            break;
        case QOrientationReading::TopDown:
#ifdef _DIAGNOSTIC
            this->diagnostic_update_xinput+="this->currentOrientation == QOrientationReading::TopDown\n";
#endif
            tmp_xinput+=this->matrixs[1];
            break;
        case QOrientationReading::LeftUp:
#ifdef _DIAGNOSTIC
            this->diagnostic_update_xinput+="this->currentOrientation == QOrientationReading::LeftUp\n";
#endif
            tmp_xinput+=this->matrixs[2];
            break;
        case QOrientationReading::RightUp:
#ifdef _DIAGNOSTIC
            this->diagnostic_update_xinput+="this->currentOrientation == QOrientationReading::RightUp\n";
#endif
            tmp_xinput+=this->matrixs[3];
            break;
        default:
#ifdef _DIAGNOSTIC
            this->diagnostic_update_xinput+="this->currentOrientation out of range 1-4 \n";
            this->diagnostic_update_xinput+="this->currentOrientation == ";
            this->diagnostic_update_xinput+=QString::number(this->currentOrientation)+"\n";
#endif
            return;
    }
#ifdef _DIAGNOSTIC
    this->diagnostic_update_xinput+="Аргументы для xinput: \n";
    this->diagnostic_update_xinput+=tmp_xinput.join(", ")+"\n";
#endif
    this->helper->setProgram("xinput");
    this->helper->setArguments(tmp_xinput);
    this->helper->start();
    if(!this->helper->waitForFinished())
    {
        this->helper->kill();
#ifdef _DIAGNOSTIC
        this->diagnostic_update_xinput+="xinput dont finished, killed manualy\n";
#endif
    }
#ifdef _DIAGNOSTIC
    this->diagnostic_update_xinput+="Стандартные ошибки xinput: \n";
    this->diagnostic_update_xinput+=this->helper->readAllStandardError()+"\n";
    this->diagnostic_update_xinput+="Стандартный вывод xinput: \n";
    this->diagnostic_update_xinput+=this->helper->readAllStandardOutput()+"\n";
#endif
    //delete this->helper;
    //this->helper = new QProcess();
    this->orientation=0;
}

#ifdef _DIAGNOSTIC
//Функция для диагностики слот DBus возвращает строку с информацией по последнему запуску внешних команд
QString ScreenRotator::get_diagnostic()
{
    QString errors="Diagnostic information: \n";
    errors+=this->diagnostic_constructor;
    errors+="\n\nFrom read_config()\n\n" + this->diagnostic_read_config;
    errors+="\n\nFrom validate_config()\n\n" + this->diagnostic_validate_config;
    errors+=this->diagnostic_updateOrientation;
    errors+=this->diagnostic_update_xinput;
    errors+= this->diagnostic_sensor_satate + "\n\n";
    return errors;
}
#endif

//Задать ориентацию экрана
int ScreenRotator::set_orientation(const int &msg)
{
    if(msg>0&&msg<5)
    {
        orientation=msg;
        if(this->rotate_quietly)
            this->updateOrientation();
        else
            this->startProgress();
        return 0;
    }
    return 1;
}

//Задать считывание сенсора для изименения ориентации экрана
void ScreenRotator::set_sensor_state(const bool &msg)
{
    this->swch=msg;
    this->sensor_state();
    if(msg)
    {
        if(this->rotate_quietly)
            this->updateOrientation();
        else
            this->startProgress();
    }
}

//Узнать включена ли орентация по сенсору
const bool& ScreenRotator::get_sensor_state()
{
    return this->swch;
}

//Получить рекущее состояние сенсора ориентации
QString ScreenRotator::get_orientation()
{
    QString str;
    if(this->swch)
        str+="O";
    else
        str+="F";
    switch(this->currentOrientation)
    {
        case QOrientationReading::LeftUp:
            str+="L";
            break;
        case QOrientationReading::RightUp:
            str+="R";
            break;
        case QOrientationReading::TopDown:
            str+="D";
            break;
        case QOrientationReading::TopUp:
            str+="T";
            break;
        default:
            str+="0";
    }
    str+="0";
    return str;
}

//Чтение конфигурации из файла
QString ScreenRotator::read_config(int& answer)
{
    QString diagnostic_read_config;
    diagnostic_read_config="\n\nFunction read_config() is started\n";
    this->display.clear();
    this->touch_screen.clear();
    this->matrixs[0].clear();
    this->matrixs[1].clear();
    this->matrixs[2].clear();
    this->matrixs[3].clear();
    this->xrandr_args.clear();
    this->xinput_args.clear();
    QStringList config_files;
    config_files<<"/etc/kded_rotation/kded_rotation.conf";
    config_files<<QDir::homePath()+"/.config/kded_rotation.conf";
    QFile file;
    for( int config_index=0 ; config_index < config_files.size() ; config_index++ )
    {
        file.setFileName(config_files.at(config_index));
        if(!file.open(QIODevice::ReadOnly))
        {
            if( 0 == config_index )
            {
                diagnostic_read_config+="Can't read the file \"/etc/kded_rotation/kded_rotation.conf\"\n";
                answer=1;
                if(!file.exists())
                {
                    diagnostic_read_config+="Configuration file \"/etc/kded_rotation/kded_rotation.conf\" does not exist.\n";
                    diagnostic_read_config+="autorotation off\n";
                    answer=-1;
                    this->swch=false;
#ifdef _DIAGNOSTIC
                    this->diagnostic_read_config=diagnostic_read_config;
#endif
                    return diagnostic_read_config;
                }
            }
            if( 1 == config_index )
            {
                diagnostic_read_config+="Can't read the file \"" + QDir::homePath() + "/.config/kded_rotation.conf\"\n";
                answer=1;
                if(!file.exists())
                {
                    diagnostic_read_config+="Configuration file \"" + QDir::homePath() + "/.config/kded_rotation.conf\" does not exist.\n";
                    answer=-2;
                }
#ifdef _DIAGNOSTIC
                this->diagnostic_read_config=diagnostic_read_config;
#endif
                return diagnostic_read_config;
            }
        }
        QTextStream read(&file);
        int line=0;
        while(!read.atEnd())
        {
            line++;
            QString str=read.readLine();
            if(!str.contains(':'))
            {
                continue;
            }
            int coment=str.indexOf('#');
            if(coment>0)
            {
                int rm_lenght=str.length();
                coment--;
                rm_lenght=rm_lenght-coment;
                diagnostic_read_config+="Removing Coments from string:\n";
                diagnostic_read_config+= str + "\nFron ";
                diagnostic_read_config+= QString::number(coment) + "too ";
                diagnostic_read_config+= QString::number(rm_lenght) + "position\n";
                str.remove(coment,rm_lenght);

            }
            str=str.trimmed();
            QStringList conf=str.split(':');
            if(conf.size()<2)
            {
                diagnostic_read_config+="Error in file \n" + config_files.at(config_index) + "\"\n";
                diagnostic_read_config+="Error in line\n" + QString::number(line) + "missing name or value or bouth\n";
                continue;
            }
            if(conf[0]=="DISPLAY")
            {
                this->display=conf[1].trimmed();
                diagnostic_read_config+="File \n" + config_files.at(config_index) + "\" line" + QString::number(line) + "\n";
                diagnostic_read_config+="this->display =" + this->display + "\n";
                continue;
            }
            if(conf[0]=="TOUCH_SCREEN")
            {
                this->touch_screen=conf[1].trimmed();
                diagnostic_read_config+="File \n" + config_files.at(config_index) + "\" line" + QString::number(line) + "\n";
                diagnostic_read_config+="this->touch_screen = " + this->touch_screen + "\n";
                continue;
            }
            if(conf[0]=="TIME_BEFORE_START")
            {
                bool time_before_start_ok=false;
                this->time_before_start=conf[1].replace(',','.').toDouble(&time_before_start_ok) * 10.0;
                if(!time_before_start_ok)
                {
                    this->time_before_start=0;
                    diagnostic_read_config+="File \n" + config_files.at(config_index) + "\" line" + QString::number(line) + "\n";
                    diagnostic_read_config+="Error in TIME_BEFORE_START\n";
                }
                diagnostic_read_config+="File \n" + config_files.at(config_index) + "\" line" + QString::number(line) + "\n";
                diagnostic_read_config+="this->time_before_start = ";
                diagnostic_read_config += QString::number(this->time_before_start) + " milisecond * 100 sicls\n";
                continue;
            }
            if(conf[0]=="TIME_DELAY")
            {
                bool time_delay_ok=false;
                this->time_delay=conf[1].replace(',','.').toDouble(&time_delay_ok) * 1000.0;
                if(!time_delay_ok)
                {
                    this->time_delay=0;
                    diagnostic_read_config+="File \n" + config_files.at(config_index) + "\" line" + QString::number(line) + "\n";
                    diagnostic_read_config+="Error in TIME_DELAY\n";
                }
                diagnostic_read_config+="File \n" + config_files.at(config_index) + "\" line" + QString::number(line) + "\n";
                diagnostic_read_config+="this->time_delay = ";
                diagnostic_read_config += QString::number(this->time_delay) + " milisecond\n";
                continue;
            }
            if(conf[0]=="SCREEN_ROTATOR_ON")
            {
                bool check=false;
                bool tmp=conf[1].trimmed().toInt(&check);
                if(!check)
                {
                    diagnostic_read_config+="Error in configuration file \"" + config_files.at(config_index) + "\"\n";
                    diagnostic_read_config+="Error in line " + QString::number(line) + " missing or wrong value\n";
                    answer=2;
                    continue;
                }
                this->swch=tmp;
                diagnostic_read_config+=conf[0] + "=\"" + conf[1] + "\" = " + QString::number(tmp) + "\n";
                continue;
            }
            if(conf[0]=="MATRIX_NEED_TO_APPLY")
            {
                bool check=false;
                bool tmp=conf[1].trimmed().toInt(&check);
                if(!check)
                {
                    diagnostic_read_config+="Error in configuration file \"" + config_files.at(config_index) + "\"\n";
                    diagnostic_read_config+="Error in line " + QString::number(line) + " missing or wrong value\n";
                    answer=2;
                    continue;
                }
                this->matrix_needed=tmp;
                diagnostic_read_config+=conf[0] + "=\"" + conf[1] + "\" = " + QString::number(tmp) + "\n";
                continue;
            }
            if(conf[0]=="ROTATE_QUIETLY")
            {
                bool check=false;
                bool tmp=conf[1].trimmed().toInt(&check);
                if(!check)
                {
                    diagnostic_read_config+="Error in configuration file \"" + config_files.at(config_index) + "\"\n";
                    diagnostic_read_config+="Error in line " + QString::number(line) + " missing or wrong value\n";
                    answer=2;
                    continue;
                }
                this->rotate_quietly=tmp;
                diagnostic_read_config+=conf[0] + "=\"" + conf[1] + "\" = " + QString::number(tmp) + "\n";
                continue;
            }
            if(conf[0]=="Matrix_normal")
            {
                this->matrixs[0]=conf[1].trimmed().replace(',','.').split(' ', Qt::SkipEmptyParts);
                diagnostic_read_config+="File \n" + config_files.at(config_index) + "\" line" + QString::number(line) + "\n";
                diagnostic_read_config+="Matrix_normal = ";
                diagnostic_read_config += conf[1].trimmed().replace(',','.') + "\n";
                continue;
            }
            if(conf[0]=="Matrix_inverted")
            {
                this->matrixs[1]=conf[1].trimmed().replace(',','.').split(' ', Qt::SkipEmptyParts);
                diagnostic_read_config+="File \n" + config_files.at(config_index) + "\" line" + QString::number(line) + "\n";
                diagnostic_read_config+="Matrix_inverted = ";
                diagnostic_read_config += conf[1].trimmed().replace(',','.') + "\n";
                continue;
            }
            if(conf[0]=="Matrix_left")
            {
                this->matrixs[2]=conf[1].trimmed().replace(',','.').split(' ', Qt::SkipEmptyParts);
                diagnostic_read_config+="File \n" + config_files.at(config_index) + "\" line" + QString::number(line) + "\n";
                diagnostic_read_config+="Matrix_left = ";
                diagnostic_read_config += conf[1].trimmed().replace(',','.') + "\n";
                continue;
            }
            if(conf[0]=="Matrix_right")
            {
                this->matrixs[3]=conf[1].trimmed().replace(',','.').split(' ', Qt::SkipEmptyParts);
                diagnostic_read_config+="File \n" + config_files.at(config_index) + "\" line" + QString::number(line) + "\n";
                diagnostic_read_config+="Matrix_right = ";
                diagnostic_read_config += conf[1].trimmed().replace(',','.') + "\n";
                continue;
            }
        }
        file.close();
    }
#ifdef _DIAGNOSTIC
                this->diagnostic_read_config=diagnostic_read_config;
#endif
    return diagnostic_read_config;
}

//Функция валидации конфига
QString ScreenRotator::validate_config(int& answer)
{
    QString diagnostic_validate_config="\n\nFunction validate_config() is started\n";
    //bool fatal_error=false;
    answer=0;
    if(this->display.isEmpty())
    {
        diagnostic_validate_config+="Display name is not set, autorotation off\n";
        //fatal_error=true;
        answer=_NO_DISPLAY;
        this->swch=false;
    }
    else
    {
        this->xrandr_args<<"--output";
        this->xrandr_args<<this->display;
        this->xrandr_args<<"--rotate";
    }
    if(this->touch_screen.isEmpty())
    {
        diagnostic_validate_config+="Error Touch screen name not set, autorotation off\n";
        //fatal_error=true;
        answer= answer | _NO_TOUCH_SCREEN;
        this->matrix_needed=false;
    }
    else
        this->xinput_args<<"set-prop"<<this->touch_screen<<"Coordinate Transformation Matrix";
    if( 2 > this->time_before_start || 100 < this->time_before_start)
    {
        diagnostic_validate_config+="TIME_BEFORE_START must be in range between 0,2 and 10\n";
        diagnostic_validate_config+="Seting TIME_BEFORE_START to default 2,5\n";
        answer= answer | _NO_NOT_CREETICAL_PARAM;
        this->time_before_start=25;
    }
    if( 25 > this->time_delay || 5000 < this->time_delay)
    {
        diagnostic_validate_config+="TIME_DELAY is not in the range between 0.025 and 5\n";
        diagnostic_validate_config+="Seting TIME_DELAY to default 0.5, if \"Coordinate Transformation\"\n";
        diagnostic_validate_config+="is not applied right increase TIME_DELAY_MICROSEC\n";
        answer= answer | _NO_NOT_CREETICAL_PARAM;
        this->time_delay=500;
    }
    for(int index=0;index<4;index++)
    {
        QString matrix_name;
        QString default_matrix;
        switch(index)
        {
            case 0:
                matrix_name="Matrix_normal";
                default_matrix="1 0 0 0 1 0 0 0 1";
                break;
            case 1:
                matrix_name="Matrix_inverted";
                default_matrix="-1 0 1 0 -1 1 0 0 1";
                break;
            case 2:
                matrix_name="Matrix_left";
                default_matrix="0 -1 1 1 0 0 0 0 1";
                break;
            case 3:
                matrix_name="Matrix_right";
                default_matrix="0 1 0 -1 0 1 0 0 1";
                break;
        }
        if(this->matrixs[index].isEmpty())
        {
            diagnostic_validate_config+="Error Coordinate Transformation " + matrix_name;
            diagnostic_validate_config+=" is empty, set default matrix\n";
            this->matrixs[index]=default_matrix.split(' ', Qt::SkipEmptyParts);
            answer= answer | _NO_NOT_CREETICAL_PARAM;
            continue;
        }
        if( this->matrixs[index].length() != 9 )
        {
            diagnostic_validate_config+="Error "+matrix_name+" must be nine digits set default matrix\n";
            this->matrixs[index]=default_matrix.split(' ', Qt::SkipEmptyParts);
            answer= answer | _NO_NOT_CREETICAL_PARAM;
            continue;
        }
        for(int num=0;num<9;num++)
        {
            bool ok;
            this->matrixs[index][num].toDouble(&ok);
            if(!ok)
            {
                diagnostic_validate_config+="Error invalid value in "+matrix_name+" set default matrix\n";
                this->matrixs[index]=default_matrix.split(' ', Qt::SkipEmptyParts);
                answer= answer | _NO_NOT_CREETICAL_PARAM;
                break;
            }
        }

    }
    /*if(!fatal_error)
    {
        this->xrandr_args<<"--output";
        this->xrandr_args<<this->display;
        this->xrandr_args<<"--rotate";

    }*/
#ifdef _DIAGNOSTIC
    this->diagnostic_validate_config=diagnostic_validate_config;
#endif
    return diagnostic_validate_config;
}


///////////////////////////////////////////////////////////////////////////
/// Функция для создания конфиг файла пользователя.
///////////////////////////////////////////////////////////////////////////
bool ScreenRotator::create_config_file()
{
    QFile root_config("/etc/kded_rotation/kded_rotation.conf");
    return root_config.copy(QDir::homePath() + "/.config/kded_rotation.conf");
}

///////////////////////////////////////////////////////////////////////////
/// Функция для записи в конфиг файл
///////////////////////////////////////////////////////////////////////////
QString ScreenRotator::save_to_config(int& answer)
{
    QString save_too_config_state="Start the function \"save_to_config()\"\n\n";
    QFile config_file(QDir::homePath() + "/.config/kded_rotation.conf");
    //bool config_file_exist=config_file.exists();
    answer=0;
    if(!config_file.open(QIODevice::ReadWrite))
    {
        save_too_config_state+="error while opening file \"" +QDir::homePath() + "/.config/kded_rotation.conf\" too ReadWrite\n";
#ifdef _DIAGNOSTIC
        this->save_too_config_state=save_too_config_state;
#endif
        answer=1;
        return save_too_config_state;
    }
    QTextStream read(&config_file);
    QString config_content;
    qint64 start_pos=0;
    bool entery_flag[4] = { false, true, true, true };
    while(!read.atEnd())
    {
        QString coments(""), str_tmp, str;
        if(!entery_flag[0])
            start_pos=read.pos();
        str_tmp = str = read.readLine();
        /// Извлечение коментариев
        int coment=str_tmp.indexOf('#');
        if(coment>=0)
        {
            if(coment==0)
            {
                if(entery_flag[0])
                    config_content+=str+"\n";
                continue;
            }
            coment--;
            int rm_lenght=str_tmp.length() - coment;
            save_too_config_state+="Removing comments from string:\n";
            save_too_config_state+= str_tmp + "\nFron ";
            save_too_config_state+= QString::number(coment) + "too ";
            save_too_config_state+= QString::number(rm_lenght) + " position\n";
            coments="   "+str_tmp.mid(coment);
            str_tmp.remove(coment,rm_lenght);
            save_too_config_state+="Save the comments \"" + coments + "\"\n";
        }
        //// Конец извлечения коментариев
        /// Если строка не содержит полезной информации
        if(!str.contains(':'))
        {
            if(entery_flag[0])
                config_content+=str+"\n";
            continue;
        }
        /// разбить строку на пары имя значение
        str_tmp=str_tmp.split(':')[0].trimmed();
        ///////////////////////////////////////////////////////////
        /// Поиск по именам и сохранение значений
        if( str_tmp == "SCREEN_ROTATOR_ON" )
        {
            entery_flag[0]=true;
            entery_flag[1]=false;
            int tmp = this->swch ? 1 : 0;
            str="SCREEN_ROTATOR_ON:" + QString::number(tmp) + coments;
            save_too_config_state+="SCREEN_ROTATOR_ON has been found, a new string has been created\n";
            save_too_config_state += str + "\n";
            config_content += str + "\n";
            continue;
        }
        if( str_tmp == "ROTATE_QUIETLY" )
        {
            entery_flag[0]=true;
            entery_flag[2]=false;
            int tmp = this->rotate_quietly ? 1 : 0;
            str="ROTATE_QUIETLY:" + QString::number(tmp) + coments;
            save_too_config_state+="ROTATE_QUIETLY has been found, a new string has been created\n";
            save_too_config_state += str + "\n";
            config_content += str + "\n";
            continue;
        }
        if( str_tmp == "MATRIX_NEED_TO_APPLY" )
        {
            entery_flag[0]=true;
            entery_flag[3]=false;
            int tmp = this->matrix_needed ? 1 : 0;
            str="MATRIX_NEED_TO_APPLY:" + QString::number(tmp) + coments;
            save_too_config_state+="MATRIX_NEED_TO_APPLY has been found, a new string has been created\n";
            save_too_config_state += str + "\n";
            config_content += str + "\n";
            continue;
        }
        if(entery_flag[0])
            config_content += str + "\n";
    }
    if(entery_flag[1])
    {
        config_content+="\n# 0 or any integer. Automatic screen rotation at startup if not 0\n";
        config_content+="SCREEN_ROTATOR_ON:" + QString::number(this->swch ? 1 : 0) + " \n";
    }
    if(entery_flag[2])
    {
        config_content+="\n# 0 or any integer. If you want to be warned before the screen is rotated 1\n";
        config_content+="ROTATE_QUIETLY:" + QString::number(this->rotate_quietly ? 1 : 0) + " \n";
    }
    if(entery_flag[3])
    {
        config_content+="\n# 0 or any integer. if you need a custom \"Coordinate Transformation Matrix\" 1\n";
        config_content+="MATRIX_NEED_TO_APPLY:" + QString::number(this->matrix_needed ? 1 : 0) + " \n";
    }
    read.seek(start_pos);
    save_too_config_state+="Write position is:" + QString::number(start_pos) + "\n";
    read<<config_content;
    config_file.close();
#ifdef _DIAGNOSTIC
        this->save_too_config_state=save_too_config_state;
#endif
    return save_too_config_state;
}


//Функция слот dbus для перечитывания конфига
QString ScreenRotator::reconfig()
{
    //////////////////////////////////////////////////////
    /// Поправить и дополнить
    ///
    int answer=0;
    QString str=this->read_config(answer);
    str+=this->validate_config(answer);
    if( !( answer & _NO_DISPLAY ) )
        this->sensor_state();
    return str;
}


void ScreenRotator::set_matrix_apply_needed(const bool matrix_needed)
{
    this->matrix_needed=matrix_needed;
}
const bool& ScreenRotator::get_matrix_apply_needed()
{
    return this->matrix_needed;
}

void ScreenRotator::set_rotate_quietly(bool rotate_quietly)
{
    this->rotate_quietly=rotate_quietly;
    this->sensor_state();
}
const bool& ScreenRotator::get_rotate_quietly()
{
    return this->rotate_quietly;
}

void ScreenRotator::sensor_state()
{
#ifdef _DIAGNOSTIC
    this->diagnostic_sensor_satate+="Function sensor_state() \n\n";
    this->diagnostic_sensor_satate+= "this->swch=" + QString::number((int)this->swch) + "\n";
#endif
    if(this->swch)
    {
#ifdef _DIAGNOSTIC
        this->diagnostic_sensor_satate+= "this->rotate_quietly=" + QString::number((int)this->rotate_quietly) + "\n";
#endif
        if(this->rotate_quietly)
        {
            if( nullptr != this->timer_before_start )
            {
                this->timer_before_start->stop();
                QObject::disconnect(this->timer_before_start, &QTimer::timeout, this, &ScreenRotator::updateProgress);
                delete this->timer_before_start;
                this->timer_before_start = nullptr;
#ifdef _DIAGNOSTIC
                this->diagnostic_sensor_satate+="nullptr != this->timer_before_start\n";
                this->diagnostic_sensor_satate+="this->timer_before_start disconnected and deleted\n";
#endif
            }
            if( nullptr == this->sensor )
            {
                this->sensor = new QOrientationSensor(this);
                this->sensor->start();
                connect(this->sensor, &QOrientationSensor::readingChanged, this, &ScreenRotator::updateOrientation, Qt::UniqueConnection);
                //connect(this->timer_before_start, &QTimer::timeout, this, &ScreenRotator::updateProgress);
#ifdef _DIAGNOSTIC
                this->diagnostic_sensor_satate+="nullptr == this->sensor\n";
                this->diagnostic_sensor_satate+="this->sensor created and connected too ScreenRotator::updateOrientation\n";
#endif
            }
            else
            {
#ifdef _DIAGNOSTIC
                this->diagnostic_sensor_satate+="nullptr != this->sensor\n";
#endif
                QObject::disconnect(this->sensor, &QOrientationSensor::readingChanged, this, &ScreenRotator::startProgress);
                if(!this->sensor->isActive())
                {
                    this->sensor->start();
#ifdef _DIAGNOSTIC
                    this->diagnostic_sensor_satate+=" !this->sensor->isActive() starting\n";
#endif
                }
                connect(this->sensor, &QOrientationSensor::readingChanged, this, &ScreenRotator::updateOrientation, Qt::UniqueConnection);
#ifdef _DIAGNOSTIC
                this->diagnostic_sensor_satate+="this->sensor disconnected from ScreenRotator::startProgress\n";
                this->diagnostic_sensor_satate+="this->sensor and connected to ScreenRotator::updateOrientation\n";
#endif
            }
        }
        else
        {
            if(nullptr == this->timer_before_start)
            {
                this->timer_before_start = new QTimer(this);
                connect(this->timer_before_start, &QTimer::timeout, this, &ScreenRotator::updateProgress, Qt::UniqueConnection);
#ifdef _DIAGNOSTIC
                this->diagnostic_sensor_satate+="nullptr == this->timer_before_start\n";
                this->diagnostic_sensor_satate+="this->timer_before_start created and connected too ScreenRotator::updateProgress\n";
#endif
            }
            if( nullptr == this->sensor )
            {
                this->sensor = new QOrientationSensor(this);
                this->sensor->start();
                QObject::connect(this->sensor, &QOrientationSensor::readingChanged, this, &ScreenRotator::startProgress, Qt::UniqueConnection);
#ifdef _DIAGNOSTIC
                this->diagnostic_sensor_satate+="nullptr == this->sensor\n";
                this->diagnostic_sensor_satate+="this->sensor created and connected too ScreenRotator::startProgress\n";
#endif
            }
            else
            {
#ifdef _DIAGNOSTIC
                this->diagnostic_sensor_satate+="nullptr != this->sensor\n";
#endif
                QObject::disconnect(this->sensor, &QOrientationSensor::readingChanged, this, &ScreenRotator::updateOrientation);
                if( !this->sensor->isActive() )
                {
                    this->sensor->start();
#ifdef _DIAGNOSTIC
                this->diagnostic_sensor_satate+="!this->sensor->isActive() starting\n";
#endif
                }
                QObject::connect(this->sensor, &QOrientationSensor::readingChanged, this, &ScreenRotator::startProgress, Qt::UniqueConnection);
#ifdef _DIAGNOSTIC
                this->diagnostic_sensor_satate+="this->sensor disconnected from ScreenRotator::updateOrientation and connected too ScreenRotator::startProgress\n";
#endif
            }
        }
    }
    else
    {
        if( nullptr != this->timer_before_start )
        {
            this->timer_before_start->stop();
            QObject::disconnect(this->timer_before_start, &QTimer::timeout, this, &ScreenRotator::updateProgress);
            delete this->timer_before_start;
            this->timer_before_start = nullptr;
#ifdef _DIAGNOSTIC
            this->diagnostic_sensor_satate+="nullptr != this->timer_before_start deleted and disconnected\n";
#endif
        }
        if( nullptr != this->sensor )
        {
            if(this->rotate_quietly)
                QObject::disconnect(this->sensor, &QOrientationSensor::readingChanged, this, &ScreenRotator::updateOrientation);
            else
                QObject::disconnect(this->sensor, &QOrientationSensor::readingChanged, this, &ScreenRotator::startProgress);
            this->sensor->stop();
            delete this->sensor;
            this->sensor = nullptr;
#ifdef _DIAGNOSTIC
            this->diagnostic_sensor_satate+="nullptr != this->sensor deleted and disconnected\n";
#endif
        }
    }
}
