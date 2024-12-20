#pragma once

#include <QtCore/QString>
#include <QtQml/QQmlEngine>

#include "qtusercore/qtusercoreexport.h"

/*

translate utils.

cpp example:
```c++
  #include <qtusercore/util/translateutil.h>

  auto foo() {
    QString translation = cx::tr("my key");
    translation = cx::translate(cx::context, "my key");
    translation = cx::translate("my context", "my key");

    using namespace cx::literals;
    translation = "my key"_tr;
  }
```

qml/js example:
```javascript
  // Must call cx::InitializeQmlTrasnlateUtil in cpp first.
  // No need to import any modules in js. cxContext, cxTr, and cxTranslate are in the global space.

  let translation = cxTr('my key')
  translation = cxTranslate(cxContext, 'my key')
  translation = cxTranslate('my context', 'my key')

  QtObject {
    property string myText: cxTr('my key')  // dynamic translate
  }
```

*/

namespace cx {

  QTUSER_CORE_API extern const QString context;

  /// @brief translate with ::cx::context
  QTUSER_CORE_API QString tr(const char* key);
  QTUSER_CORE_API QString tr(const QString& key);

  QTUSER_CORE_API QString translate(const char* context, const char* key);
  QTUSER_CORE_API QString translate(const QString& context, const QString& key);

  namespace literals {

    /// @brief translate with ::cx::context
    QTUSER_CORE_API QString operator""_tr(const char* key, size_t length);

  }  // namespace literals

  /// @brief Initialize some variant in js global space, includes:
  ///   cxContext: string  // same as ::cx::context in cpp
  ///   cxTr: function  // same as ::cx::tr in cpp, work like qsTr in qml
  ///   cxTranslate: function  // same as ::cx::translate in cpp, work like qsTranslate in qml
  /// @return true if initialize successed
  QTUSER_CORE_API bool InitializeQmlTrasnlateUtil(QQmlEngine* engine);

}  // namespace cx
