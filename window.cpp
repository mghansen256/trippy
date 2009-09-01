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

#include "window.h"

Window::Window(QWidget *parent)
    :QMainWindow(parent)
{
  ui.setupUi(this);

  setAcceptDrops(true);

  ui.lv_photos->setIconSize(QSize(60, 60));
  m_marble = new TrippyMarbleWidget(this);
  QSizePolicy marblePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  marblePolicy.setHorizontalStretch(3);
  m_marble->setSizePolicy(marblePolicy);

  //make the actions exclusive (we have to do this here since it can not be done in the UI-editor):
  m_actionGroupMap = new QActionGroup(this);
  m_actionGroupMap->addAction(ui.actionAtlas);
  m_actionGroupMap->addAction(ui.actionOpen_Street_Map);
  m_actionGroupMap->setExclusive(true);

  m_actionGroupProjection = new QActionGroup(this);
  m_actionGroupProjection->addAction(ui.actionFlat);
  m_actionGroupProjection->addAction(ui.actionMercator);
  m_actionGroupProjection->addAction(ui.actionGlobe);
  m_actionGroupProjection->setExclusive(true);

  ui.centralLayout->addWidget(m_marble);
  
  m_fileDialog = new QFileDialog(this, "Select geo-tagged images");
  m_fileDialog->setNameFilter("Image Files (*.jpg)");
  m_fileDialog->setFileMode(QFileDialog::ExistingFiles);  

  // load the settings:
  QSettings appSettings;
  ui.actionZoomOnSelectedPhoto->setChecked(appSettings.value(QLatin1String("ZoomOnSelectedPhoto"), true).toBool());
  m_fileDialog->restoreState(appSettings.value(QLatin1String("AddPhotosState")).toByteArray());
  const int setting_map = appSettings.value(QLatin1String("MapType"), 0).toInt();
  switch (setting_map)
  {
      case 0:
      default:
        ui.actionAtlas->setChecked(true);
        break;
      case 1:
        ui.actionOpen_Street_Map->setChecked(true);
        break;
  }
  const int setting_projection = appSettings.value(QLatin1String("ProjectionType"), 1).toInt();
  switch (setting_projection)
  {
      case 0:
        ui.actionFlat->setChecked(true);
        break;
      default:
      case 1:
        ui.actionMercator->setChecked(true);
        break;
      case 2:
        ui.actionGlobe->setChecked(true);
        break;
  }

  // cause the checked actions to be applied:
  mapActionTriggered(0);
  projectionActionTriggered(0);

  //Add photos button and menu item
  QObject::connect(ui.pb_addPhotos, SIGNAL(clicked()), this, SLOT(selectFile()));
  QObject::connect(ui.action_Add_Photos, SIGNAL(triggered()), this, SLOT(selectFile()));

  //Menubar items:
  QObject::connect(m_actionGroupMap, SIGNAL(triggered(QAction*)), this, SLOT(mapActionTriggered(QAction*)));
  QObject::connect(m_actionGroupProjection, SIGNAL(triggered(QAction*)), this, SLOT(projectionActionTriggered(QAction*)));

  //Files selected from the file dialog
  QObject::connect(m_fileDialog, SIGNAL(filesSelected(const QStringList &)), this, SLOT(filesSelected(const QStringList &)));

  //An item (photo) was clicked in the list view.
  QObject::connect(ui.lv_photos, SIGNAL(clicked(const QModelIndex &)), this, SLOT(photoClicked(const QModelIndex &)));
  
  //Back and Next buttons
  QObject::connect(ui.pb_back, SIGNAL(clicked()), this, SLOT(backClicked()));
  QObject::connect(ui.pb_next, SIGNAL(clicked()), this, SLOT(nextClicked()));

  // get context-menu events on the image list:
  ui.lv_photos->installEventFilter(this);
  ui.lv_photos->setSelectionMode(QAbstractItemView::ExtendedSelection);
}

void Window::hideMapClutter()
{
  m_marble->setShowCompass(false);
  m_marble->setShowScaleBar(false);
  m_marble->setShowOverviewMap(false);
}

void Window::repaintMarbleWidget()
{
  m_marble->repaint();
}

void Window::selectFile()
{
  m_fileDialog->show();
}

void Window::backClicked()
{
  const int rowCount = ((QStandardItemModel *)ui.lv_photos->model())->rowCount();

  if (0 == rowCount)
  {
    return;
  }

  const int currentRow = ui.lv_photos->currentIndex().row();
  QStandardItem *nextItem = ((QStandardItemModel *)ui.lv_photos->model())->item(currentRow-1);
  
  if (!nextItem)
  {
    nextItem = ((QStandardItemModel *)ui.lv_photos->model())->item(rowCount-1);
  }
  
  ui.lv_photos->setCurrentIndex(nextItem->index());
  photoClicked(nextItem->index());
}

