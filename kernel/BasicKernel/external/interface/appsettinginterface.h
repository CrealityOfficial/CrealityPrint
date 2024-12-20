#ifndef CREATIVE_KERNEL_APP_SETTING_INTERFACE_1593676766939_H
#define CREATIVE_KERNEL_APP_SETTING_INTERFACE_1593676766939_H
#include "basickernelexport.h"
#include "data/kernelenum.h"
#include "kernel/enginedatatype.h"
#include <QColor>
#include <QString>

//bundle
#define global_render_bundle "global_render_config"
#define global_render_light_bundle "global_render_light_config"
//key

#define model_group "model"
#define modeleffect_shadowcolor "modeleffect_shadowcolor"
#define modeleffect_clearcolor "modeleffect_clearcolor"

#define modeleffect_statecolors "modeleffect_statecolors"   // "unclicked|hover|clicked|preview"

#define printer_skirt_color "printer_skirt_color"
#define printer_bottom_color "printer_bottom_color"
#define printer_bottomlog_color "printer_bottomlog_color"
#define printer_gridline_color "printer_gridline_color"
#define printer_gridx_color "printer_gridx_color"
#define printer_gridy_color "printer_gridy_color"
#define printer_box_color "printer_box_color"
#define printer_skirt_inner_color "printer_skirt_inner_color"
#define printer_skirt_outer_color "printer_skirt_outer_color"
#define printer_skirt_vertical_bottom_color "printer_skirt_vertical_bottom_color"

//bundle
#define plugin_render_bundle "plugin_render_config"
#define plugin_render_light_bundle "plugin_render_light_config"
//key
#define letter_group "letter"
#define letter_text_color "letter_text_color"
#define pickbottom_group "pickbottom"
#define pickbottom_hover_color "pickbottom_hover_color"
#define pickbottom_nohover_color "pickbottom_nohover_color"
#define slice_group "slice"
#define gcodeeffect_speedcolors "gcodeeffect_speedcolors"
#define gcodeeffect_typecolors "gcodeeffect_typecolors"
#define gcodeeffect_nozzlecolors "gcodeeffect_nozzlecolors"


//bundle
#define default_config_bundle "default_config"
//key
#define experience_group "experience"
#define experience_slicer "experience_slicer"

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
	BASIC_KERNEL_API QString useTutorialWebsite();
	BASIC_KERNEL_API QString calibrationTutorialWebsite();
	BASIC_KERNEL_API QString officialWebsite();
	BASIC_KERNEL_API QString cloudTutorialWeb(const QString& name);

	BASIC_KERNEL_API QString languageName(MultiLanguage language);
	BASIC_KERNEL_API QString languageTsFile(MultiLanguage language);
	BASIC_KERNEL_API MultiLanguage tsFile2Language(const QString& tsFile);

	BASIC_KERNEL_API QString themeName(ThemeCategory theme);

	BASIC_KERNEL_API QString writableLocation(const QString& subDir, const QString& subSubDir = QString());
	BASIC_KERNEL_API QString multiColorGuide();
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
