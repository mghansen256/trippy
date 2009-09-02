/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2009-08-28
 * Description : clustering of markers support for worldmapwidget
 *
 * Copyright (C) 2009 by Michael G. Hansen <mhansen at mghansen dot de>
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation;
 * either version 2, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * ============================================================ */

// C++ includes
#include <algorithm>

// Qt includes
#include <QMouseEvent>
#include <QToolTip>

// KDE includes
#include <kdebug.h>

// Marble includes
#include <marble/MarbleMap.h>
#include <marble/GeoPainter.h>

// local includes
#include "markerclusterholder.h"
#include "markerclusterholderplugin/externaldraw.h"

// constants for clusters
const int ClusterRadius = 15;
const QSize ClusterDefaultSize = QSize(2*ClusterRadius, 2*ClusterRadius);
const int ClusterGridSizeScreen = 60;
const QSize ClusterMaxPixmapSize = QSize(60, 60);

class MarkerClusterHolderPrivate
{
  public:
    Marble::MarbleWidget* marbleWidget;
    QList<MarkerClusterHolder::ClusterInfo> clusters;
    QList<MarkerClusterHolder::MarkerInfo> markers;
    int lastZoom;
    qreal lastCenterLatitude;
    qreal lastCenterLongitude;
    int markerCountDirty;
    bool autoRedrawOnMarkerAdd;
    bool clusterStateDirty;
    bool haveAnySoloMarkers;
    MarkerClusterHolder::MarkerDataEqualFunction markerDataEqual;
    void* markerDataEqualData;
    bool allowSelection;
    bool allowFiltering;
    MarkerClusterHolder::TooltipFunction tooltipFunction;
    void* tooltipFunctionData;
    MarkerClusterHolder::ClusterPixmapFunction clusterPixmapFunction;
    void* clusterPixmapFunctionData;
    Marble::ExternalDrawPlugin* externalDrawPlugin;
    
    MarkerClusterHolderPrivate(Marble::MarbleWidget* parameterMarbleWidget)
    : marbleWidget(parameterMarbleWidget),
      clusters(),
      markers(),
      lastZoom(-1),
      lastCenterLatitude(marbleWidget->centerLatitude()),
      lastCenterLongitude(marbleWidget->centerLongitude()),
      markerCountDirty(true),
      autoRedrawOnMarkerAdd(true),
      clusterStateDirty(false),
      haveAnySoloMarkers(false),
      markerDataEqual(0),
      markerDataEqualData(0),
      allowSelection(true),
      allowFiltering(true),
      tooltipFunction(0),
      tooltipFunctionData(0),
      clusterPixmapFunction(0),
      clusterPixmapFunctionData(0),
      externalDrawPlugin(0)
    {
    }
    
    
  private:
    Q_DISABLE_COPY(MarkerClusterHolderPrivate)
    
};

/**
 * @brief Callback function for custom painting, called by ExternalDrawPlugin
 * @param painter Painter to paint clusters on
 * @param yourdata Pointer to MarkerClusterHolder
 */
void MarkerClusterHolder::ExternalDrawCallback(Marble::GeoPainter *painter, void* yourdata)
{
  MarkerClusterHolder* const myMCH = reinterpret_cast<MarkerClusterHolder*>(yourdata);
  if (!myMCH)
    return;
  
  myMCH->paintOnMarbleInternal(painter);
}

/**
 * @brief Constructs a MarkerClusterHolder
 * @param marbleWidget Map on which the clusters should be shown. Will also be used as parent for this object.
 */
MarkerClusterHolder::MarkerClusterHolder(Marble::MarbleWidget* const marbleWidget)
  : QObject(marbleWidget), d(new MarkerClusterHolderPrivate(marbleWidget))
{
  d->marbleWidget->installEventFilter(this);
  
  // try to find the ExternalDrawPlugin
  d->externalDrawPlugin = Marble::ExternalDrawPlugin::findPluginInstance(d->marbleWidget);
  if (d->externalDrawPlugin)
  {
    d->externalDrawPlugin->setRenderCallback(ExternalDrawCallback, this);
  }
}

MarkerClusterHolder::~MarkerClusterHolder()
{
  // remove the callback function from the ExternalDrawPlugin
  if (d->externalDrawPlugin)
  {
    d->externalDrawPlugin->setRenderCallback(0, 0);
  }
}

/**
 * @brief Adds a marker
 * @param marker Marker to be added
 */
void MarkerClusterHolder::addMarker(const MarkerInfo& marker)
{
  d->markers<<marker;
  d->markerCountDirty = true;
  redrawIfNecessary();
}

/**
 * @brief Adds a list of markers
 * @param markerList List of markers to be added
 */
void MarkerClusterHolder::addMarkers(const QList<MarkerInfo>& markerList)
{
  d->markers<<markerList;
  d->markerCountDirty = true;
  redrawIfNecessary();
}

/**
 * @brief Removes a range of markers
 * @param start Start index
 * @param end End index (inclusive!)
 */
void MarkerClusterHolder::removeMarkers(const int start, const int end)
{
  for (int i = end; i>=start; --i)
  {
    d->markers.removeAt(i);
  }
  d->markerCountDirty = true;
  redrawIfNecessary();
}

/**
 * @brief Removes a list of markers identitifed by their indices
 *
 * Note that the indices have to be sorted in ascending order
 *
 * @param markerIndices List of indices of markers to be remove
 */
