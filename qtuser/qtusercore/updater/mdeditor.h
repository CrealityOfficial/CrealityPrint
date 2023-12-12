#ifndef MARKDOWNPARSER_H
#define MARKDOWNPARSER_H

#include <QObject>
#include <QString>
#include <QJSEngine>

class MDEditor : public QObject
{
    Q_OBJECT
public:
    explicit MDEditor(QObject *parent = nullptr);
    Q_INVOKABLE static MDEditor& instance()
    {
        static MDEditor _instance;
        return _instance;
    }

    Q_INVOKABLE QString md2html(QString md);

    Q_INVOKABLE QString readMarkdownFile(QString fname = ":/qtuserqml/res/markdown/example.md");

    void toPreviewMode();

private:
    void initJS();
    void loadJSFileRes(QJSEngine *jsEngine,QString res_filename);
    QJSValue evalJSProgram(QJSEngine *jsEngine,QString program);
    void ModifyHtmlTags();
    void ModifyHtmlTagsStyle();

signals:

private:
    int evalJS_count = 0;
    QString markdown_html;
    QJSEngine myJSEngine;
    static QString defaultMarkdown;
};

#endif // MARKDOWNPARSER_H
