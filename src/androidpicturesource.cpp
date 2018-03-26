/***************************************************************************
  androidpicturesource.cpp - AndroidPictureSource

 ---------------------
 begin                : 5.7.2016
 copyright            : (C) 2016 by Matthias Kuhn
 email                : matthias@opengis.ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "androidpicturesource.h"
#include <qgsmessagelog.h>
#include <qgsapplication.h>
#include <QAndroidJniEnvironment>
#include <QtAndroid>
#include <QDir>
#include <QFile>
#include <QDebug>

AndroidPictureSource::AndroidPictureSource(const QString& filePath )
  : PictureSource( nullptr )
  , QAndroidActivityResultReceiver()
  , mFilePath( filePath )
{

}

void AndroidPictureSource::handleActivityResult( int receiverRequestCode, int resultCode, const QAndroidJniObject& data )
{
  jint RESULT_OK = QAndroidJniObject::getStaticField<jint>( "android/app/Activity", "RESULT_OK" );
  if ( receiverRequestCode == 101 && resultCode == RESULT_OK )
  {
    emit pictureReceived( mFilePath );
  }
  else
  {
    emit pictureReceived( QString::null );
  }
}