void MarkerClusterHolder::removeMarkers(const QIntList& markerIndices)
{
  for (int i = markerIndices.size()-1; i>=0; --i)
  {
    d->markers.removeAt(markerIndices.at(i));
  }
  d->markerCountDirty = true;
  redrawIfNecessary();
}

/**
 * @brief Removes a list of markers
 * @param markerList List of markers to be remove
 */
void MarkerClusterHolder::removeMarkers(const QList<MarkerInfo>& markerList)
{
  MarkerInfo::List markersToDelete = markerList;
  int i = 0;
  while ((i<d->markers.count())&&!markersToDelete.isEmpty())
  {
    bool deletedOne = false;
    for (int di = 0; di<markersToDelete.count(); ++di)
    {
      if (markersEqual(d->markers.at(i), markersToDelete.at(di)))
      {
        d->markers.removeAt(i);
        markersToDelete.removeAt(di);
        deletedOne = true;
        break;
      }
    }
    if (!deletedOne)
    {
      ++i;
    }
  }
  
  d->markerCountDirty = true;
  redrawIfNecessary();
}

/**
 * @brief Returns the label for this cluster
 * @return The label for this cluster
 */
QString MarkerClusterHolder::ClusterInfo::getLabelText() const
{
  const int nMarkers = markerCount();
  
  QString text;
  if (nMarkers<1000)
  {
    text = QString::number(nMarkers);
  }
  else if ((nMarkers>=1000)&&(nMarkers<=1950))
  {
    // TODO: use KDE-versions instead
    text = QString("%L1k").arg(qreal(nMarkers)/1000.0, 0, 'f', 1);
  }
  else if ((nMarkers>=1951)&&(nMarkers<19500))
  {
    // TODO: use KDE-versions instead
    text = QString("%L1k").arg(qreal(nMarkers)/1000.0, 0, 'f', 0);
  }
  else
  {
    // convert to "1E5" notation for numbers >=20k:
    qreal exponent = floor(log(nMarkers)/log(10));
    qreal nMarkersFirstDigit=round(qreal(nMarkers)/pow(10,exponent));
    if (nMarkersFirstDigit>=10)
    {
      nMarkersFirstDigit=round(nMarkersFirstDigit/10.0);
      exponent++;
    }
    text = QString("%1E%2").arg(int(nMarkersFirstDigit)).arg(int(exponent));
  }
  return text;
}

/**
  * @brief Determines the colors for this cluster
  * @param haveAnySolo Are there any markers that are solo?
  * @param fillColor Color used to fill the circle
  * @param strokeColor Color used for the stroke around the circle
  * @param strokeStyle Style used to draw the stroke around the crircle
  * @param labelText Text for the label
  * @param labelColor Color for the label text
  */
void MarkerClusterHolder::ClusterInfo::getColorInfos(const bool haveAnySolo, QColor *fillColor, QColor *strokeColor, Qt::PenStyle *strokeStyle, QString *labelText, QColor *labelColor) const
{
  *labelText = getLabelText();
  *labelColor = QColor(Qt::black);
  
  switch (selected)
  {
    case PartialNone:
      *strokeStyle = Qt::NoPen;
      break;
    case PartialSome:
      *strokeStyle = Qt::DotLine;
      break;
    case PartialAll:
      *strokeStyle = Qt::SolidLine;
      break;
  }
  *strokeColor = QColor(Qt::blue);
  
  QColor fillAll, fillSome, fillNone;
  const int nMarkers = markerCount();
  if (nMarkers>=100)
  {
    fillAll  = QColor(255, 0, 0);
    fillSome = QColor(255, 188, 125);
    fillNone = QColor(255, 185, 185);
  }
  else if (nMarkers>=50)
  {
    fillAll  = QColor(255, 127, 0);
    fillSome = QColor(255, 190, 125);
    fillNone = QColor(255, 220, 185);
  }
  else if (nMarkers>=10)
  {
    fillAll  = QColor(255, 255, 0);
    fillSome = QColor(255, 255, 105);
    fillNone = QColor(255, 255, 185);
  }
  else if (nMarkers>=2)
  {
    fillAll  = QColor(0, 255, 0);
    fillSome = QColor(125, 255, 125);
    fillNone = QColor(185, 255, 255);
  }
  else
  {
    fillAll  = QColor(0, 255, 255);
    fillSome = QColor(125, 255, 255);
    fillNone = QColor(185, 255, 255);
  }
  
  switch (solo)
  {
    case PartialAll:
      *fillColor = fillAll;
      break;
    case PartialSome:
      *fillColor = fillSome;
      break;
    case PartialNone:
      if (haveAnySolo)
      {
        *fillColor = fillNone;
      }
      else
      {
        *fillColor = fillAll;
      }
      break;
  }
}

/**
 * @brief Paints the clusters on MarbleWidget
 *
 * Call this function from a customPaint of a Marble::MarbleWidget subclass
 * If you do not want to subclass Marble::MarbleWidget yourself, use
 * MarbleSubClassWidget
 *
 * @param painter Painter on which the clusters should be painted
 */
void MarkerClusterHolder::paintOnMarble(Marble::GeoPainter* const painter)
{
  if (!d->externalDrawPlugin)
  {
    paintOnMarbleInternal(painter);
  }
}

/**
 * @brief Paints the clusters on MarbleWidget
 *
 * This function is called internally either from paintOnMarble or from
 * the externaldraw-plugin, if found
 *
 * @param painter Painter on which the clusters should be painted
 */
