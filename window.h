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

#ifndef WINDOW_H
#define WINDOW_H

#include <QtGui>
#include <QStandardItemModel>

#include <MarbleWidget.h>

#include "ui_window.h"
#include "trippymarblewidget.h"
#include "photo.h"
#include "loadscreen.h"

using namespace Marble;

class Window : public QMainWindow
{
  Q_OBJECT
  public:
    Window(QWidget *parent=0);
    Ui::window ui;
    void centerMapOn(const Photo * const photo);
//    void addPhoto(Photo photo);
    void sortPhotos();
    void repaintMarbleWidget();
    TrippyMarbleWidget *m_marble;

  private:
    QFileDialog *m_fileDialog;
    QActionGroup *m_actionGroupMap;
    QActionGroup *m_actionGroupProjection;
 
  private slots:
    void on_actionCopyCoordinates_triggered();
    void on_actionRemovePhoto_triggered();
    void selectFile();
    void filesSelected(const QStringList &files);
    void photoClicked(const QModelIndex &index);
    void hideMapClutter();

    //Buttons/Menu items
    void backClicked();
    void nextClicked();
    void mapActionTriggered(QAction *action);
    void projectionActionTriggered(QAction *action);

  protected:
    virtual void closeEvent(QCloseEvent *event);
    virtual void dragEnterEvent(QDragEnterEvent *event);
    virtual void dropEvent(QDropEvent *event);
  
  public slots:
    bool eventFilter(QObject *object, QEvent* event);


  signals:  
    void selectedFiles(const QStringList &files);
};

#endif
