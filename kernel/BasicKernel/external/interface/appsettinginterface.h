#ifndef CREATIVE_KERNEL_APP_SETTING_INTERFACE_1593676766939_H
#define CREATIVE_KERNEL_APP_SETTING_INTERFACE_1593676766939_H
#include "basickernelexport.h"
#include "utils/namedeclare.h"
#include "data/kernelenum.h"
#include "kernel/enginedatatype.h"
#include <QColor>
#include <QString>

namespace creative_kernel
{
	BASIC_KERNEL_API void registerBundle(const QString& bundle);
	BASIC_KERNEL_API QColor getSettingColor(const QString& bundle, const QString& key, const QString& group);
	BASIC_KERNEL_API QVector4D getSettingVector4D(const QString& bundle, const QString& key, const QString& group);
	BASIC_KERNEL_API QVariantList getSettingVariantList(const QString& bundle, const QString& key, const QString& group);
	BASIC_KERNEL_API QString getSettingString(const QString& bundle, const QString& key, const QString& group);

	BASIC_KERNEL_API QString version();
	BASIC_KERNEL_API QString userCoursePath();
	BASIC_KERNEL_API QString userFeedbackWebsite();
	BASIC_KERNEL_API QString calibrationTutorialWebsite();
	BASIC_KERNEL_API QString officialWebsite();
	BASIC_KERNEL_API QString cloudTutorialWeb(const QString& name);

	BASIC_KERNEL_API QString languageName(MultiLanguage language);
	BASIC_KERNEL_API QString languageTsFile(MultiLanguage language);
	BASIC_KERNEL_API MultiLanguage tsFile2Language(const QString& tsFile);

	BASIC_KERNEL_API QString themeName(ThemeCategory theme);

	BASIC_KERNEL_API QString writableLocation(const QString& subDir, const QString& subSubDir = QString());

	BASIC_KERNEL_API QString getResourcePath(creative_kernel::ResourcesType resource);
	BASIC_KERNEL_API QString getEnginePathPrefix();
	BASIC_KERNEL_API EngineType getEngineType();
	BASIC_KERNEL_API QString getEngineVersion();
}

#define CONFIG_GET_COLOR(x, y, z) creative_kernel::getSettingColor(x, y, z)
#define CONFIG_GET_VEC4(x, y, z) creative_kernel::getSettingVector4D(x, y, z)
#define CONFIG_GET_VARLIST(x, y, z) creative_kernel::getSettingVariantList(x, y, z)
#define CONFIG_GET_STRING(x, y, z) creative_kernel::getSettingString(x, y, z)

#define CONFIG_GLOBAL_COLOR(x, y) CONFIG_GET_COLOR(global_render_bundle, x, y)
#define CONFIG_GLOBAL_VEC4(x, y) CONFIG_GET_VEC4(global_render_bundle, x, y)
#define CONFIG_GLOBAL_VARLIST(x, y) CONFIG_GET_VARLIST(global_render_bundle, x, y)

#define CONFIG_PLUGIN_COLOR(x, y) CONFIG_GET_COLOR(plugin_render_bundle, x, y)
#define CONFIG_PLUGIN_VEC4(x, y) CONFIG_GET_VEC4(plugin_render_bundle, x, y)
#define CONFIG_PLUGIN_VARLIST(x, y) CONFIG_GET_VARLIST(plugin_render_bundle, x, y)

#define CONFIG_GLOBAL_L_COLOR(x, y) CONFIG_GET_COLOR(global_render_light_bundle, x, y)
#define CONFIG_GLOBAL_L_VEC4(x, y) CONFIG_GET_VEC4(global_render_light_bundle, x, y)
#define CONFIG_GLOBAL_L_VARLIST(x, y) CONFIG_GET_VARLIST(global_render_light_bundle, x, y)

#define CONFIG_PLUGIN_L_COLOR(x, y) CONFIG_GET_COLOR(plugin_render_light_bundle, x, y)
#define CONFIG_PLUGIN_L_VEC4(x, y) CONFIG_GET_VEC4(plugin_render_light_bundle, x, y)
#define CONFIG_PLUGIN_L_VARLIST(x, y) CONFIG_GET_VARLIST(plugin_render_light_bundle, x, y)

#define CONFIG_DEFAULT_STRING(x, y) CONFIG_GET_STRING(default_config_bundle, x, y)





//resources macro
#define DEFAULT_CONFIG_ROOT creative_kernel::getResourcePath(creative_kernel::ResourcesType::rt_default_config_root)
#define LOG_PATH creative_kernel::getResourcePath(creative_kernel::ResourcesType::rt_log)
#define DUMP_PATH creative_kernel::getResourcePath(creative_kernel::ResourcesType::rt_dump)
#define ANALYTICS_PATH creative_kernel::getResourcePath(creative_kernel::ResourcesType::rt_analytics)
#define SLICE_PATH creative_kernel::getResourcePath(creative_kernel::ResourcesType::rt_slice)
#define TEMPGCODE_PATH creative_kernel::getResourcePath(creative_kernel::ResourcesType::rt_gcode)
#define MODEL_PATH creative_kernel::getResourcePath(creative_kernel::ResourcesType::rt_cloud_model_load)
#endif // CREATIVE_KERNEL_APP_SETTING_INTERFACE_1593676766939_H
