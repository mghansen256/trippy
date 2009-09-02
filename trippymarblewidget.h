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

#ifndef TRIPPYMARBLEWIDGET_H
#define TRIPPYMARBLEWIDGET_H

#include <MarbleWidget.h>
#include <GeoPainter.h>
#include <QStandardItemModel>

#include "photo.h"
#include "roles.h"
#include <QItemSelectionModel>

using namespace Marble;

class MarkerClusterHolder;

class TrippyMarbleWidget : public MarbleWidget
{
  Q_OBJECT
  
  public:
    TrippyMarbleWidget(QWidget *parent=0);
    void setPhotoModel(QStandardItemModel *model);
    void setSelectionModel(QItemSelectionModel *model);

  public slots:
    void slotSetUseClustering(const bool doIt);
    
  protected:
    void customPaint(GeoPainter *painter);

  private slots:
    void slotModelRowsAdded(const QModelIndex& parent, int start, int end);
    void slotModelRowsAboutToBeRemoved(const QModelIndex& parent, int start, int end);
    
  private:
    QStandardItemModel *m_photoModel;
    QItemSelectionModel *m_selectionModel;
    MarkerClusterHolder *m_markerClusterHolder;
    bool m_useClustering;
    
  private:
    Q_DISABLE_COPY(TrippyMarbleWidget)
};

#endif
