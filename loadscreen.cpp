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

#include "loadscreen.h"

LoadScreen::LoadScreen(QWidget *parent, QFutureWatcher<QString> *watcher)
  :QDialog(parent), ui(), m_watcher(watcher), m_canceled(false)
{
  ui.setupUi(this);
  showFailedPhotos(false);
  setProgressValue(0);
  setProgressRange(0, 0);
  setProgressText(QLatin1String(""));
  clearFailedPhotos();
  showCancel(true);

  connect(m_watcher, SIGNAL(finished()), this, SLOT(finished()));
  connect(m_watcher, SIGNAL(canceled()), this, SLOT(canceled()));
  connect(m_watcher, SIGNAL(progressRangeChanged(int, int)), this, SLOT(setProgressRange(int,int)));
  connect(m_watcher, SIGNAL(progressValueChanged(int)), this, SLOT(setProgressValue(int)));
}

void LoadScreen::showCancel(const bool show)
{
  ui.pb_cancel->setVisible(show);
  ui.pb_close->setVisible(!show);
}

void LoadScreen::clearFailedPhotos()
{
    ui.lw_failPhotos->clear();
}

void LoadScreen::showFailedPhotos(bool show)
{
  ui.gb_failures->setVisible(show);
  adjustSize();
}

void LoadScreen::setProgressRange(int minimum, int maximum)
{
  ui.progressBar->setRange(minimum, maximum);
}

void LoadScreen::setProgressValue(int value)
{
  ui.progressBar->setValue(value);
}

void LoadScreen::setProgressText(QString text)
{
  ui.l_currentPhoto->setText(text);
}

void LoadScreen::addFailedFile(QString filename)
{
  ui.lw_failPhotos->addItem(filename);
  showFailedPhotos(true);
}

void LoadScreen::finished()
{
  showCancel(false);
  if (!m_canceled)
    setProgressText(tr("Finished"));
}

void LoadScreen::canceled()
{
  m_canceled = true;
  showCancel(false);
  setProgressText(tr("Canceled"));
}

void LoadScreen::on_pb_cancel_clicked()
{
    // user wants to cancel the loading-process
  m_watcher->cancel();
}