void MarkerClusterHolder::paintOnMarbleInternal(Marble::GeoPainter* const painter)
{
  // reorder the clusters if necessary
  reorderClusters();
  
  painter->save();
  painter->autoMapQuality();
  
  // determine the color:
  QPen labelPen;
  QPen circlePen;
  
  // draw all clusters:
  for (QList<ClusterInfo>::iterator it = d->clusters.begin(); it!=d->clusters.end(); ++it)
  {
    const ClusterInfo& cluster = *it;
    
    const int radius = ClusterRadius;// + cluster.markerCount()/10.0;
    
    int clusterX = cluster.pixelPos.x();
    int clusterY = cluster.pixelPos.y();
    
    bool havePixmap = false;
    QPixmap clusterPixmap;
    // should we draw a pixmap instead?
    if (d->clusterPixmapFunction)
    {
      const QSize maxPixmapSize = cluster.maxSize;
      havePixmap = d->clusterPixmapFunction(cluster, &(d->markers), maxPixmapSize, d->clusterPixmapFunctionData, &clusterPixmap);
    }
    
    // determine the color:
    QColor fillColor;
    QColor strokeColor;
    Qt::PenStyle strokeStyle;
    QColor labelColor;
    QString labelText;
    
    cluster.getColorInfos(d->haveAnySoloMarkers, &fillColor, &strokeColor, &strokeStyle, &labelText, &labelColor);
    
    if (havePixmap)
    {
      // is the cluster partially hidden?
      if (d->haveAnySoloMarkers && (cluster.solo!=ClusterInfo::PartialAll) )
      {
        // TODO: is there a smarter way to add an alpha-channel?
        // Qt documentation warns that this way is expensive...
        QPixmap alphaPixmap(clusterPixmap.size());
        alphaPixmap.fill(QColor::fromRgb(0x80,0x80,0x80));
        clusterPixmap.setAlphaChannel(alphaPixmap);
      }
      
      const int pixmapX = clusterX - clusterPixmap.width()/2;
      const int pixmapY = clusterY - clusterPixmap.height()/2;
      if (cluster.selected!=ClusterInfo::PartialNone)
      {
        circlePen.setColor(strokeColor);
        circlePen.setStyle(strokeStyle);
        circlePen.setWidth(2);
        painter->setPen(circlePen);
        // size of the rectangle is the size of the filled area, the border is drawn around it
        painter->drawRect(pixmapX, pixmapY, clusterPixmap.width(), clusterPixmap.height());
      }
      painter->drawPixmap(pixmapX, pixmapY, clusterPixmap);
      
      // save the size of the pixmap back to the cluster, because it defines the bounding box:
      it->lastSize = clusterPixmap.size();
    }
    else
    {
      circlePen.setColor(strokeColor);
      circlePen.setStyle(strokeStyle);
      circlePen.setWidth(2);
      labelPen.setColor(labelColor);
    
      painter->setPen(circlePen);
      painter->setBrush(QBrush(fillColor));
      painter->drawEllipse(clusterX-radius, clusterY-radius, 2*radius, 2*radius);

      painter->setPen(labelPen);
      painter->drawText(QRect(clusterX-radius,clusterY-radius,2*radius,2*radius), Qt::AlignHCenter|Qt::AlignVCenter, labelText);
      
      // we used the default size of the cluster:
      it->lastSize = ClusterDefaultSize;
    }
  }
  
  painter->restore();
}

/**
 * @brief Clears all markers and clusters
 */
void MarkerClusterHolder::clear()
{
  d->clusters.clear();
  d->markers.clear();
  d->markerCountDirty = true;
  d->haveAnySoloMarkers = false;
  emit(signalSoloChanged());
  emit(signalSelectionChanged());
  redrawIfNecessary();
}

/**
 * @brief Causes the map to be redrawn if the number of markers has changed and autoRedrawOnMarkerAdd is set
 * @param force Force a redraw
 */
void MarkerClusterHolder::redrawIfNecessary(const bool force)
{
  if ( ( (d->clusterStateDirty||d->markerCountDirty) && d->autoRedrawOnMarkerAdd ) || force )
  {
    d->marbleWidget->update();
  }
}

/**
 * @brief Returns whether the map should be updated when a marker is added/deleted
 * @return Whether the map is updated automatically
 */
bool MarkerClusterHolder::autoRedrawOnMarkerAdd() const
{
  return d->autoRedrawOnMarkerAdd;
}

/**
 * @brief Set whether the map should be updated when a marker is added/deleted
 * @param doRedraw Whether the map is redrawn on changes
 */
void MarkerClusterHolder::setAutoRedrowOnMarkerAdd(const bool doRedraw)
{
  d->autoRedrawOnMarkerAdd = doRedraw; 
}

/**
 * @brief Reorders the markers into clusters
 */
void MarkerClusterHolder::reorderClusters()
{
//   reorderClustersSimple();
  reorderClustersPixelGrid();
}

#if 0
/**
 * @brief Reorder the clusters if the map has changed
 */
