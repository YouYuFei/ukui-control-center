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
#ifndef SCREENLOCK_H
#define SCREENLOCK_H

#include <QObject>
#include <QtPlugin>

#include <QLabel>
#include <QThread>
#include <QGSettings>
#include <QSettings>
#include <QtDBus>

#include "shell/interface.h"
#include "SwitchButton/switchbutton.h"
#include "FlowLayout/flowlayout.h"
#include "Uslider/uslider.h"

#include "buildpicunitsworker.h"
#include "pictureunit.h"

#ifdef signals
#undef signals
#endif

extern "C" {
#include <glib.h>
#include <gio/gio.h>
}

namespace Ui {
class Screenlock;
}

class Screenlock : public QObject, CommonInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kycc.CommonInterface")
    Q_INTERFACES(CommonInterface)

public:
    Screenlock();
    ~Screenlock();

    QString get_plugin_name()   Q_DECL_OVERRIDE;
    int get_plugin_type()       Q_DECL_OVERRIDE;
    QWidget * get_plugin_ui()   Q_DECL_OVERRIDE;
    void plugin_delay_control() Q_DECL_OVERRIDE;
    const QString name() const  Q_DECL_OVERRIDE;

    void initSearchText();
    void setupComponent();
    void setupConnect();
    void initScreenlockStatus();

private:
    int convertToLocktime(const int value);
    int lockConvertToSlider(const int value);
    void setLockBackground(bool status);
    bool getLockStatus();
    void connectToServer();
    QString copyLoginFile(QString fileName);

private:
    Ui::Screenlock *ui;

    QString pluginName;
    int pluginType;
    QWidget *pluginWidget;

    SwitchButton *loginbgSwitchBtn; // 显示锁屏壁纸在登录页面
    SwitchButton *lockSwitchBtn;    // 激活屏保时锁定屏幕

    Uslider *uslider;

    QGSettings *lSetting;
    QSettings *lockSetting;         // 锁屏状态QSettings
    QSettings *lockLoginSettings;

    QSize lockbgSize;

    QThread *pThread;

    QDBusInterface *m_cloudInterface;
    bool bIsCloudService;

    FlowLayout *flowLayout;

    BuildPicUnitsWorker *pWorker;
    PictureUnit *prePicUnit;

    bool mFirstLoad;

    QString mUKCConfig;

public Q_SLOTS:
    void keyChangedSlot(const QString &key);

private slots:
    void setScreenLockBgSlot();
};

#endif // SCREENLOCK_H
