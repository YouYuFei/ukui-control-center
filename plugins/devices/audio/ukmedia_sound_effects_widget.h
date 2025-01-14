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
#ifndef UKMEDIASOUNDEFFECTSWIDGET_H
#define UKMEDIASOUNDEFFECTSWIDGET_H

#include <QWidget>
#include <QComboBox>
#include <QLabel>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QStyledItemDelegate>
#include "ukui_custom_style.h"
#include "SwitchButton/switchbutton.h"

class UkuiMessageBox : public QMessageBox
{
public:
    explicit UkuiMessageBox();
};

class UkmediaSoundEffectsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit UkmediaSoundEffectsWidget(QWidget *parent = nullptr);
    ~UkmediaSoundEffectsWidget();
    friend class UkmediaMainWidget;
Q_SIGNALS:

public Q_SLOTS:

private:
    QFrame *m_pThemeWidget;
    QFrame *m_pAlertSoundWidget;
    QFrame *m_pLagoutWidget;
    QFrame *m_pAlertSoundSwitchWidget;
    QFrame *m_pAlertSoundVolumeWidget;
    QFrame *m_pWakeupMusicWidget;
    QFrame *m_pVolumeChangeWidget;
    QFrame *m_pStartupMusicWidget;
    QFrame *m_pPoweroffMusicWidget;

    QString qss;
    QStyledItemDelegate *itemDelegate;
    QLabel *m_pSoundEffectLabel;
    QLabel *m_pSoundThemeLabel;
    QLabel *m_pShutdownlabel;
    QLabel *m_pLagoutLabel;
    QLabel *m_pAlertSoundSwitchLabel;
    QLabel *m_pAlertSoundLabel;
    QLabel *m_pAlertVolumeLabel;
    QLabel *m_pWakeupMusicLabel;
    QLabel *m_pVolumeChangeLabel;
    QLabel *m_pPoweroffMusicLabel;
    QLabel *m_pStartupMusicLabel;

    QComboBox *m_pSoundThemeCombobox;
    QComboBox *m_pAlertSoundCombobox;
    QComboBox *m_pLagoutCombobox;
    QComboBox *m_pVolumeChangeCombobox;
    QVBoxLayout *m_pSoundLayout;
    SwitchButton *m_pStartupButton;
    SwitchButton *m_pPoweroffButton;
    SwitchButton *m_pLogoutButton;
    SwitchButton *m_pAlertSoundSwitchButton;
    SwitchButton *m_pWakeupMusicButton;
    UkmediaVolumeSlider *m_pAlertSlider;
    UkuiButtonDrawSvg *m_pAlertIconBtn;
};

#endif // UKMEDIASOUNDEFFECTSWIDGET_H
