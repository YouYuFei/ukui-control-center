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
#ifndef DELUSERDIALOG_H
#define DELUSERDIALOG_H

#include <QDialog>
#include <QSignalMapper>
#include <QAbstractButton>
#include <QPainter>
#include <QPainterPath>

namespace Ui {
class DelUserDialog;
}

class DelUserDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DelUserDialog(QWidget *parent = 0);
    ~DelUserDialog();

public:
    void setupComonpent();
    void setupConnect();

    void setFace(QString file);
    void setUsername(QString name);

protected:
    void paintEvent(QPaintEvent *);

private:
    Ui::DelUserDialog *ui;


Q_SIGNALS:
    void removefile_send(bool removefile);
};

#endif // DELUSERDIALOG_H