void MarkerClusterHolder::reorderClustersSimple()
{
  // check whether the parameters of the map changed:
  const int newZoom = d->marbleWidget->zoom();
  const qreal newCenterLatitude = d->marbleWidget->centerLatitude();
  const qreal newCenterLongitude = d->marbleWidget->centerLongitude();
  
  if (! ((newZoom!=d->lastZoom)||(newCenterLatitude!=d->lastCenterLatitude)||(newCenterLongitude!=d->lastCenterLongitude)||d->markerCountDirty) )
  {
    // no big changes, check if highlighting has changed:
    updateClusterHighlighting();
    
    return;
  }
  
  // save map settings:
  d->lastZoom = newZoom;
  d->lastCenterLatitude = newCenterLatitude;
  d->lastCenterLongitude = newCenterLongitude;
  d->markerCountDirty = true;
  
  // clear all clusters:
  d->clusters.clear();
  
  // re-add the markers to clusters:
  for (QList<MarkerInfo>::const_iterator itMarker = d->markers.constBegin(); itMarker!=d->markers.constEnd(); ++itMarker)
  {
    const MarkerInfo& marker = *itMarker;
    
    const qreal m_gridSize = 30;
    // get the screen coordinates and check whether the marker is on screen:
    qreal markerX, markerY;
    if (!d->marbleWidget->screenCoordinates(marker.lon, marker.lat, markerX, markerY))
      continue;
    
    // any existing cluster?
    bool foundCluster = false;
    for (QList<ClusterInfo>::iterator itCluster = d->clusters.begin(); itCluster!=d->clusters.end(); ++itCluster)
    {
      ClusterInfo& cluster = *itCluster;
      qreal clusterX, clusterY;
      if (!d->marbleWidget->screenCoordinates(cluster.lon, cluster.lat, clusterX, clusterY))
        continue; // this should not happen, but lets make sure!
      
      // test whether the marker belongs into the cluster:
      if ( ( abs(markerX-clusterX)<=m_gridSize ) && ( abs(markerY-clusterY)<=m_gridSize ) )
      {
        // marker found:
        cluster.addMarker(marker);
        foundCluster = true;
        break;
      }
    }
      
    if (!foundCluster)
    {
      // add new cluster:
      ClusterInfo info;
      info.addMarker(marker);
      d->clusters << info;
    }
  }
  
  updateClusterHighlighting(true);
  
  kDebug(50003) << QString("MarkerClusterHolder::reorderClusters(): %1 markers in %2 clusters").arg(d->markers.size()).arg(d->clusters.count());
}
#endif

/**
 * @brief Helper function, returns the square of the distance between two points
 *
 * @param a Point a
 * @param b Point b
 * @return Square of the distance between a and b
 */
inline int QPointSquareDistance(const QPoint& a, const QPoint& b)
{
  return (a.x()-b.x())*(a.x()-b.x()) + (a.y()-b.y())*(a.y()-b.y());
}

/**
 * @brief Reorder the clusters if the map has changed
 */
