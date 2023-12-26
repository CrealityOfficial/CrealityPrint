#ifndef CREATIVE_KERNEL_ENUM_1595470868902_H
#define CREATIVE_KERNEL_ENUM_1595470868902_H

namespace creative_kernel
{
	enum class ThemeCategory
	{
		tc_dark,
		tc_light,
		tc_num
	};

	enum class MirrorOperation
	{
		mo_x,
		mo_y,
		mo_z,
		mo_reset
	};

	enum class ModelVisualMode
	{
		mvm_face = 1,
		mvm_line,
		mvm_line_and_face
	};

	enum EViewShow
	{
		eFrontViewShow = 0,
		eBackViewShow,
		eLeftViewShow,
		eRightViewShow,
		eTopViewShow,
		eBottomViewShow,
		ePerspectiveViewShow,
		eOrthographicViewShow
	};

	enum class MultiLanguage
	{
		eLanguage_EN_TS = 0,
		eLanguage_ZHCN_TS,
		eLanguage_ZHTW_TS,
		eLanguage_KO_TS,
		eLanguage_JP_TS,
		eLanguage_DE_TS,
		eLanguage_ES_TS,
		eLanguage_FR_TS,
		eLanguage_IT_TS,
		eLanguage_PT_TS,
		eLanguage_RU_TS,
		eLanguage_TR_TS,
		eLanguage_PL_TS,
		eLanguage_NL_TS,
		eLanguage_CZ_TS,
		eLanguage_HU_TS,
		eLanguage_SE_TS,
		eLanguage_UA_TS,
		eLanguage_NUM
	};

	enum class KernelPhaseType
	{
		kpt_prepare,
		kpt_preview,
		kpt_device
	};

	enum class ResourcesType
	{
		rt_default_config_root,
		rt_cloud_model_load,
		rt_log,
		rt_dump,
		rt_analytics,
		rt_slice,
		rt_gcode,
		rt_profile,
		rt_machine,
		rt_material,
		rt_extruder,
		re_num_max
	};
}
#endif // CREATIVE_KERNEL_HEADER_1595470868902_H