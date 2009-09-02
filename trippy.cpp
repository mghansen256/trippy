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

#include "trippy.h"

Trippy::Trippy()
: m_window(0), m_fileDialog(0), m_photos(), m_watcher()
{
  m_window = new Window();
  m_window->show();
  
  m_window->ui.lv_photos->setModel(&m_photos);
  m_window->m_marble->setPhotoModel(&m_photos);
  m_window->m_marble->setSelectionModel(m_window->ui.lv_photos->selectionModel());

  qRegisterMetaType<Photo>("Photo");

  m_watcher = new QFutureWatcher<QString>(this);

  QObject::connect(m_window, SIGNAL(selectedFiles(const QStringList &)), this, SLOT(filesSelected(const QStringList &)));
  connect(this, SIGNAL(photoReady(Photo)), this, SLOT(addPhoto(Photo)));
}

struct LoadImageHelper
{
  LoadImageHelper(QStandardItemModel *model, LoadScreen *loadScreen, Trippy *trippy)
      : m_model(model), m_loadScreen(loadScreen), m_trippy(trippy)
  {
  }

  typedef QString result_type;
  QStandardItemModel *m_model;
  LoadScreen *m_loadScreen;
  Trippy *m_trippy;

  QString operator()(const QString &filename)
  {
    qDebug()<<"LoadImageHelper::operator()("<<filename<<")";
    try
    {
      m_trippy->fileLoadingFromConcurrent(filename);
      Photo photo(filename);
      if (photo.isGeoTagged())
      {
        photo.getThumbnailImage(); // cause the thumbnail to be generated
        m_trippy->photoReadyFromConcurrent(photo);
      }
      else
      {
        m_trippy->fileFailedFromConcurrent(filename);
      }
    }
    catch (std::exception& e)
    {
      qDebug()<<"Exception: "<<e.what();
      m_trippy->fileFailedFromConcurrent(filename);
    }

    return filename;
  }

};

// this is a forwarding-function which is called from the thread created by QtConcurrent and transports the data into the GUI-thread
void Trippy::photoReadyFromConcurrent(Photo photo)
{
  qDebug()<<"void Trippy::photoReadyFromConcurrent(Photo photo): "<<photo.getFilename();
  emit(photoReady(photo));
}

// this is a forwarding-function which is called from the thread created by QtConcurrent and transports the data into the GUI-thread
void Trippy::fileFailedFromConcurrent(QString filename)
{
  qDebug()<<"void Trippy::fileFailedFromConcurrent(QString filename): "<<filename;
  emit(fileFailed(filename));
}

void Trippy::filesSelected(const QStringList &selected)
{
  if (selected.isEmpty())
    return;
  
  QStringList sortedFiles = selected;
  sortedFiles.sort();

  LoadScreen *loadScreen = new LoadScreen(m_window, m_watcher);
  connect(this, SIGNAL(fileLoading(QString)), loadScreen, SLOT(setProgressText(QString)));
  connect(this, SIGNAL(fileFailed(QString)), loadScreen, SLOT(addFailedFile(QString)));

  loadScreen->show();

  // do the expensive loading of the EXIF-data and scaling to the thumbnail in separate threads:
  QFuture<QString> resultingNames = QtConcurrent::mapped(sortedFiles, LoadImageHelper(&m_photos, loadScreen, this));
  m_watcher->setFuture(resultingNames);

  loadScreen->exec();

  resultingNames.waitForFinished();

  loadScreen->deleteLater();

  sortPhotos();
  m_window->repaintMarbleWidget();

}

//Takes a Photo object and creates a QStandardItem, then places it into the model.
void Trippy::addPhoto(Photo photo)
{
  QStandardItem *newItem = new QStandardItem(photo.getIcon(), photo.getTimestamp().toString());
  newItem->setEditable(false);
  newItem->setData(QVariant::fromValue(photo), PhotoRole);
  newItem->setData(photo.getTimestamp(), TimestampRole);
  newItem->setData(false, SelectedRole);
  newItem->setData(photo.getFilename(), Qt::ToolTipRole);
  m_photos.appendRow(newItem);
}

void Trippy::sortPhotos()
{
  QList<QStandardItem *> photos;
  
  //Move the items from the model to a list...
  while (m_photos.rowCount() > 0)
  {
    photos.append(m_photos.takeRow(0).first());
  }

  /* A very simple version of insertion sort from wikipedia...

  insertionSort(array A)
    for i = 1 to length[A]-1 do
    begin
        value = A[i]
        j = i-1
        while j ≥ 0 and A[j] > value do
        begin
            A[j + 1] = A[j]
            j = j-1
        end
        A[j+1] = value
    end
  */

  //Using insertion sort to sort the items in the List by timestamp.
  for (int n=0; n < photos.size(); ++n)
  {
    QStandardItem *temp = photos.at(n);
    int j = n - 1;
    while ((j >= 0) && 
          (photos.at(j)->data(TimestampRole).toDateTime() > temp->data(TimestampRole).toDateTime()))
    {
      photos[j + 1] = photos.at(j);
      j = j-1;
    }
    photos[j+1] = temp;
  }

  //Put the sorted items back into the model.
  for (int k=0; k < photos.size(); ++k)
  {
    m_photos.insertRow(k, photos.at(k));
  }
}

void Trippy::fileLoadingFromConcurrent(QString filename)
{
  emit(fileLoading(filename));
}