void Window::nextClicked()
{
  const int rowCount = ((QStandardItemModel *)ui.lv_photos->model())->rowCount();

  if (0 == rowCount)
  {
    return;
  }

  const int currentRow = ui.lv_photos->currentIndex().row();
  
  QStandardItem *nextItem = ((QStandardItemModel *)ui.lv_photos->model())->item(currentRow+1);
  
  if (!nextItem)
  {
    nextItem = ((QStandardItemModel *)ui.lv_photos->model())->item(0);
  }
  
  ui.lv_photos->setCurrentIndex(nextItem->index());
  photoClicked(nextItem->index());
}

void Window::filesSelected(const QStringList &selected)
{
  ui.pb_next->setEnabled(true);
  ui.pb_back->setEnabled(true);
 
  m_fileDialog->hide();
  emit selectedFiles(selected);
}

void Window::photoClicked(const QModelIndex &index)
{
  const QStandardItemModel * const model = static_cast<QStandardItemModel*>(ui.lv_photos->model());
  const QStandardItem * const item = model->itemFromIndex(index);

  const QVariant v = item->data(PhotoRole);
  const Photo photo = v.value<Photo>();
  centerMapOn(&photo);
}

void Window::centerMapOn(const Photo * const photo)
{
  ui.l_photo->setPixmap(photo->getThumbnailPixmap());
  m_marble->centerOn(photo->getGpsLong(), photo->getGpsLat());
  if (ui.actionZoomOnSelectedPhoto->isChecked())
  {
    m_marble->zoomView(3000);
  }
}

void Window::mapActionTriggered(QAction *action)
{
  Q_UNUSED(action);

  if (ui.actionAtlas->isChecked())
  {
    m_marble->setMapThemeId(QLatin1String("earth/srtm/srtm.dgml"));
  }
  else if (ui.actionOpen_Street_Map->isChecked())
  {
    m_marble->setMapThemeId(QLatin1String("earth/openstreetmap/openstreetmap.dgml"));
  }
  hideMapClutter();
}

void Window::projectionActionTriggered(QAction *action)
{
    Q_UNUSED(action);

    if (ui.actionFlat->isChecked())
    {
      m_marble->setProjection(Equirectangular);
    }
    else if (ui.actionMercator->isChecked())
    {
      m_marble->setProjection(Mercator);
    }
    else if (ui.actionGlobe->isChecked())
    {
      m_marble->setProjection(Spherical);
    }
}

void Window::closeEvent(QCloseEvent *event)
{
  // save the settings:
  QSettings appSettings;

  appSettings.setValue(QLatin1String("ZoomOnSelectedPhoto"), ui.actionZoomOnSelectedPhoto->isChecked());

  int projection_value = 0;
  if (ui.actionFlat->isChecked())
  {
    projection_value = 0;
  }
  else if (ui.actionMercator->isChecked())
  {
    projection_value = 1;
  }
  else if (ui.actionGlobe->isChecked())
  {
    projection_value = 2;
  }
  appSettings.setValue(QLatin1String("ProjectionType"), projection_value);

  int map_value = 0;
  if (ui.actionAtlas->isChecked())
  {
    map_value = 0;
  }
  else if (ui.actionOpen_Street_Map->isChecked())
  {
    map_value = 1;
  }
  appSettings.setValue(QLatin1String("MapType"), map_value);

  appSettings.setValue(QLatin1String("AddPhotosState"), m_fileDialog->saveState());

  event->accept();
}

bool Window::eventFilter(QObject *object, QEvent* event)
{
  if (event->type() == QEvent::ContextMenu)
  {
    QContextMenuEvent * const e = static_cast<QContextMenuEvent*>(event);

    const QModelIndexList selectedStuff = ui.lv_photos->selectionModel()->selectedIndexes();
    qDebug()<<"ContextMenuEvent: "<<selectedStuff.size()<<" items selected";

    if (selectedStuff.isEmpty())
      return true; // nothing selected

    // construct the context menu:
    QMenu * const contextMenu = new QMenu(this);
    contextMenu->addAction(ui.actionRemovePhoto);
    ui.actionCopyCoordinates->setEnabled(ui.lv_photos->selectionModel()->selectedIndexes().size()==1);
    contextMenu->addAction(ui.actionCopyCoordinates);

    contextMenu->exec(e->globalPos());

    return true;
  }
  else
  {
    return QObject::eventFilter(object, event);
  }
}


