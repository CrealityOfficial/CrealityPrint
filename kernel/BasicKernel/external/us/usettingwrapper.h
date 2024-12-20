#ifndef _US_USETTING_WRAPPER_1589354685441_H
#define _US_USETTING_WRAPPER_1589354685441_H
#include "basickernelexport.h"
#include <QtCore/QString>
#include "trimesh2/Vec.h"

namespace us
{
	class USettings;
}
namespace creative_kernel
{
	//settings must exist
	BASIC_KERNEL_API int get_machine_extruder_count(us::USettings* settings, us::USettings* user_settings = nullptr);
	BASIC_KERNEL_API QString get_machine_name(us::USettings* settings, us::USettings* user_settings = nullptr);
	BASIC_KERNEL_API bool get_machine_is_belt(us::USettings* settings, us::USettings* user_settings = nullptr);
	BASIC_KERNEL_API bool get_machine_center_is_zero(us::USettings* settings, us::USettings* user_settings = nullptr);

	BASIC_KERNEL_API float get_machine_width(us::USettings* settings, us::USettings* user_settings = nullptr);
	BASIC_KERNEL_API float get_machine_depth(us::USettings* settings, us::USettings* user_settings = nullptr);
	BASIC_KERNEL_API float get_machine_height(us::USettings* settings, us::USettings* user_settings = nullptr);
	
	BASIC_KERNEL_API int get_nozzle(us::USettings* settings);
	BASIC_KERNEL_API void set_nozzle(us::USettings* settings, int nozzle);

	BASIC_KERNEL_API float get_prime_volume(us::USettings* settings, us::USettings* user_settings = nullptr);
	BASIC_KERNEL_API float get_prime_tower_width(us::USettings* settings, us::USettings* user_settings = nullptr);
	BASIC_KERNEL_API float get_layer_height(us::USettings* settings, us::USettings* user_settings = nullptr);
	BASIC_KERNEL_API float get_initial_layer_print_height(us::USettings* settings, us::USettings* user_settings = nullptr);
	BASIC_KERNEL_API bool get_enable_prime_tower(us::USettings* settings, us::USettings* user_settings = nullptr);
	BASIC_KERNEL_API bool get_enable_support(us::USettings* settings, us::USettings* user_settings = nullptr);
	BASIC_KERNEL_API int get_support_filament(us::USettings* settings, us::USettings* user_settings = nullptr);
	BASIC_KERNEL_API int get_support_interface_filament(us::USettings* settings, us::USettings* user_settings = nullptr);
	BASIC_KERNEL_API float get_extruder_clearance_radius(us::USettings* settings, us::USettings* user_settings = nullptr);
	BASIC_KERNEL_API float get_extruder_clearance_height_to_lid(us::USettings* settings, us::USettings* user_settings = nullptr);
	BASIC_KERNEL_API float get_extruder_clearance_height_to_rod(us::USettings* settings, us::USettings* user_settings = nullptr);

	BASIC_KERNEL_API QString get_software_version(us::USettings* settings);
	BASIC_KERNEL_API void set_software_version(us::USettings* settings, const QString& value);
}
#endif // _US_USETTING_WRAPPER_1589354685441_H