void MarkerClusterHolder::reorderClustersPixelGrid()
{
  // check whether the parameters of the map changed:
  const int newZoom = d->marbleWidget->zoom();
  const qreal newCenterLatitude = d->marbleWidget->centerLatitude();
  const qreal newCenterLongitude = d->marbleWidget->centerLongitude();
   
  if (! ((newZoom!=d->lastZoom)||(newCenterLatitude!=d->lastCenterLatitude)||(newCenterLongitude!=d->lastCenterLongitude)||d->markerCountDirty) )
  {
    // no big changes, check if highlighting has changed:
    updateClusterStates();
    return;
  }
  
  // save map settings:
  d->lastZoom = newZoom;
  d->lastCenterLatitude = newCenterLatitude;
  d->lastCenterLongitude = newCenterLongitude;
  d->markerCountDirty = false;
  
  // clear all clusters:
  d->clusters.clear();
  
  const int gridSizeScreen = ClusterGridSizeScreen;
  const int pixelScaling = 1;
  const int gridSize = gridSizeScreen/pixelScaling;
  
  // add all markers to a grid:
  const QSize mapSize = d->marbleWidget->map()->size();
  const int gridWidth = mapSize.width()/pixelScaling;
  const int gridHeight = mapSize.height()/pixelScaling;
  QVector<QIntList> pixelGrid(gridWidth*gridHeight, QIntList());
  QList<QPair<QPoint, QIntList> > leftOverList;
  for (int i = 0; i<d->markers.count(); ++i)
  {
    const MarkerInfo& marker = d->markers.at(i);
    
    // get the screen coordinates and check whether the marker is on screen:
    qreal markerX, markerY;
    if (!d->marbleWidget->screenCoordinates(marker.lon(), marker.lat(), markerX, markerY))
      continue;
    markerX/=pixelScaling; markerY/=pixelScaling;
    if (markerX<0)
      markerX=0;
    if (markerY<0)
      markerY=0;
    if (markerX>=gridWidth)
      markerX=gridWidth-1;
    if (markerY>=gridHeight)
      markerY=gridHeight-1;
   
    // save the position of the marker:
    pixelGrid[int(markerX)+int(markerY)*gridWidth]<<i;
  }
  
  // TODO: cleanup this list every ... iterations
  QIntList pixelGridIndices;
  for (int i=0; i<gridWidth*gridHeight; ++i)
  {
    if (!pixelGrid[i].isEmpty())
      pixelGridIndices << i;
  }
  
  // re-add the markers to clusters:
  int lastTooCloseClusterIndex = 0;
  while (true)
  {
    int markerMax(0), markerX(0), markerY(0), pixelGridMetaIndexMax = 0;
    
    for (int pixelGridMetaIndex = 0; pixelGridMetaIndex<pixelGridIndices.size(); ++pixelGridMetaIndex)
    {
      const int index = pixelGridIndices[pixelGridMetaIndex];
      if (index<0)
        continue;
      
      if (pixelGrid[index].isEmpty())
      {
        pixelGridIndices[pixelGridMetaIndex] = -1;
        continue;
      }
      
      const int x = index % gridWidth;
      const int y = (index-x)/gridWidth;
      const QPoint markerPosition(x, y);
      
      if (pixelGrid[index].size()>markerMax)
      {
        // only create a cluster here if it is not too close to another cluster:
        bool tooClose = false;
        
        // check the cluster that was a problem last time first:
        if (lastTooCloseClusterIndex<d->clusters.size())
        {
          tooClose = QPointSquareDistance(d->clusters.at(lastTooCloseClusterIndex).pixelPos, markerPosition) < pow(ClusterGridSizeScreen/2, 2);
        }
        
        // now check all other clusters:
        for (int i=0; (!tooClose)&&(i<d->clusters.size()); ++i)
        {
          if (i==lastTooCloseClusterIndex)
              continue;
            
          tooClose = QPointSquareDistance(d->clusters.at(i).pixelPos, markerPosition) < pow(ClusterGridSizeScreen/2, 2);
          if (tooClose)
            lastTooCloseClusterIndex = i;
        }
          
        if (tooClose)
        {
          // move markers into leftover list
          leftOverList << QPair<QPoint, QIntList>(QPoint(x,y), pixelGrid[index]);
          pixelGrid[index].clear();
          pixelGridIndices[pixelGridMetaIndex] = -1;
        }
        else
        {
          markerMax=pixelGrid[x+y*gridWidth].size();
          markerX=x;
          markerY=y;
          pixelGridMetaIndexMax = pixelGridMetaIndex;
        }
      }
    }
    
    if (markerMax==0)
      break;
    
    // create a cluster at this point:
    ClusterInfo cluster;
    cluster.setCenter(d->markers.at(pixelGrid[markerX+markerY*gridWidth].first()));
    cluster.pixelPos = QPoint(markerX, markerY);
    cluster.addMarkerIndices(pixelGrid[markerX+markerY*gridWidth]);
    pixelGrid[markerX+markerY*gridWidth].clear();
    pixelGridIndices[pixelGridMetaIndexMax] = -1;
    
    // absorb all markers around it:
    // Now we only remove the markers from the pixelgrid. They will be cleared from the
    // pixelGridIndices in the loop above
    // make sure we do not go over the grid boundaries:
    const int eatRadius = gridSize/4;
    const int xStart = std::max( (markerX-eatRadius), 0);
    const int yStart = std::max( (markerY-eatRadius), 0);
    const int xEnd = std::min( (markerX+eatRadius), gridWidth-1);
    const int yEnd = std::min( (markerY+eatRadius), gridHeight-1);
    for (int indexX=xStart; indexX<=xEnd; ++indexX)
    {
      for (int indexY=yStart; indexY<=yEnd; ++indexY)
      {
        const int index = indexX + indexY*gridWidth;
        cluster.addMarkerIndices(pixelGrid[index]);
        pixelGrid[index].clear();
      }
    }
    
    d->clusters<<cluster;
  }
  
  // now move all leftover markers into clusters:
  for (QList<QPair<QPoint, QIntList> >::const_iterator it = leftOverList.constBegin(); it!=leftOverList.constEnd(); ++it)
  {
    const QPoint markerPosition = it->first;

    // find the closest cluster:
    int closestSquareDistance = 0;
    int closestIndex = -1;
    for (int i=0; i<d->clusters.size(); ++i)
    {
      const int squareDistance = QPointSquareDistance(d->clusters.at(i).pixelPos, markerPosition);
      if ((closestIndex<0)||(squareDistance<closestSquareDistance))
      {
        closestSquareDistance = squareDistance;
        closestIndex = i;
      }
    }
        
    if (closestIndex>=0)
    {
      d->clusters[closestIndex].addMarkerIndices(it->second);
    }
  }
  
  // compute the distances between the clusters:
  computeClusterDistances();
  
  // highlight the clusters:
  updateClusterStates();
  
  kDebug(50003) << QString("%1 markers in %2 clusters").arg(d->markers.size()).arg(d->clusters.count());
}

