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
#include "sharemain.h"

#include <QProcess>
#include <QHBoxLayout>
#include <QAbstractButton>

#include <QDBusInterface>
#include <QDBusConnection>

ShareMain::ShareMain(QWidget *parent) :
    QWidget(parent)
{
    mVlayout = new QVBoxLayout(this);
    mVlayout->setContentsMargins(0, 0, 32, 0);
    initUI();
    initConnection();
    initTitleLabel();
}

ShareMain::~ShareMain()
{
}

void ShareMain::initTitleLabel()
{
    QFont font;
    font.setPixelSize(18);
    mShareTitleLabel->setFont(font);
}

void ShareMain::initUI()
{
    mShareTitleLabel = new QLabel(tr("Share"), this);

    mEnableFrame = new QFrame(this);
    mEnableFrame->setFrameShape(QFrame::Shape::Box);
    mEnableFrame->setMinimumSize(550, 50);
    mEnableFrame->setMaximumSize(960, 50);

    QHBoxLayout *enableHLayout = new QHBoxLayout();

    mEnableBtn = new SwitchButton(this);
    mEnableLabel = new QLabel(tr("Allow others to view your desktop"), this);
    enableHLayout->addWidget(mEnableLabel);
    enableHLayout->addStretch();
    enableHLayout->addWidget(mEnableBtn);

    mEnableFrame->setLayout(enableHLayout);

    mViewFrame = new QFrame(this);
    mViewFrame->setFrameShape(QFrame::Shape::Box);
    mViewFrame->setMinimumSize(550, 50);
    mViewFrame->setMaximumSize(960, 50);

    QHBoxLayout *viewHLayout = new QHBoxLayout();

    mViewBtn = new SwitchButton(this);
    mViewLabel = new QLabel(tr("Allow connection to control screen"), this);
    viewHLayout->addWidget(mViewLabel);
    viewHLayout->addStretch();
    viewHLayout->addWidget(mViewBtn);

    mViewFrame->setLayout(viewHLayout);

    mSecurityTitleLabel = new QLabel(tr("Security"), this);

    mSecurityFrame = new QFrame(this);
    mSecurityFrame->setFrameShape(QFrame::Shape::Box);
    mSecurityFrame->setMinimumSize(550, 50);
    mSecurityFrame->setMaximumSize(960, 50);

    QHBoxLayout *secHLayout = new QHBoxLayout();

    mAccessBtn = new SwitchButton(this);
    mAccessLabel = new QLabel(tr("You must confirm every visit for this machine"), this);
    secHLayout->addWidget(mAccessLabel);
    secHLayout->addStretch();
    secHLayout->addWidget(mAccessBtn);

    mSecurityFrame->setLayout(secHLayout);

    mSecurityPwdFrame = new QFrame(this);
    mSecurityPwdFrame->setFrameShape(QFrame::Shape::Box);
    mSecurityPwdFrame->setMinimumSize(550, 50);
    mSecurityPwdFrame->setMaximumSize(960, 50);

    QHBoxLayout *pwdHLayout = new QHBoxLayout();

    mPwdBtn = new SwitchButton(this);
    mPwdsLabel = new QLabel(tr("Require user to enter this password: "), this);

    // mHintLabel = new QLabel(tr("Password can not be blank"), this);
    mHintLabel = new QLabel(this);
    mHintLabel->setStyleSheet("color:red;");

    mPwdLineEdit = new QLineEdit(this);
    pwdHLayout->addWidget(mPwdsLabel);
    pwdHLayout->addStretch();
    pwdHLayout->addWidget(mPwdLineEdit);
    pwdHLayout->addStretch();
    pwdHLayout->addWidget(mHintLabel);
    pwdHLayout->addWidget(mPwdBtn);

    mSecurityPwdFrame->setLayout(pwdHLayout);

    mVlayout->addWidget(mShareTitleLabel);
    mVlayout->addWidget(mEnableFrame);
    mVlayout->addWidget(mViewFrame);

    mVlayout->addWidget(mSecurityTitleLabel);
    mVlayout->addWidget(mSecurityFrame);
    mVlayout->addWidget(mSecurityPwdFrame);

    mVlayout->addStretch();
}

void ShareMain::initConnection()
{
    QByteArray id(kVinoSchemas);
    if (QGSettings::isSchemaInstalled(id)) {
        mVinoGsetting = new QGSettings(kVinoSchemas, QByteArray(), this);

        initEnableStatus();

        connect(mEnableBtn, &SwitchButton::checkedChanged, this, &ShareMain::enableSlot);
        connect(mViewBtn, &SwitchButton::checkedChanged, this, &ShareMain::viewBoxSlot);
        connect(mAccessBtn, &SwitchButton::checkedChanged, this, &ShareMain::accessSlot);
        connect(mPwdBtn, &SwitchButton::checkedChanged, this, &ShareMain::pwdEnableSlot);
        connect(mPwdLineEdit, &QLineEdit::textChanged, this, &ShareMain::pwdInputSlot);
    }
}

