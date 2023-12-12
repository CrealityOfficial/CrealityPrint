#include "mdeditor.h"
#include <QFile>
#include <QDebug>

static int QUOTE_INDENT_WIDTH = 30;

static int CODE_X = 35;
static QString CODE_BG = "#F2F2F2";
static QString CODE_PRE_BG = "#EAEAEA";

static QString QUOTE_PRE_BG = "#D6DBDF";

MDEditor::MDEditor(QObject *parent) : QObject(parent)
{
    initJS();
    //toPreviewMode();
}


void MDEditor::loadJSFileRes(QJSEngine *jsEngine,QString res_filename)
{
    QFile requireJSFile(res_filename);
    //qDebug()<<requireJSFile.fileName();
    if (requireJSFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        auto jsStr = QString::fromStdString(requireJSFile.readAll().toStdString());
        //qDebug()<<jsStr.length();
        evalJSProgram(jsEngine,jsStr);
    }
}

QJSValue MDEditor::evalJSProgram(QJSEngine *jsEngine,QString program)
{
    QJSValue result = jsEngine->evaluate(program);
    ++evalJS_count;
    if (result.isError())
        qDebug()<< evalJS_count
                << "Uncaught exception at line"<< result.property("lineNumber").toInt()<< ":" << result.toString();
    return result;
}

QString MDEditor::readMarkdownFile(QString fname)
{
    QFile file(fname);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString error;
        error.append(tr("Log file Not found:")).append(fname);
        qDebug() << error;
        return error;
    }
    QByteArray ba = file.readAll();
    QString md = QString(ba);
    md = md.replace("\n", "      \n");
    md = md.replace("~>", "~      \n>");
    file.close();
    return md;
}

void MDEditor::initJS()
{
    loadJSFileRes(&myJSEngine,":/qtuserqml/res/markdown/js/markdown-it.js");
    loadJSFileRes(&myJSEngine,":/qtuserqml/res/markdown/js/markdown-it-ins.js");
    loadJSFileRes(&myJSEngine,":/qtuserqml/res/markdown/js/markdown-it-sub.js");
    loadJSFileRes(&myJSEngine,":/qtuserqml/res/markdown/js/markdown-it-sup.js");
    loadJSFileRes(&myJSEngine,":/qtuserqml/res/markdown/js/markdown-it-mark.js");

    evalJSProgram(&myJSEngine,"var md = markdownit({html: true,linkify: true,typographer: true})");
    evalJSProgram(&myJSEngine,"md.use(markdownitIns);");
    evalJSProgram(&myJSEngine,"md.use(markdownitSub);");
    evalJSProgram(&myJSEngine,"md.use(markdownitSup);");
    evalJSProgram(&myJSEngine,"md.use(markdownitMark);");
#if 0
    myJSEngine.globalObject().setProperty("text", "++123++**kkkk**");
    QJSValue markdownText = myJSEngine.evaluate("md.render(text)");
    if(!markdownText.isError()) qDebug()<<markdownText.toString();
#endif
}

void MDEditor::ModifyHtmlTags(){
    markdown_html.replace("ins","u");
}

void MDEditor::ModifyHtmlTagsStyle(){
    markdown_html.replace("<a href","<a style='color:#428BCA' href");

//    markdown_html.replace("<p>","<p style='white-space:pre-wrap'>");

    markdown_html.replace("<blockquote>",QString("<blockquote style='margin-left:%1px;'>").arg(QUOTE_INDENT_WIDTH));

    markdown_html.replace("<pre><code>",QString(u8"<pre style='background-color:%1;margin-left:%2px;'><code style='font-family:Source Code Pro,方正悠黑简体 508R;'>").arg(CODE_BG).arg(CODE_X));
    markdown_html.replace("\n</code></pre>","</code></pre>");

    markdown_html.replace("<code>",u8"<code style='background-color:#F9F2F4;color:#c7254e;white-space:pre-wrap;font-family:Source Code Pro,方正悠黑简体 508R;'> ");
    markdown_html.replace(QRegExp("</code>((?!</pre)[^>]*)")," </code>\\1");
}
void MDEditor::toPreviewMode()
{
    myJSEngine.globalObject().setProperty("text", "# Hello World!");
    QJSValue result = myJSEngine.evaluate("md.render(text)");
    if(!result.isError()){
        markdown_html = result.toString();
        //qDebug()<<markdown_html;
        ModifyHtmlTags();
        ModifyHtmlTagsStyle();
        qDebug()<<markdown_html;
    }else{
        markdown_html = "error";
    }
}

QString MDEditor::md2html(QString md)
{
    QString html;
    myJSEngine.globalObject().setProperty("text", md);
    QJSValue result = myJSEngine.evaluate("md.render(text)");
    if(!result.isError()){
        html = result.toString();
        //qDebug()<<markdown_html;
        ModifyHtmlTags();
        ModifyHtmlTagsStyle();
        qDebug()<<markdown_html;
    }else{
        html = tr("Log file error!");
    }
    return html;
}
#if 0
QString MDEditor::defaultMarkdown =QString(\
"# 一级标题 \r\n\
## 二级标题\n\
### 三级标题\
#### 四级标题\
\
**这是加粗的文字**\
\
*这是倾斜的文字*\
\
***这是斜体加粗的文字***\
    \
~~这是加删除线的文字~~\
    \
>这是引用的内容\
    \
[百度](http:\/\/baidu.com)\
    \
<img src=\"https:\/\/www.baidu.com/img/flexible/logo/pc/result.png\">\
    \
- 列表内容\
    \
+ 列表内容\
    \
* 列表内容\
    \
1. 列表内容\
    \
2. 列表内容\
    \
3. 列表内容\
    \
- dd\
    \
- ss\
    \
   - dd\
    \
   - dds\
    \
");
#endif
