#ifndef UKSCCONN_H
#define UKSCCONN_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QMessageBox>
#include <QApplication>
#include <QSqlError>
#include <QFile>
#include <QDebug>
#include <QFileInfo>

class UKSCConn : public QObject
{
public:
    explicit UKSCConn();
    QSqlDatabase ukscDb;
    QSqlQuery query;
    bool isConnectUskc = true;

    // 根据应用名获取图标、中文名、描述
    QStringList getInfoByName(QString appName);

};

#endif // UKSCCONN_H
