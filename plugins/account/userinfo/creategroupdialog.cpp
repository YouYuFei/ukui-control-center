/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * Copyright (C) 2020 KYLINOS Information Technology Co., Ltd.
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

#include "creategroupdialog.h"
#include "ui_creategroupdialog.h"
#include "userinfo.h"
#include "changegroupdialog.h"
#include "CloseButton/closebutton.h"

extern void qt_blurImage(QImage &blurImage, qreal radius, bool quality, int transposed);

CreateGroupDialog::CreateGroupDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CreateGroupDialog),
    cgDialog(new ChangeGroupDialog),
    _nameHasModified(false),
    _idHasModified(false),
    _boxModified(false)
{
    ui->setupUi(this);
    setupInit();
    getUsersList();
    signalsBind();
}

CreateGroupDialog::~CreateGroupDialog()
{
    delete cgDialog;
    cgDialog = nullptr;
    delete ui;
    ui = nullptr;
}

void CreateGroupDialog::limitInput()
{
    QIntValidator *intValidator = new QIntValidator;
    // QRegExp rx("^[a-zA-z]+$");// 首字符为字母
    QRegExp rx("^[a-zA-Z][a-zA-Z0-9_-]*${32}");
    QRegExpValidator *regValidator = new QRegExpValidator(rx);
    // intValidator->setRange(0, 65535);
    intValidator->setBottom(0);
    // 整形输入限制
    ui->lineEdit_id->setValidator(intValidator);
    // 字母输入限制
    ui->lineEdit_name->setValidator(regValidator);
    // 字符长度限制
    // ui->lineEdit_name->setMaxLength(4);
}

void CreateGroupDialog::refreshCertainBtnStatus()
{
    if (ui->lineEdit_name->text().isEmpty()
        || ui->lineEdit_id->text().isEmpty())
        ui->certainBtn->setEnabled(false);
    else
        ui->certainBtn->setEnabled(_nameHasModified || _idHasModified);
}

UserInfomationss CreateGroupDialog::_acquireUserInfo(QString objpath)
{
    UserInfomationss user;

    // 默认值
    user.current = false;
    user.logined = false;
    user.autologin = false;

    QDBusInterface *iproperty = new QDBusInterface("org.freedesktop.Accounts",
                                                   objpath,
                                                   "org.freedesktop.DBus.Properties",
                                                   QDBusConnection::systemBus());
    QDBusReply<QMap<QString, QVariant> > reply = iproperty->call("GetAll",
                                                                 "org.freedesktop.Accounts.User");
    if (reply.isValid()) {
        QMap<QString, QVariant> propertyMap;
        propertyMap = reply.value();
        user.username = propertyMap.find("UserName").value().toString();
        if (user.username == QString(g_get_user_name())) {
            user.current = true;
            user.logined = true;
        }
    } else
        qDebug() << "reply failed";

    delete iproperty;
    iproperty = nullptr;

    return user;
}

void CreateGroupDialog::getUsersList()
{
    qDebug() << "当前文件 :" << __FILE__ << "当前函数 :" << __FUNCTION__ << "当前行号 :" << __LINE__;
    QStringList allUsers;
    sysdispatcher = new SystemDbusDispatcher(this);

    QStringList objectpaths = sysdispatcher->list_cached_users();
    allUserInfoMap.clear();
    // root
    if (!getuid()) {
        UserInfomationss root;
        root.username = g_get_user_name();
        root.current = true;
        root.logined = true;
        root.autologin = false;
        root.uid = 0;
        root.accounttype = ADMINISTRATOR;
        // root.iconfile = DEFAULTFACE;
        allUserInfoMap.insert(root.username, root);
    }
    for (QString objectpath : objectpaths) {
        UserInfomationss user;
        user = _acquireUserInfo(objectpath);
        allUserInfoMap.insert(user.username, user);
    }
    for (QVariant tmp : allUserInfoMap.keys()) {
        allUsers << tmp.toString();
    }
    QStringList usersList = allUsers;

    for (int i = 0; i < usersList.size(); i++) {
        QListWidgetItem *item = new QListWidgetItem(ui->listWidget);
        item->setSizeHint(QSize(ui->listWidget->width(), 36));
        item->setData(Qt::UserRole, "");
        QCheckBox *box = new QCheckBox(usersList.at(i));
        ui->listWidget->addItem(item);
        ui->listWidget->setItemWidget(item, box);
        connect(box, &QCheckBox::clicked, this, [=](bool checked){
            qDebug() << "checkbox clicked" << checked;
            _boxModified = true;
            refreshCertainBtnStatus();
        });
    }
}

QPushButton *CreateGroupDialog::certainBtnComponent()
{
    return ui->certainBtn;
}

QLineEdit *CreateGroupDialog::lineNameComponent()
{
    return ui->lineEdit_name;
}

QLineEdit *CreateGroupDialog::lineIdComponent()
{
    return ui->lineEdit_id;
}

QListWidget *CreateGroupDialog::listWidgetComponent()
{
    return ui->listWidget;
}

void CreateGroupDialog::signalsBind()
{
    connect(ui->cancelBtn, &QPushButton::clicked, [=](){
        close();
    });
    connect(ui->lineEdit_name, &QLineEdit::textChanged, [=](QString txt){
        Q_UNUSED(txt);
        refreshCertainBtnStatus();
    });
    connect(ui->lineEdit_id, &QLineEdit::textChanged, [=](QString txt){
        Q_UNUSED(txt);
        refreshCertainBtnStatus();
    });
    connect(ui->lineEdit_id, &QLineEdit::textEdited, [=](){
        for (int j = 0; j < cgDialog->groupList->size(); j++) {
            if (ui->lineEdit_id->text() == cgDialog->groupList->at(j)->groupid) {
                _idHasModified = false;
            }
        }
        _idHasModified = true;
    });
    connect(ui->lineEdit_name, &QLineEdit::textEdited, [=](){
        for (int j = 0; j < cgDialog->groupList->size(); j++) {
            if (ui->lineEdit_id->text() == cgDialog->groupList->at(j)->groupname) {
                _nameHasModified = false;
            }
        }
        _nameHasModified = true;
    });
}

void CreateGroupDialog::setupInit()
{
    setWindowTitle(tr("Add user group"));
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);

    ui->listWidget->setFocusPolicy(Qt::NoFocus);
    ui->listWidget->setSelectionMode(QAbstractItemView::NoSelection);
    ui->listWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->listWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    ui->listWidget->setStyleSheet("QListWidget{border-radius: 4px;}"
                                  "QListWidget::Item{padding-left:20px;}");

    // 设置确认按钮
    refreshCertainBtnStatus();
    // 限制组名输入规则
    limitInput();
}

void CreateGroupDialog::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
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
    p.fillPath(rectPath, palette().color(QPalette::Base));
    p.restore();
}
