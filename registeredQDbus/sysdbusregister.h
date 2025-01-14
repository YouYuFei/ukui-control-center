/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * Copyright (C) 2019 Tianjin KYLIN Information Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */
#ifndef SYSDBUSREGISTER_H
#define SYSDBUSREGISTER_H

#include <QObject>
#include <QCoreApplication>
#include <QProcess>
#include <QFile>
#include <QSettings>

class SysdbusRegister : public QObject
{
    Q_OBJECT

    Q_CLASSINFO("D-Bus Interface", "com.control.center.interface")

public:
    explicit SysdbusRegister();
    ~SysdbusRegister();

private:
    QString mHibernateFile;

    QSettings *mHibernateSet;

signals:
    Q_SCRIPTABLE void nameChanged(QString);
    Q_SCRIPTABLE void computerinfo(QString);

public slots:

    Q_SCRIPTABLE void exitService();
    Q_SCRIPTABLE QString GetComputerInfo();

    // 设置免密登录状态
    Q_SCRIPTABLE void setNoPwdLoginStatus(bool status,QString username);

    // 获取免密登录状态
    Q_SCRIPTABLE QString getNoPwdLoginStatus();

    // 设置自动登录状态
    Q_SCRIPTABLE void setAutoLoginStatus(QString username);

    // 获取挂起到休眠时间
    Q_SCRIPTABLE QString getSuspendThenHibernate();

    // 设置挂起到休眠时间
    Q_SCRIPTABLE void setSuspendThenHibernate(QString time);

    // 设置密码时效
    Q_SCRIPTABLE void setPasswdAging(int days, QString username);

    // 提权修改其他用户密码
    Q_SCRIPTABLE int changeOtherUserPasswd(QString username, QString pwd);

    // 调节外接台式屏幕亮度
    Q_SCRIPTABLE void setDDCBrightness(QString brightness, QString type);

    // 获取外接台式屏幕亮度
    Q_SCRIPTABLE int getDDCBrightness(QString type);

    // 修改硬件时间
    Q_SCRIPTABLE int changeRTC();

};

#endif // SYSDBUSREGISTER_H
