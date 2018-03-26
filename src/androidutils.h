#ifndef ANDROIDUTILS_H
#define ANDROIDUTILS_H

#include <QString>
#include <QAndroidJniObject>

class AndroidUtils
{
  public:
    AndroidUtils();

  static QString uriToPath( QAndroidJniObject &uri );
};

#endif // ANDROIDUTILS_H
