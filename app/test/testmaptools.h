/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef TESTMAPTOOLS_H
#define TESTMAPTOOLS_H

#include <QObject>
#include <qglobal.h>

class TestMapTools : public QObject
{
    Q_OBJECT

  private slots:
    void init();
    void cleanup();

    void testSnapping();
    void testSplitting();
    void testRecording();
};

#endif // TESTMAPTOOLS_H
