/*
    Author: Jesper Thomschutz 2008, jesper@jespersaur.com

    This file is part of Trippy.

    Trippy is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Trippy is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Trippy.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef LOADSCREEN_H
#define LOADSCREEN_H

#include <QDialog>
#include <QFutureWatcher>
#include "ui_loadscreen.h"

class LoadScreen : public QDialog
{
  Q_OBJECT
  public:
    LoadScreen(QWidget *parent, QFutureWatcher<QString> *watcher);

  private:
    Ui::loadScreen ui;
    QFutureWatcher<QString> *m_watcher;
    bool m_canceled;

  public slots:
    void showFailedPhotos(bool show);
    void clearFailedPhotos();
    void setProgressRange(int minimum, int maximum);
    void setProgressValue(int value);
    void setProgressText(QString text);
    void addFailedFile(QString filename);
    void finished();
    void canceled();
    void showCancel(const bool show);


private slots:
    void on_pb_cancel_clicked();
    
private:
  Q_DISABLE_COPY(LoadScreen)
};

#endif