void Window::on_actionRemovePhoto_triggered()
{
    qDebug()<<"on_actionRemovePhoto_triggered()";

    // remove selected photos from the model:
    const QModelIndexList selectedStuff = ui.lv_photos->selectionModel()->selectedIndexes();
    qDebug()<<"ContextMenuEvent: "<<selectedStuff.size()<<" items selected";

    if (selectedStuff.isEmpty())
      return; // nothing selected

    QStandardItemModel * const stdmodel = static_cast<QStandardItemModel*>(ui.lv_photos->model());
    if (!stdmodel)
    {
      qDebug()<<"model not found";
      return;
    }

    // convert the model-indices to persistent model indices which will stay valid across deletion:
    QList<QPersistentModelIndex> persistentIndices;
    for (QModelIndexList::const_iterator it = selectedStuff.begin(); it!=selectedStuff.end(); ++it) {
       persistentIndices << *it;
    }

    // TODO: this does not work for more than one item!
    for (QList<QPersistentModelIndex>::const_iterator it = persistentIndices.begin(); it!=persistentIndices.end(); ++it)
    {
      stdmodel->removeRow(it->row());
    }

    repaintMarbleWidget();
}

void Window::on_actionCopyCoordinates_triggered()
{
    const QStandardItemModel * const model = static_cast<QStandardItemModel*>(ui.lv_photos->model());
    const QVariant v = model->itemFromIndex(ui.lv_photos->currentIndex())->data(PhotoRole);
    const Photo photo = v.value<Photo>();

    const qreal lat = photo.getGpsLat();
    const qreal lon = photo.getGpsLong();

    const QString coordinates = QString::fromLatin1("%1,%2").arg(lon, 0, 'f', 10).arg(lat, 0, 'f', 10);
    qDebug()<<"Copying coordinates to clipboard: \""<<coordinates<<"\"";

    // importing this representation into Marble does not show anything, but Merkaartor shows the point
    const QString kmlRepresentation = QString::fromLatin1(
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      "<kml xmlns=\"http://www.opengis.net/kml/2.2\">\n"
      "<Document>\n"
      " <Placemark>\n"
      "   <name>%1</name>\n"
      "   <Point>\n"
      "     <coordinates>%2</coordinates>\n"
      "   </Point>\n"
      " </Placemark>\n"
      "</Document>\n"
      "</kml>\n"
      ).arg(photo.getFilename()).arg(coordinates);

    // importing this data into Marble and Merkaartor works
    const QString gpxRepresentation = QString::fromLatin1(
      "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\" ?>\n"
      "<gpx xmlns=\"http://www.topografix.com/GPX/1/1\" creator=\"trippy\" version=\"0.1\"\n"
      " xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
      " xsi:schemaLocation=\"http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd\">\n"
      "  <wpt lat=\"%1\" lon=\"%2\">\n"
//      "   <ele></ele>\n"
//      "   <time></time>\n"
      "   <name>%3</name>\n"
      "  </wpt>\n"
      "</gpx>\n"
      ).arg(lat, 0, 'f', 10).arg(lon, 0, 'f', 10).arg(photo.getFilename());

    QMimeData * const myMimeData = new QMimeData();
    myMimeData->setText(coordinates);
    myMimeData->setData(QLatin1String("application/vnd.google-earth.kml+xml"), kmlRepresentation.toUtf8());
    myMimeData->setData(QLatin1String("application/gpx+xml"), gpxRepresentation.toUtf8());

    QClipboard * const clipboard = QApplication::clipboard();
    clipboard->setMimeData(myMimeData);
}

void Window::dragEnterEvent(QDragEnterEvent *event)
{
  if (event->mimeData()->hasUrls())
  {
    event->setDropAction(Qt::CopyAction); // TODO: is "copy" the correct action? or should we use "link" instead?
    event->accept();
  }
}

void Window::dropEvent(QDropEvent *event)
{
  if (event->mimeData()->hasUrls())
  {
    event->setDropAction(Qt::CopyAction); // TODO: is "copy" the correct action? or should we use "link" instead?
    event->accept();
  }

  // get the list of files to be dropped:
  const QList<QUrl> urlList = event->mimeData()->urls();

  // get the files which we can actually accept:
  QStringList acceptedFiles;
  for (QList<QUrl>::const_iterator it = urlList.begin(); it!=urlList.end(); ++it)
  {
    const QString localFileName = it->toLocalFile();
    if (localFileName.isEmpty())
      continue;

    acceptedFiles<<localFileName;
  }

  // import the files:
  emit selectedFiles(acceptedFiles);
}
