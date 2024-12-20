#include "qtusercore/util/translateutil.h"

#include <QtCore/QCoreApplication>
#include <QtQml/QJSValue>

#include "buildinfo.h"

namespace cx {

  constexpr auto context_cstr{
#ifdef TRANSLATE_CONTEXT
    TRANSLATE_CONTEXT
#else
    PROJECT_NAME
#endif
  };

  const QString context{ context_cstr };

  QString tr(const char* key) {
    return QCoreApplication::translate(context_cstr, key);
  }

  QString tr(const QString& key) {
    auto key_bytes = key.toUtf8();
    return QCoreApplication::translate(context_cstr, key_bytes.constData());
  }

  QString translate(const char* ctx, const char* key) {
    return QCoreApplication::translate(ctx, key);
  }

  QString translate(const QString& ctx, const QString& key) {
    auto ctx_bytes = ctx.toUtf8();
    auto key_bytes = key.toUtf8();
    return QCoreApplication::translate(ctx_bytes.constData(), key_bytes.constData());
  }

  namespace literals {

    QString operator""_tr(const char* key, size_t length) {
      return QCoreApplication::translate(context_cstr, key);
    }

  }  // namespace literals

  bool InitializeQmlTrasnlateUtil(QQmlEngine* engine) {
    if (!engine) {
      return false;
    }

    QJSValue root = engine->globalObject();
    if (!root.isObject()) {
      return false;
    }

    if (root.property(QStringLiteral("cxContext")).toString() != context) {
      root.setProperty(QStringLiteral("cxContext"), QJSValue{ context });
    }

    constexpr auto script = u8R"(
      cxTr = source => qsTranslate(cxContext, source)
      cxTranslate = (context, source) => qsTranslate(context, source)
    )";

    return !engine->evaluate(QString::fromUtf8(script)).isError();
  }

}  // namespace cx
