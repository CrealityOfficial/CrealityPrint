#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QFile>
#include <QDebug>
#include <QQuickWindow>
#include <QDir>
#include <QQmlContext>
#include <QTranslator>
void InitUiByLanguage();
int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QGuiApplication app(argc, argv);

//    InitUiByLanguage();

    QQmlApplicationEngine engine;
    QString path = QCoreApplication::applicationDirPath() + "/plugin";
    engine.addImportPath(QCoreApplication::applicationDirPath() + "/plugin");

    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);


    engine.load(url);

    QFile file(":/UI/qml/images/move.png");

    if(file.exists())
    {
        qDebug()<<"--";
    }
    return app.exec();
}

void InitUiByLanguage()
{

    QTranslator translator;
    QString strLanguageFile;

    qDebug()<<"qApp->applicationDirPath() =" << qApp->applicationDirPath();
    strLanguageFile = "E:/work_Creality/Creative3D/locales/zh_CN.ts";
    if (QFile(strLanguageFile).exists())
    {
        translator.load(strLanguageFile);
        qApp->installTranslator(&translator);
    }
    else
    {
        qDebug() << "[houqd] authclient language file does not exists ...";
    }
}