void MarkerClusterHolder::computeClusterDistances()
{
  // compute distances only if thumbnails will be used
  if (!d->clusterPixmapFunction)
    return;

  const int nClusters = d->clusters.size();
  int minDistX[nClusters];
  int minDistY[nClusters];
  for (int i=0; i<nClusters; ++i)
  {
    minDistX[i]=ClusterMaxPixmapSize.width();
    minDistY[i]=ClusterMaxPixmapSize.height();
  }
  
  for (int idest = 0; idest<nClusters; ++idest)
  {
    const QPoint destClusterPos = d->clusters.at(idest).pixelPos;

    // only compute distances for clusters behind idest, all distances before that were already calculated
    for (int isource = 0;/*idest+1*/ isource<nClusters; ++isource)
    {
      if (isource==idest)
        continue;
      
      const QPoint sourceClusterPos = d->clusters.at(isource).pixelPos;
      
      const QPoint distance = sourceClusterPos - destClusterPos;
      const int distanceX = abs(distance.x());
      const int distanceY = abs(distance.y());
      
      kDebug(50003)<<QString("cluster %1 - %2: %3, %4").arg(idest).arg(isource).arg(distanceX).arg(distanceY);
      
      if (distanceX>distanceY)
      {
        minDistX[idest] = std::min(minDistX[idest], distanceX);
        if (distanceX<minDistX[idest])
        {
          minDistY[idest] = std::min(minDistY[idest], distanceY);
        }
      }
      else
      {
        minDistY[idest] = std::min(minDistY[idest], distanceY);
        if (distanceY<minDistY[idest])
        {
          minDistX[idest] = std::min(minDistX[idest], distanceX);
        }
      }
//       if ((distanceX<minDistX[idest])&&(distanceY<minDistY[idest]/2))
//       {
//         minDistX[idest] = distanceX;
//       }
//       if ((distanceY<minDistY[idest])&&(distanceX<minDistX[idest]/2))
//       {
//         minDistY[idest] = distanceY;
//       }
//       if ((distanceX<minDistX[isource])&&(distanceY<minDistY[isource]/2))
//       {
//         minDistX[isource] = distanceX;
//       }
//       if ((distanceY<minDistY[isource])&&(distanceX<minDistX[isource]/2))
//       {
//         minDistY[isource] = distanceY;
//       }
    }

    d->clusters[idest].maxSize = QSize(minDistX[idest], minDistY[idest]);
    kDebug(50003)<<QString("cluster %1: %2, %3").arg(idest).arg(minDistX[idest]).arg(minDistY[idest]);
  }
}

/**
 * @brief Clear filtering on all markers
 */
void MarkerClusterHolder::clearFiltering()
{
  for (MarkerInfo::List::iterator destIt = d->markers.begin(); destIt!=d->markers.end(); ++destIt)
  {
    destIt->setSolo(false);
  }
  updateClusterStates();
  redrawIfNecessary();
}

/**
 * @brief Clear selections for all markers
 */
void MarkerClusterHolder::clearSelection()
{
  for (MarkerInfo::List::iterator destIt = d->markers.begin(); destIt!=d->markers.end(); ++destIt)
  {
    destIt->setSelected(false);
  }
  updateClusterStates();
  redrawIfNecessary();
}

/**
 * @brief Sets which markers should be 'solo'
 * @param markerIndicesList Indices of the markers to be changed
 * @param setAsSolo Whether the markers are to be set as solo or not solo
 * @param resetOthers Whether other markers are to be reset to not solo
 */
void MarkerClusterHolder::setSoloMarkers(const QIntList &markerIndicesList, const bool setAsSolo, const bool resetOthers)
{
  // TODO: this function is a convenience function and should be optimized!
  setSoloMarkers(indicesToMarkers(markerIndicesList), setAsSolo, resetOthers);
}

/**
 * @brief Sets which markers should be 'solo'
 * @param markerList List of markers to be changed
 * @param setAsSolo Whether the markers are to be set as solo or not solo
 * @param resetOthers Whether other markers are to be reset to not solo
 */
void MarkerClusterHolder::setSoloMarkers(const MarkerClusterHolder::MarkerInfoList &markerList, const bool setAsSolo, const bool resetOthers)
{
  for (MarkerInfo::List::iterator destIt = d->markers.begin(); destIt!=d->markers.end(); ++destIt)
  {
    // NOTE: do not use QList::contains here, because MarkerInfo contains a QVariant with a custom type, therefore QVariant::operator== does not work!!!
    // is the marker to be selected?
    bool inList = false;
    for (MarkerInfo::List::const_iterator sourceIt = markerList.constBegin(); sourceIt!=markerList.constEnd(); ++sourceIt)
    {
      inList = markersEqual(*destIt, *sourceIt);
      if (inList)
        break;
    }
    if (inList)
    {
      destIt->setSolo( setAsSolo );
    }
    else if (resetOthers)
    {
      destIt->setSolo(false);
    }
  }
  updateClusterStates();
  redrawIfNecessary();
}

/**
 * @brief Returns the markers for a list of indices
 * @param indicesList Indices of the markers
 * @return List of markers for the given indices
 */
MarkerClusterHolder::MarkerInfo::List MarkerClusterHolder::indicesToMarkers(const QIntList indicesList) const
{
  MarkerInfo::List result;
  for (QIntList::const_iterator it = indicesList.constBegin(); it!=indicesList.constEnd(); ++it)
  {
    result << d->markers.at(*it);
  }
  return result;
}

/**
 * @brief returns the currently selected markers
 * @return List of currently selected markers
 */
MarkerClusterHolder::MarkerInfo::List MarkerClusterHolder::selectedMarkers() const
{
  MarkerInfo::List result;
  for (MarkerInfo::List::const_iterator it = d->markers.begin(); it!=d->markers.end(); ++it)
  {
    if (it->isSelected())
      result << *it;
  }
  return result;
}

/**
 * @brief returns the currently solo markers
 * @return List of markers currently marked as 'solo'
 */
MarkerClusterHolder::MarkerInfo::List MarkerClusterHolder::soloMarkers() const
{
  MarkerInfo::List result;
  if (!d->haveAnySoloMarkers)
    return result;
  
  for (MarkerInfo::List::const_iterator it = d->markers.begin(); it!=d->markers.end(); ++it)
  {
    if (it->isSolo())
      result << *it;
  }
  return result;
}