void ShareMain::initEnableStatus()
{
    bool isShared = mVinoGsetting->get(kVinoViewOnlyKey).toBool();
    bool secPwd = mVinoGsetting->get(kVinoPromptKey).toBool();
    QString pwd = mVinoGsetting->get(kAuthenticationKey).toString();
    QString secpwd = mVinoGsetting->get(kVncPwdKey).toString();

    mAccessBtn->setChecked(secPwd);
    mViewBtn->setChecked(!isShared);
    if (pwd == "vnc") {
        mPwdBtn->setChecked(true);
        mHintLabel->setVisible(true);
        mPwdLineEdit->setText(QByteArray::fromBase64(secpwd.toLatin1()));

        if (secpwd == NULL) {
            mHintLabel->setText(tr("Password can not be blank"));
        } else if (QByteArray::fromBase64(secpwd.toLatin1()).length() == 8) {
            mHintLabel->setText(tr("Password length must be less than or equal to 8"));
        }
    } else {
        mPwdBtn->setChecked(false);
        mPwdLineEdit->setVisible(false);
        mHintLabel->setVisible(false);
    }

    QProcess *process = new QProcess;

    process->start("systemctl", QStringList() << "--user" << "is-active" << "vino-server.service");
    process->waitForFinished();

    setFrameVisible((process->readAllStandardOutput().replace("\n", "") == "active"));
    delete process;
}

void ShareMain::setFrameVisible(bool visible)
{
    mEnableBtn->setChecked(visible);

    mViewFrame->setVisible(visible);
    mSecurityFrame->setVisible(visible);
    mSecurityPwdFrame->setVisible(visible);
    mSecurityTitleLabel->setVisible(visible);
}

void ShareMain::setVinoService(bool status)
{
    QDBusInterface vinoIfc("org.ukui.SettingsDaemon.Sharing",
                           "/org/ukui/SettingsDaemon/Sharing",
                           "org.ukui.SettingsDaemon.Sharing",
                           QDBusConnection::sessionBus());
    if (vinoIfc.isValid()) {
        if (status) {
            vinoIfc.call("EnableService", "vino-server");
        } else {
            vinoIfc.call("DisableService", "vino-server");
        }
    }
}

void ShareMain::enableSlot(bool status)
{
    setFrameVisible(status);
    setVinoService(status);
}

void ShareMain::viewBoxSlot(bool status)
{
    mVinoGsetting->set(kVinoViewOnlyKey, !status);
}

void ShareMain::accessSlot(bool status)
{
    if (status) {
        mVinoGsetting->set(kVinoPromptKey, true);
    } else {
        mVinoGsetting->set(kVinoPromptKey, false);
    }
}

void ShareMain::pwdEnableSlot(bool status)
{
    if (status) {
        mVinoGsetting->set(kAuthenticationKey, "vnc");
        mPwdLineEdit->setVisible(true);
        mHintLabel->setVisible(true);
    } else {
        mPwdLineEdit->setVisible(false);
        mHintLabel->setVisible(false);
        mVinoGsetting->set(kAuthenticationKey, "none");
    }
}

void ShareMain::pwdInputSlot(const QString &pwd)
{
    if (pwd.length() <= 7 && !pwd.isEmpty()) {
        mHintLabel->setText(tr(""));
        mHintLabel->setVisible(false);
        QByteArray text = pwd.toLocal8Bit();
        QByteArray secPwd = text.toBase64();
        mVinoGsetting->set(kVncPwdKey, secPwd);
    } else if (pwd.isEmpty() && mPwdLineEdit->text().isEmpty()) {
        mHintLabel->setText(tr("Password can not be blank"));
        mHintLabel->setVisible(true);
        QByteArray text = pwd.toLocal8Bit();
        QByteArray secPwd = text.toBase64();
        mVinoGsetting->set(kVncPwdKey, secPwd);
    } else {
        mHintLabel->setText(tr("Password length must be less than or equal to 8"));
        mHintLabel->setVisible(true);
        mPwdLineEdit->setText(pwd.mid(0, 8));
        QByteArray text = pwd.mid(0, 8).toLocal8Bit();
        QByteArray secPwd = text.toBase64();
        mVinoGsetting->set(kVncPwdKey, secPwd);
    }
}
