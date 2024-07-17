#ifndef DUMPTOOL_DUMPTOOL_H_
#define DUMPTOOL_DUMPTOOL_H_

#include <QVariant>
#include <QObject>
#include <QString>

class DumpTool : public QObject {
  Q_OBJECT;

public:
  explicit DumpTool(QObject* parent = nullptr);
  explicit DumpTool(const QString& dump_file_path,
                    const QString& scene_file_path,
                    const QString& user_id,
                    QObject* parent = nullptr);

  virtual ~DumpTool();

public:
  Q_INVOKABLE QString getDumpFilePath() const;
  Q_INVOKABLE QString getApplicationName() const;
  Q_INVOKABLE QString getApplicationVersion() const;
  Q_INVOKABLE QString getApplicationLanguage() const;
  Q_INVOKABLE QString getOperatingSystemName() const;
  Q_INVOKABLE QString getGraphicsCardName() const;
  Q_INVOKABLE QString getOpenglVersion() const;
  Q_INVOKABLE QString getOpenglVendor() const;
  Q_INVOKABLE QString getOpenglRenderer() const;

public:
  Q_INVOKABLE void sendReport();
  Q_SIGNAL void sendReoprtFinished(QVariant successed) const;

private:
  bool prepareDumpFile() const;
  bool prepareSceneFile() const;
  bool prepareInfoJson() const;

private:
  QString dump_file_path_;
  QString dump_file_name_;
  QString dump_dir_path_;
  bool dump_file_ready_;

  QString scene_file_path_;
  QString scene_file_name_;
  QString scene_dir_path_;
  bool scene_file_ready_;

  QString graphics_card_;
  QString opengl_version_;
  QString opengl_vendor_;
  QString opengl_renderer_;

  QString temp_dir_name_;
  QString temp_dir_path_;

  QString info_json_name_;
  QString info_json_path_;

  QString zip_file_name_;
  QString zip_file_path_;
};

#endif // !DUMPTOOL_DUMPTOOL_H_
