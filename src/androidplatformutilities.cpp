/***************************************************************************
                            androidplatformutilities.cpp  -  utilities for qfield

                              -------------------
              begin                : February 2016
              copyright            : (C) 2016 by Matthias Kuhn
              email                : matthias@opengis.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "androidplatformutilities.h"
#include "androidpicturesource.h"
#include "androidprojectsource.h"

#include <QMap>
#include <QString>
#include <QtAndroid>
#include <QDebug>
#include <QAndroidJniEnvironment>
#include <QDateTime>
#include <QDir>
#include <qgsapplication.h>
#include <qgsmessagelog.h>


AndroidPlatformUtilities::AndroidPlatformUtilities()
{

}

QString AndroidPlatformUtilities::configDir() const
{
  return getIntentExtra( "DOTQGIS2_DIR" );
}

QString AndroidPlatformUtilities::shareDir() const
{
  return getIntentExtra( "SHARE_DIR" );
}

QString AndroidPlatformUtilities::packagePath() const
{
  return getIntentExtra( "PACKAGE_PATH" );
}

QString AndroidPlatformUtilities::qgsProject() const
{
  return getIntentExtra( "QGS_PROJECT" );
}

QString AndroidPlatformUtilities::getIntentExtra( const QString& extra, QAndroidJniObject extras ) const
{
  if ( extras == 0 )
  {
    extras = getNativeExtras();
  }
  if( extras.isValid() )
  {
    QAndroidJniObject extraJni = QAndroidJniObject::fromString( extra );
    extraJni = extras.callObjectMethod( "getString", "(Ljava/lang/String;)Ljava/lang/String;", extraJni.object<jstring>() );
    if ( extraJni.isValid() )
    {
      return extraJni.toString();
    }
  }
  return QString();
}

QAndroidJniObject AndroidPlatformUtilities::getNativeIntent() const
{
  QAndroidJniObject activity = QtAndroid::androidActivity();
  if ( activity.isValid() )
  {
    QAndroidJniObject intent = activity.callObjectMethod( "getIntent", "()Landroid/content/Intent;" );
    return intent;
  }
  return 0;
}

QAndroidJniObject AndroidPlatformUtilities::getNativeExtras() const
{
  QAndroidJniObject intent = getNativeIntent();
  if ( intent.isValid() )
  {
    QAndroidJniObject extras = intent.callObjectMethod( "getExtras", "()Landroid/os/Bundle;" );

    return extras;
  }
  return 0;
}

PictureSource* AndroidPlatformUtilities::getPicture( const QString& prefix )
{
  QAndroidJniObject actionImageCapture = QAndroidJniObject::getStaticObjectField( "android/provider/MediaStore", "ACTION_IMAGE_CAPTURE", "Ljava/lang/String;" );

  QAndroidJniObject intent = QAndroidJniObject( "android/content/Intent", "(Ljava/lang/String;)V", actionImageCapture.object<jstring>() );

  if ( !QDir::root().mkpath( prefix ) )
  {
    QgsApplication::messageLog()->logMessage( tr( "Could not create folder %1" ).arg( prefix ), "QField", Qgis::Critical );
    return nullptr;
  }

  QString filename = QDateTime().toString() + QStringLiteral(".jpg");

  qWarning() << "1";
  QAndroidJniObject storageDir = QAndroidJniObject( "java/io/File", "(Ljava/lang/String;)V", QAndroidJniObject::fromString( prefix ).object<jstring>() );
  qWarning() << "2";
  QAndroidJniObject image = QAndroidJniObject::callStaticObjectMethod( "java/io/File", "createTempFile",
                                                                       "(Ljava/lang/String;Ljava/lang/String;Ljava/io/File;)Ljava/io/File;",
                                                                       QAndroidJniObject::fromString( filename ).object<jstring>(),
                                                                       QAndroidJniObject::fromString( QStringLiteral(".jpg") ).object<jstring>(),
                                                                       storageDir.object<jobject>() );

  qWarning() << "3";
  QString imagePath = image.callObjectMethod( "getAbsolutePath", "()Ljava/lang/String;" ).toString();

  qWarning() << "4";
  QAndroidJniObject photoURI = QAndroidJniObject::callStaticObjectMethod( "android/support/v4/content/FileProvider", "getUriForFile",
                                                                          "(Landroid/content/Context;Ljava/lang/String;Ljava/io/File;)Landroid/net/Uri;",
                                                                          QtAndroid::androidActivity().object(),
                                                                          QAndroidJniObject::fromString( QStringLiteral("com.example.android.fileprovider")).object<jstring>(),
                                                                          image.object()
                                                                          );
  qWarning() << "5";
  QAndroidJniObject extraOutput = QAndroidJniObject::getStaticObjectField( "android/provider/MediaStore", "EXTRA_OUTPUT", "Ljava/lang/String;" );

  qWarning() << "6";
  intent.callObjectMethod( "putExtra", "(Ljava/lang/String;Landroid/os/Parcelable)Landroid/content/Intent;", extraOutput.object<jstring>(), photoURI.object<jobject>() );

  qWarning() << "7";
  AndroidPictureSource* pictureSource = nullptr;

  if ( actionImageCapture.isValid() && intent.isValid() )
  {
    pictureSource = new AndroidPictureSource( imagePath );
    QtAndroid::startActivity( intent.object<jobject>(), 101, pictureSource );
  }
  else
  {
    qDebug() << "Something went wrong creating the picture request";
  }

  return pictureSource;
}

void AndroidPlatformUtilities::open( const QString& data, const QString& type )
{
  QAndroidJniObject actionView = QAndroidJniObject::getStaticObjectField( "android/intent/action", "ACTION_VIEW", "Ljava/lang/String;" );

  QAndroidJniObject intent = QAndroidJniObject( "android/content/Intent", "(Ljava/lang/String;)V", actionView.object<jstring>() );

  QAndroidJniObject jDataString = QAndroidJniObject::fromString( data );
  QAndroidJniObject jType = QAndroidJniObject::fromString( type );

  QAndroidJniObject jData = QAndroidJniObject::callStaticObjectMethod( "android/net/Uri", "parse", "(Ljava/lang/String;)Landroid/net/Uri;", jDataString.object<jstring>() );

  intent.callObjectMethod( "setDataAndType", "(Landroid/net/Uri;Ljava/lang/String;)Landroid/content/Intent;", jData.object<jobject>(), jType.object<jstring>() );

  QtAndroid::startActivity( intent.object<jobject>(), 102 );
}

ProjectSource *AndroidPlatformUtilities::openProject()
{
  QAndroidJniObject actionOpenDocument = QAndroidJniObject::getStaticObjectField( "android/content/Intent", "ACTION_OPEN_DOCUMENT", "Ljava/lang/String;" );
  QAndroidJniObject categoryOpenable = QAndroidJniObject::getStaticObjectField( "android/content/Intent", "CATEGORY_OPENABLE", "Ljava/lang/String;" );

  QAndroidJniObject intent = QAndroidJniObject( "android/content/Intent", "(Ljava/lang/String;)V", actionOpenDocument.object<jstring>() );
  intent.callObjectMethod( "addCategory", "(Ljava/lang/String;)Landroid/content/Intent;", categoryOpenable.object<jstring>() );
  QAndroidJniObject mimeType = QAndroidJniObject::fromString( QStringLiteral( "application/*" ) );
  intent.callObjectMethod( "setType", "(Ljava/lang/String;)Landroid/content/Intent;", mimeType.object<jstring>() );

  AndroidProjectSource* projectSource = nullptr;

  if ( intent.isValid() )
  {
    projectSource = new AndroidProjectSource();
    QtAndroid::startActivity( intent.object<jobject>(), 103, projectSource );
  }
  else
  {
    qDebug() << "Something went wrong creating the project intent";
  }

  return projectSource;
}
