/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef GUIDELINECONTROLLER_H
#define GUIDELINECONTROLLER_H

#include <QObject>
#include <qglobal.h>

#include "qgsgeometry.h"
#include "qgsquickmapsettings.h"

class GuidelineController : public QObject
{
    Q_OBJECT

    // input properties (geometry of real feature and curent crosshair position in map CRS)
    Q_PROPERTY( QgsGeometry realGeometry READ realGeometry WRITE setRealGeometry NOTIFY realGeometryChanged )
    Q_PROPERTY( QPointF crosshairPosition READ crosshairPosition WRITE setCrosshairPosition NOTIFY crosshairPositionChanged )
    Q_PROPERTY( QgsQuickMapSettings *mapSettings READ mapSettings WRITE setMapSettings NOTIFY mapSettingsChanged )

    // output properties (real geometry + crosshair position ) in map CRS
    Q_PROPERTY( QgsGeometry guidelineGeometry READ guidelineGeometry WRITE setGuidelineGeometry NOTIFY guidelineGeometryChanged )

  public:
    explicit GuidelineController( QObject *parent = nullptr );

    const QgsGeometry &guidelineGeometry() const;
    void setGuidelineGeometry( const QgsGeometry &newGuidelineGeometry );

    QPointF crosshairPosition() const;
    void setCrosshairPosition( QPointF newCrosshairPosition );

    const QgsGeometry &realGeometry() const;
    void setRealGeometry( const QgsGeometry &newRealGeometry );

    QgsQuickMapSettings *mapSettings() const;
    void setMapSettings( QgsQuickMapSettings *newMapSettings );

  signals:

    void guidelineGeometryChanged( const QgsGeometry &guidelineGeometry );
    void crosshairPositionChanged( QPointF crosshairPosition );

    void realGeometryChanged( const QgsGeometry &realGeometry );

    void mapSettingsChanged( QgsQuickMapSettings *mapSettings );

  private:
    void buildGuideline();

    QgsGeometry mGuidelineGeometry;
    QPointF mCrosshairPosition;
    QgsGeometry mRealGeometry;
    QgsQuickMapSettings *mMapSettings = nullptr; // not owned
};

#endif // GUIDELINECONTROLLER_H