/**
 * @brief Sets which markers should be selected
 * @param markerIndicesList Indices of markers to be changed
 * @param setAsSelected Whether the given markers are to be set as selected or not selected
 * @param resetOthers Whether other markers should be set as not selected
 */
void MarkerClusterHolder::setSelectedMarkers(const QIntList &markerIndicesList, const bool setAsSelected, const bool resetOthers)
{
  // TODO: this function is a convenience function and should be optimized!
  setSelectedMarkers(indicesToMarkers(markerIndicesList), setAsSelected, resetOthers);
}

/**
 * @brief Sets which markers should be selected
 * @param markerList List of markers to be changed
 * @param setAsSelected Whether the given markers are to be set as selected or not selected
 * @param resetOthers Whether other markers should be set as not selected
 */
void MarkerClusterHolder::setSelectedMarkers(const MarkerClusterHolder::MarkerInfoList &markerList, const bool setAsSelected, const bool resetOthers)
{
  for (MarkerInfo::List::iterator destIt = d->markers.begin(); destIt!=d->markers.end(); ++destIt)
  {
    // NOTE: do not use QList::contains here, because MarkerInfo contains a QVariant with a custom type, therefore QVariant::operator== does not work!!!
    // is the marker to be selected?
    bool inList = false;
    for (MarkerInfo::List::const_iterator sourceIt = markerList.constBegin(); sourceIt!=markerList.constEnd(); ++sourceIt)
    {
      inList|= markersEqual(*destIt, *sourceIt);
      if (inList)
        break;
    }
    if (inList)
    {
      destIt->setSelected( setAsSelected );
    }
    else if (resetOthers)
    {
      destIt->setSelected( false );
    }
  }
  updateClusterStates();
  redrawIfNecessary();
}

/**
 * @brief Forwards customPaint requests from Marble::MarbleWidget to MarkerClusterHolder
 * @param painter Painter on which the clusters are to be painted
 */
void MarbleSubClassWidget::customPaint(Marble::GeoPainter* painter)
{
  m_markerClusterHolder->paintOnMarble(painter);
}

/**
 * @brief Updates the selected/solo states of the clusters from the states of the markers
 */
void MarkerClusterHolder::updateClusterStates()
{
  bool newDirtyState = false;
  bool newHaveAnySolo = false;
  for (ClusterInfo::List::iterator clusterIt = d->clusters.begin(); clusterIt!=d->clusters.end(); ++clusterIt)
  {
    const int markerCount = clusterIt->markerCount();
    int selectedMarkersCount = 0;
    int soloMarkersCount = 0;
    for (QIntList::const_iterator indexIt = clusterIt->markerIndices.constBegin(); indexIt!=clusterIt->markerIndices.constEnd(); ++indexIt)
    {
      const MarkerInfo& marker = d->markers.at(*indexIt);
      if (marker.isSelected())
      {
        selectedMarkersCount++;
      }
      if (marker.isSolo())
      {
        soloMarkersCount++;
      }
    }
    
    // compute the state of the cluster:
    ClusterInfo::PartialState selectedState = ClusterInfo::PartialNone;
    if (selectedMarkersCount==markerCount)
    {
      selectedState = ClusterInfo::PartialAll;
    }
    else if (selectedMarkersCount>0)
    {
      selectedState = ClusterInfo::PartialSome;
    }
    
    newHaveAnySolo|= soloMarkersCount>0;
    ClusterInfo::PartialState soloState = ClusterInfo::PartialNone;
    if (soloMarkersCount==markerCount)
    {
      soloState = ClusterInfo::PartialAll;
    }
    else if (soloMarkersCount>0)
    {
      soloState = ClusterInfo::PartialSome;
    }
    
    if ((clusterIt->selected != selectedState)||(clusterIt->solo != soloState))
    {
      newDirtyState = true;
    }
    clusterIt->selected = selectedState;
    clusterIt->solo = soloState;
  }
  d->clusterStateDirty|= newDirtyState;
  d->haveAnySoloMarkers = newHaveAnySolo;
}

/**
 * @brief Finds the cluster shown at position pos
 * @param pos Position of interest
 * @return Index of cluster under position pos, or -1 if no cluster was found
 */
int MarkerClusterHolder::findClusterAt(const QPoint pos) const
{
  // check whether we are in the bounding box of any cluster:
  for (int i=0; i<d->clusters.size(); ++i)
  {
    const ClusterInfo& cluster = d->clusters.at(i);
    const QPoint clusterPosition = d->clusters.at(i).pixelPos;
          
    // is the mouse over the cluster?
    QPoint distance = clusterPosition - pos;
    const bool hasHit = (abs(distance.x()) < cluster.lastSize.width()/2) && (abs(distance.y()) < cluster.lastSize.height()/2);
    if (hasHit)
      return i;
  }
  
  return -1;
}

/**
 * @brief Eventfilter for filtering mouse interactions with Marble::MarbleWidget
 *
 * Gets called by Qt
 *
 * @param obj Pointer to object for which the event occurred
 * @param event Pointer to the event that occurred
 * @return true if the event was filtered by this function
 */
