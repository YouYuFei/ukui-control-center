#include "dataformat.h"
#include "ui_dataformat.h"
#include "CloseButton/closebutton.h"

#include <QDateTime>
#include <QDebug>

#define PANEL_GSCHEMAL   "org.ukui.control-center.panel.plugins"
#define CALENDAR_KEY     "calendar"
#define DAY_KEY          "firstday"
#define DATE_FORMATE_KEY "date"
#define TIME_KEY         "hoursystem"

extern void qt_blurImage(QImage &blurImage, qreal radius, bool quality, int transposed);

DataFormat::DataFormat(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DataFormat)
{
    ui->setupUi(this);
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);

    ui->titleLabel->setStyleSheet("QLabel{font-size: 18px; color: palette(windowText);}");

    QByteArray id(PANEL_GSCHEMAL);
    if(QGSettings::isSchemaInstalled(id)) {
        m_gsettings = new QGSettings(id)        ;
    }

    locale = QLocale::system().name();

    initUi();
    initConnect();
    initComponent();
}

DataFormat::~DataFormat()
{
    delete ui;
    ui = nullptr;
    delete m_itimer;
    m_itimer = nullptr;
}


void DataFormat::initUi() {

    ui->calendarLabel->setText(tr("calendar"));
    ui->dayLabel->setText(tr("first day of week"));
    ui->dateLabel->setText(tr("date"));
    ui->timeLabel->setText(tr("time"));

    ui->calendarBox->addItem(tr("solar calendar"));

    if ("zh_CN" == locale){
        ui->calendarBox->addItem(tr("lunar"));
    }

    ui->dayBox->addItem(tr("monday"));
    ui->dayBox->addItem(tr("sunday"));        

    QString currentsecStr;
    QDateTime current = QDateTime::currentDateTime();

    currentsecStr = current.toString("yyyy/MM/dd ");
    ui->dateBox->addItem(currentsecStr);

    currentsecStr = current.toString("yyyy-MM-dd ");
    ui->dateBox->addItem(currentsecStr);

    QString timeStr;
    timeStr = current.toString("hh: mm : ss");
    ui->timeBox->addItem(timeStr);

    timeStr = current.toString("AP hh: mm : ss");
    ui->timeBox->addItem(timeStr);
}

void DataFormat::initConnect() {
    m_itimer = new QTimer();
    m_itimer->start(1000);

    connect(m_itimer,SIGNAL(timeout()), this, SLOT(datetime_update_slot()));
    connect(ui->confirmButton, SIGNAL(clicked(bool)), SLOT(confirm_btn_slot()));
    connect(ui->cancelButton, SIGNAL(clicked()), SLOT(close()));
}

void DataFormat::initComponent() {

    if (!m_gsettings) {
        return ;
    }
    const QStringList list = m_gsettings->keys();

    if (!list.contains("calendar") || !list.contains("firstday")
            || !list.contains("date") || !list.contains("hoursystem")){
        return ;
    }

    QString clac = m_gsettings->get(CALENDAR_KEY).toString();
    if ("solarlunar" == clac) {
        ui->calendarBox->setCurrentIndex(0);
    } else {
        ui->calendarBox->setCurrentIndex(1);
    }

    QString day = m_gsettings->get(DAY_KEY).toString();
    if ("monday" == day) {
        ui->dayBox->setCurrentIndex(0);
    } else {
        ui->dayBox->setCurrentIndex(1);
    }

    QString dateFormat = m_gsettings->get(DATE_FORMATE_KEY).toString();
    if ("cn" == dateFormat) {
       ui->dateBox->setCurrentIndex(0);
    } else {
       ui->dateBox->setCurrentIndex(1);
    }

    QString timeformate = m_gsettings->get(TIME_KEY).toString();
    if ("24" == timeformate) {
       ui->timeBox->setCurrentIndex(0);
    } else {
       ui->timeBox->setCurrentIndex(1);
    }
}

void DataFormat::writeGsettings(const QString &key, const QString &value) {
    if(!m_gsettings) {
        return ;
    }

    const QStringList list = m_gsettings->keys();
    if (!list.contains(key)) {
        return ;
    }    
    m_gsettings->set(key,value);    
}

void DataFormat::datetime_update_slot() {
    QString timeStr;
    QDateTime current = QDateTime::currentDateTime();

    timeStr = current.toString("hh: mm : ss");
    ui->timeBox->setItemText(0,timeStr);

    timeStr = current.toString("AP hh: mm : ss");
    ui->timeBox->setItemText(1,timeStr);
}


void DataFormat::confirm_btn_slot() {
    QString calendarValue;
    QString dayValue;
    QString dateValue;
    QString timeValue;

    if ( 0 == ui->calendarBox->currentIndex()) {
        calendarValue = "solarlunar";
    } else {
        calendarValue = "lunar";
    }

    if ( 0 == ui->dayBox->currentIndex()) {
        dayValue = "monday";
    } else {
        dayValue = "sunday";
    }

    if ( 0 == ui->dateBox->currentIndex()) {
        dateValue = "cn";
    } else {
        dateValue = "en";
    }

    if ( 0 == ui->timeBox->currentIndex()) {
        timeValue = "24";
    } else {
        timeValue = "12";
    }

    writeGsettings("calendar", calendarValue);
    writeGsettings("firstday", dayValue);
    writeGsettings("date", dateValue);
    writeGsettings("hoursystem", timeValue);

    emit dataChangedSignal();
    this->close();
}

void DataFormat::paintEvent(QPaintEvent *event) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QPainterPath rectPath;
    rectPath.addRoundedRect(this->rect().adjusted(10, 10, -10, -10), 6, 6);

    // 画一个黑底
    QPixmap pixmap(this->rect().size());
    pixmap.fill(Qt::transparent);
    QPainter pixmapPainter(&pixmap);
    pixmapPainter.setRenderHint(QPainter::Antialiasing);
    pixmapPainter.setPen(Qt::transparent);
    pixmapPainter.setBrush(Qt::black);
    pixmapPainter.setOpacity(0.65);
    pixmapPainter.drawPath(rectPath);
    pixmapPainter.end();

    // 模糊这个黑底
    QImage img = pixmap.toImage();
    qt_blurImage(img, 10, false, false);


    // 挖掉中心
    pixmap = QPixmap::fromImage(img);
    QPainter pixmapPainter2(&pixmap);
    pixmapPainter2.setRenderHint(QPainter::Antialiasing);
    pixmapPainter2.setCompositionMode(QPainter::CompositionMode_Clear);
    pixmapPainter2.setPen(Qt::transparent);
    pixmapPainter2.setBrush(Qt::transparent);
    pixmapPainter2.drawPath(rectPath);

    // 绘制阴影
    p.drawPixmap(this->rect(), pixmap, pixmap.rect());
    // 绘制一个背景
    p.save();
    p.fillPath(rectPath,palette().color(QPalette::Base));
    p.restore();

}
