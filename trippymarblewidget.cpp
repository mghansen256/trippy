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

#include "trippymarblewidget.h"
#include "trippymarblewidget.moc"

#include <QDebug>
#include <GeoDataPoint.h>
#include "markerclusterholder.h"

TrippyMarbleWidget::TrippyMarbleWidget(QWidget *parent)
  : MarbleWidget(parent), m_photoModel(0), m_selectionModel(0), m_markerClusterHolder(new MarkerClusterHolder(this)), m_useClustering(true)
{
}

void TrippyMarbleWidget::slotSetUseClustering(const bool doIt)
{
  m_useClustering = doIt;
  update();
}

void TrippyMarbleWidget::setPhotoModel(QStandardItemModel *model)
{
  m_photoModel = model;
  connect(m_photoModel, SIGNAL(rowsInserted(const QModelIndex&, int, int)),
          this, SLOT(slotModelRowsAdded(const QModelIndex&, int, int)));
  connect(m_photoModel, SIGNAL(rowsAboutToBeRemoved(const QModelIndex&, int, int)),
          this, SLOT(slotModelRowsAboutToBeRemoved(const QModelIndex&, int, int)));
  connect(m_photoModel, SIGNAL(modelReset()),
          m_markerClusterHolder, SLOT(clear()));
}

template<> MarkerClusterHolder::MarkerInfo MarkerClusterHolder::MarkerInfo::fromData<Photo>(const Photo& yourdata)
{
  return MarkerClusterHolder::MarkerInfo(yourdata.getGpsLong(), yourdata.getGpsLat(), QVariant::fromValue(yourdata));
}

template<> Photo MarkerClusterHolder::MarkerInfo::data<Photo>() const
{
  return m_data.value<Photo>();
}

void TrippyMarbleWidget::slotModelRowsAdded(const QModelIndex& parent, int start, int end)
{
  Q_UNUSED(parent)
  
  qDebug()<<QString("slotModelRowsAdded: start=%1, end=%2").arg(start).arg(end);
  // add the new rows to the clustering system:
  MarkerClusterHolder::MarkerInfo::List markerList;
  for (int i=start; i<=end; ++i)
  {
    const QVariant v = m_photoModel->item(i)->data(PhotoRole);
    const Photo current = v.value<Photo>();
    markerList << MarkerClusterHolder::MarkerInfo::fromData<Photo>(current);
  }
  m_markerClusterHolder->addMarkers(markerList);
}

void TrippyMarbleWidget::slotModelRowsAboutToBeRemoved(const QModelIndex& parent, int start, int end)
{
  Q_UNUSED(parent)
  
  qDebug()<<QString("slotModelRowsAboutToBeRemoved: start=%1, end=%2").arg(start).arg(end);
  m_markerClusterHolder->removeMarkers(start, end);
}

void TrippyMarbleWidget::setSelectionModel(QItemSelectionModel *model)
{
  m_selectionModel = model;
}

void TrippyMarbleWidget::customPaint(GeoPainter *painter)
{
  if (!m_photoModel)
    return; // no photos to display!
    
  if (m_useClustering)
  {
    m_markerClusterHolder->paintOnMarble(painter);
    return;
  }

  QPen pen(Qt::blue);
  pen.setWidth(2);
  painter->setPen(pen);

  GeoDataPoint lastPoint;
  for (int i=0; i<m_photoModel->rowCount(); ++i)
  {
    const QVariant v = m_photoModel->item(i)->data(PhotoRole);
    const Photo current = v.value<Photo>();

    const GeoDataPoint currentPoint = GeoDataPoint(current.getGpsLong(), current.getGpsLat(), 0 , GeoDataCoordinates::Degree);
    painter->drawEllipse(currentPoint, 6, 6);

    if (i > 0)
    {
      painter->drawLine(currentPoint, lastPoint);
    }

    lastPoint = currentPoint;
  }

  // re-draw the selected items to make them stand out
  if (!m_selectionModel)
    return;

  pen.setColor(Qt::red);
  painter->setPen(pen);

  QModelIndexList selectedIndices = m_selectionModel->selectedIndexes();
  for (QModelIndexList::const_iterator it = selectedIndices.begin(); it!=selectedIndices.end(); ++it)
  {
    const QVariant v = m_photoModel->itemFromIndex(*it)->data(PhotoRole);
    const Photo current = v.value<Photo>();
    const GeoDataPoint currentPoint = GeoDataPoint(current.getGpsLong(), current.getGpsLat(), 0 , GeoDataCoordinates::Degree);
    painter->drawEllipse(currentPoint, 6, 6);

    if (it->row()>0)
    {
      const QVariant v = m_photoModel->item(it->row()-1)->data(PhotoRole);
      const Photo previous = v.value<Photo>();
      const GeoDataPoint previousPoint = GeoDataPoint(previous.getGpsLong(), previous.getGpsLat(), 0 , GeoDataCoordinates::Degree);
      painter->drawLine(currentPoint, previousPoint);
    }
  }
}