bool MarkerClusterHolder::eventFilter(QObject *obj, QEvent *event)
{
  if ( (event->type() != QEvent::MouseButtonPress) && ( !d->tooltipFunction || (event->type() != QEvent::MouseMove) ) )
  {
    return QObject::eventFilter(obj, event);
  }
  
  QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
  const Qt::KeyboardModifiers currentModifiers = mouseEvent->modifiers();
  const bool shiftPressed = currentModifiers & Qt::ShiftModifier;
  const bool controlPressed = currentModifiers & Qt::ControlModifier;
  const bool leftButtonPressed = mouseEvent->button()==Qt::LeftButton;
  
  bool doFilterEvent = false;
  if ((controlPressed||shiftPressed)&&leftButtonPressed)
  {
    const QPoint mousePos = mouseEvent->pos();
    int clusterIndex = findClusterAt(mousePos);
    if (clusterIndex>=0)
    {
      const ClusterInfo cluster = d->clusters.at(clusterIndex);
      doFilterEvent = true;
      
      if (d->allowSelection&&shiftPressed&&!controlPressed)
      {
        // determine how to change the state:
        ClusterInfo::PartialState selectionState = cluster.selected;
        if ((selectionState==ClusterInfo::PartialNone)||(selectionState==ClusterInfo::PartialSome))
        {
          setSelectedMarkers(cluster.markerIndices, true, false);
        }
        else
        {
          setSelectedMarkers(cluster.markerIndices, false, false);
        }
        emit(signalSelectionChanged());
      }
      else if (d->allowFiltering&&controlPressed)
      {
        // control: interaction with filtering
        // if shift is pressed: allow selection of multiple filter clusters
        const bool doResetOtherClusters = !shiftPressed;
        
        ClusterInfo::PartialState soloState = cluster.solo;
        if ((soloState==ClusterInfo::ClusterInfo::PartialNone)||(soloState==ClusterInfo::ClusterInfo::PartialSome))
        {
          // mark all markers in the cluster as solo:
          setSoloMarkers(cluster.markerIndices, true, doResetOtherClusters);
        }
        else if (soloState==ClusterInfo::ClusterInfo::PartialAll)
        {
          // mark all markers in the cluster as not solo:
          setSoloMarkers(cluster.markerIndices, false, doResetOtherClusters);
        }
        emit(signalSoloChanged());
      }
    }
  }
  else if (d->tooltipFunction)
  {
    // no button handling to be done, check for tooltips:
    const QPoint mousePos = mouseEvent->pos();
    const int clusterIndex = findClusterAt(mousePos);
    if (clusterIndex>=0)
    {
      const ClusterInfo cluster = d->clusters.at(clusterIndex);
      QString tooltipText = d->tooltipFunction(cluster, &d->markers, d->tooltipFunctionData);
      if (!tooltipText.isEmpty())
      {
        QToolTip::showText(mouseEvent->globalPos(), tooltipText);
      }
      else
      {
        QToolTip::hideText();
      }
    }
    else
    {
      QToolTip::hideText();
    }
  }

  if (doFilterEvent)
  {
    return true;
  }
  // standard event processing
  return QObject::eventFilter(obj, event);
}

/**
 * @brief Checks whether two markers can are the same
 *
 * If custom types are used in the data-QVariant, a MarkerDataEqualFunction
 * has to be set using setMarkerDataEqualFunction, otherwise the comparison will fail
 *
 * @param one First marker
 * @param two Second marker
 * @return true if the markers are equal
 * @see setMarkerDataEqualFunction, MarkerDataEqualFunction
 */
bool MarkerClusterHolder::markersEqual(const MarkerInfo& one, const MarkerInfo& two)
{
  if (d->markerDataEqual!=0)
  {
    return d->markerDataEqual(one.m_data, two.m_data, d->markerDataEqualData);
  }
  else
  {
    return (one.lat()==two.lat())&&(one.lon()==two.lon())&&(one.m_data==two.m_data);
  }
}

/**
 * @brief Sets the comparison function for the marker user data
 * @param compareFunction Function which compares the user data parts of two markers
 */
void MarkerClusterHolder::setMarkerDataEqualFunction(const MarkerDataEqualFunction compareFunction, void* const yourdata)
{
  d->markerDataEqual = compareFunction;
  d->markerDataEqualData = yourdata;
}

/**
 * @brief Set whether the user can select clusters for filtering or not
 * @param allow Whether filtering is enabled
 */
void MarkerClusterHolder::setAllowFiltering(const bool allow)
{
  d->allowFiltering = allow;
  if (!allow)
    clearFiltering();
}

/**
 * @brief Set whether the user can select clusters
 * @param allow Whether the user can select clusters
 */
void MarkerClusterHolder::setAllowSelection(const bool allow)
{
  d->allowSelection = allow;
  if (!allow)
    clearSelection();
}

/**
 * @brief Set the tooltip helper function
 *
 * Set this to zero to disable tooltips
 *
 * @param newTooltipFunction Function which returns the tooltip text for a given cluster
 */
void MarkerClusterHolder::setTooltipFunction(TooltipFunction newTooltipFunction, void* const yourdata)
{
  d->tooltipFunction = newTooltipFunction;
  d->tooltipFunctionData = yourdata;
}

/**
 * @brief Set the pixmap generation helper function
 * @param clusterPixmapFunction Function which creates pixmaps for clusters
 * @param yourdata User data for pixmap generation function
 */
void MarkerClusterHolder::setClusterPixmapFunction(const ClusterPixmapFunction clusterPixmapFunction, void* const yourdata)
{
  d->clusterPixmapFunction = clusterPixmapFunction;
  d->clusterPixmapFunctionData = yourdata;
}

