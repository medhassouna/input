/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef IMAGEUTILS_H
#define IMAGEUTILS_H

#include <QString>

class ImageUtils
{
  public:
    explicit ImageUtils( ) = default;
    ~ImageUtils() = default;

    /**
     * Copies EXIF metadata from sourceImage to targetImage.
     */
    static bool copyExifMetadata( const QString &sourceImage, const QString &targetImage );

    /**
     * Rescales image to the given quality taking into account its orientation
     * and preserving EXIF metadata.
     */
    static bool rescale( const QString &path, int quality );
};

#endif // IMAGEUTILS_H
