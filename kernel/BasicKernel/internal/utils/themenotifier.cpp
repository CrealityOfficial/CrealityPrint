#include "themenotifier.h"
#include "interface/reuseableinterface.h"
#include "interface/appsettinginterface.h"
#include "interface/renderinterface.h"

#include "internal/render/printerentity.h"

#include "data/kernelmacro.h"
#include "entity/worldindicatorentity.h"
#include "external/kernel/kernel.h"
#include "internal/multi_printer/printermanager.h"

namespace creative_kernel
{
	ThemeNotifier::ThemeNotifier(QObject* parent)
		: QObject(parent)
	{

	}

	ThemeNotifier::~ThemeNotifier()
	{

	}

	void ThemeNotifier::onThemeChanged(ThemeCategory theme)
	{
		PrinterColorConfig printerConfig;
		QVector4D clearColor;
		switch (theme)
		{
			case creative_kernel::ThemeCategory::tc_dark:
			{
				printerConfig.skirt = CONFIG_GLOBAL_VEC4(printer_skirt_color, model_group);
				printerConfig.bottom = CONFIG_GLOBAL_VEC4(printer_bottom_color, model_group);
				printerConfig.bottomLog = CONFIG_GLOBAL_VEC4(printer_bottomlog_color, model_group);
				printerConfig.gridLine = CONFIG_GLOBAL_VEC4(printer_gridline_color, model_group);
				printerConfig.gridX = CONFIG_GLOBAL_VEC4(printer_gridx_color, model_group);
				printerConfig.grideY = CONFIG_GLOBAL_VEC4(printer_gridy_color, model_group);
				printerConfig.box = CONFIG_GLOBAL_VEC4(printer_box_color, model_group);
				printerConfig.skirtInner = CONFIG_GLOBAL_VEC4(printer_skirt_inner_color, model_group);
				printerConfig.skirtOuter = CONFIG_GLOBAL_VEC4(printer_skirt_outer_color, model_group);
				printerConfig.skirtVerticalBottom = CONFIG_GLOBAL_VEC4(printer_skirt_vertical_bottom_color, model_group);

				clearColor = CONFIG_GLOBAL_VEC4(modeleffect_clearcolor, model_group);

				getKernel()->setSceneClearColor(QColor("#363638"));
			}
				break;
			case creative_kernel::ThemeCategory::tc_light:
			default:
			{
				printerConfig.skirt = CONFIG_GLOBAL_L_VEC4(printer_skirt_color, model_group);
				printerConfig.bottom = CONFIG_GLOBAL_L_VEC4(printer_bottom_color, model_group);
				printerConfig.bottomLog = CONFIG_GLOBAL_L_VEC4(printer_bottomlog_color, model_group);
				printerConfig.gridLine = CONFIG_GLOBAL_L_VEC4(printer_gridline_color, model_group);
				printerConfig.gridX = CONFIG_GLOBAL_L_VEC4(printer_gridx_color, model_group);
				printerConfig.grideY = CONFIG_GLOBAL_L_VEC4(printer_gridy_color, model_group);
				printerConfig.box = CONFIG_GLOBAL_L_VEC4(printer_box_color, model_group);
				printerConfig.skirtInner = CONFIG_GLOBAL_L_VEC4(printer_skirt_inner_color, model_group);
				printerConfig.skirtOuter = CONFIG_GLOBAL_L_VEC4(printer_skirt_outer_color, model_group);
				printerConfig.skirtVerticalBottom = CONFIG_GLOBAL_L_VEC4(printer_skirt_vertical_bottom_color, model_group);

				clearColor = CONFIG_GLOBAL_L_VEC4(modeleffect_clearcolor, model_group);

				getKernel()->setSceneClearColor(QColor("#F2F2F5"));
			}
				break;
		}

		//entity->updatePrinterColor(printerConfig);
		//entity->setTheme((int)theme);
		getPrinterMangager()->setTheme((int)theme);
		setModelClearColor(clearColor);

		qtuser_3d::WorldIndicatorEntity* indicator = getIndicatorEntity();
		indicator->setTheme((int)theme);

		renderOneFrame();
	}

	void ThemeNotifier::onLanguageChanged(MultiLanguage language)
	{
		qtuser_3d::WorldIndicatorEntity* indicator = getIndicatorEntity();

		std::string dark, light, selected;
		std::string prefix = "qrc:/shader_entity/images/indicator/";
		switch (language)
		{
		case creative_kernel::MultiLanguage::eLanguage_ZHCN_TS:
			dark = prefix + "scene_all_dir_dark_hans.png";
			light = prefix + "scene_all_dir_light_hans.png";
			break;

		case creative_kernel::MultiLanguage::eLanguage_ZHTW_TS:
			dark = prefix + "scene_all_dir_dark_hant.png";
			light = prefix + "scene_all_dir_light_hant.png";
			break;

		case creative_kernel::MultiLanguage::eLanguage_EN_TS:			
		default:
			dark = prefix + "scene_all_dir_dark_en.png";
			light = prefix + "scene_all_dir_light_en.png";
			break;
		}
		
		indicator->setupDarkTexture(QUrl(dark.c_str()));
		indicator->setupLightTexture(QUrl(light.c_str()));
	}
}