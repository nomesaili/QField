#include "androidutils.h"

AndroidUtils::AndroidUtils()
{

}

QString AndroidUtils::uriToPath(QAndroidJniObject& uri)
{
  QAndroidJniObject path = uri.callObjectMethod( "getPath", "()Ljava/lang/String;" );
  QAndroidJniObject file = QAndroidJniObject( "java/io/File", "(Ljava/lang/String;)V", path.object<jstring>() );
  QString absolutePath = file.callObjectMethod( "getAbsolutePath", "()Ljava/lang/String;" ).toString();

  if ( absolutePath.startsWith( QStringLiteral( "/document/primary:" ) ) )
  {
    QAndroidJniObject extStorage = QAndroidJniObject::callStaticObjectMethod( "android/os/Environment", "getExternalStorageDirectory", "()Ljava/io/File;" );
    QString extStoragePath = extStorage.callObjectMethod( "getAbsolutePath", "()Ljava/lang/String;" ).toString();

    extStoragePath += '/';

    absolutePath.replace( QStringLiteral( "/document/primary:" ), extStoragePath );
  }

  return absolutePath;
}
