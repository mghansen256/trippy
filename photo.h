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

#ifndef PHOTO_H
#define PHOTO_H

#include <QMetaType>

#include <QPixmap>
#include <QDateTime>
#include <QDebug>
#include <QImage>
#include <QIcon>

#include <exiv2/image.hpp>
#include <exiv2/exif.hpp>
#include <exiv2/tags.hpp>



class Photo
{
  public:
    Photo(const QString &path = 0);
    inline bool isGeoTagged() const { return ((m_gpsLat != -1) && (m_gpsLong != -1)); }
    QImage getImage() const;
    QPixmap getPixmap() const;
    QImage getThumbnailImage() const;
    QPixmap getThumbnailPixmap() const;
    inline QIcon getIcon() const { return QIcon(getThumbnailPixmap()); }
    inline qreal getGpsLat() const { return m_gpsLat; }
    inline qreal getGpsLong() const { return m_gpsLong; }
    inline QDateTime getTimestamp() const { return m_timestamp; }
    inline QString getFilename() const { return m_filename; }

  private:
    static bool exivHasKey(const QString &key, Exiv2::ExifData &data);
    static qreal convertToCoordinate(const QString &coord, const QString &ref);

    QDateTime m_timestamp;
    qreal m_gpsLat;
    qreal m_gpsLong;
    QString m_filename;
    mutable QImage m_thumbnail;
};

Q_DECLARE_METATYPE(Photo)

#endif

