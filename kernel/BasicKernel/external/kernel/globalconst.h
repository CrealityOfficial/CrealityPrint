#ifndef CREATIVE_KERNEL_GLOBALCONST_1672882923747_H
#define CREATIVE_KERNEL_GLOBALCONST_1672882923747_H

#include <cxkernel/kernel/const.h>

#include "basickernelexport.h"
#include "data/kernelenum.h"
#include "enginedatatype.h"

namespace creative_kernel {

  class BASIC_KERNEL_API GlobalConst : public cxkernel::CXKernelConst{
    Q_OBJECT;
    Q_PROPERTY(bool customized READ isCustomized CONSTANT);
    Q_PROPERTY(bool cxcloudEnabled READ isCxcloudEnabled CONSTANT);
    Q_PROPERTY(bool upgradeable READ isUpgradeable CONSTANT);
    Q_PROPERTY(bool feedbackable READ isFeedbackable CONSTANT);
    Q_PROPERTY(bool teachable READ isTeachable CONSTANT);
    Q_PROPERTY(bool hollowEnabled READ isHollowEnabled CONSTANT);
    Q_PROPERTY(bool hollowFillEnabled READ isHollowFillEnabled CONSTANT);
    Q_PROPERTY(bool drillable READ isDrillable CONSTANT);
    Q_PROPERTY(bool distanceMeasureable READ isDistanceMeasureable CONSTANT);
    Q_PROPERTY(bool sliceHeightSettingEnabled READ isSliceHeightSettingEnabled CONSTANT);
    Q_PROPERTY(bool partitionPrintEnabled READ isPartitionPrintEnabled CONSTANT);
    Q_PROPERTY(bool laserEnabled READ isLaserEnabled CONSTANT);
    Q_PROPERTY(QString translateContext READ getTranslateContext CONSTANT);

    Q_PROPERTY(bool isDebug READ isDebug CONSTANT);
    Q_PROPERTY(bool isAlpha READ isAlpha CONSTANT);
    Q_PROPERTY(QStringList languages READ languages CONSTANT);
   public:
    explicit GlobalConst(QObject* parent = nullptr);
    virtual ~GlobalConst();

   public:
    QStringList languages();
    bool isAlpha() const;
    bool isCustomized() const;
    bool isCxcloudEnabled() const;
    bool isUpgradeable() const;
    bool isFeedbackable() const;
    bool isTeachable() const;
    bool isHollowEnabled() const;
    bool isHollowFillEnabled() const;
    bool isDrillable() const;
    bool isDistanceMeasureable() const;
    bool isSliceHeightSettingEnabled() const;
    bool isPartitionPrintEnabled() const;
    bool isLaserEnabled() const;
    bool isDebug() const;
    QString getTranslateContext() const;

    int appType() const;
    QString multiColorGuide();
    QString userCoursePath();
    QString getUiParameterDirPath();
    QString userFeedbackWebsite();
    QString calibrationTutorialWebsite();
    QString useTutorialWebsite();
    QString officialWebsite();
    QString getResourcePath(ResourcesType resource);
    QString getEnginePathPrefix();
    EngineType getEngineType() const;
    Q_INVOKABLE int getEngineIntType() const;
    void setEngineType(const EngineType& engineType);
    QString getEngineVersion() const;
    void setEngineVersion(const QString& engineVersion);

    QString languageName(MultiLanguage language);
    QString languageTsFile(MultiLanguage language);
    MultiLanguage tsFile2Language(const QString& tsFile);

    QString themeName(ThemeCategory theme);

    QString moneyType();

    QString cloudTutorialWeb(const QString& name);

    Q_INVOKABLE void writeRegister(const QString& key, const QVariant& value);
    Q_INVOKABLE QVariant readRegister(const QString& key);

   //protected:
    void initialize();

   private:
    void copyOldVersionSettings();

   protected:
    const QStringList m_lanTsFiles;
    const QStringList m_lanNames;

  private:
      EngineType m_engineType = EngineType::ET_CURA;
      QString m_engineVersion;
      QString m_enginePathPrefix;
  };

}  // namespace creative_kernel

#endif // CREATIVE_KERNEL_GLOBALCONST_1672882923747_H
