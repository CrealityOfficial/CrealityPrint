#include "us/usettingwrapper.h"
#include "us/usettings.h"
#include "interface/appsettinginterface.h"

namespace creative_kernel
{
	bool isCura()
	{
		return getEngineType() == EngineType::ET_CURA;
	}

	int get_machine_extruder_count(us::USettings* settings, us::USettings* user_settings)
	{
        int count = 1;
        return count;
	}

	QString get_machine_name(us::USettings* settings, us::USettings* user_settings)
	{
        QString name;
        return name;
	}

	bool get_machine_is_belt(us::USettings* settings, us::USettings* user_settings)
	{
        bool belt = false;
        return belt;
	}

	bool get_machine_center_is_zero(us::USettings* settings, us::USettings* user_settings)
	{
        bool is_zero = false;
        return is_zero;
	}

	float get_machine_width(us::USettings* settings, us::USettings* user_settings)
	{
        float width = 500.0f;
        if (isCura())
        {
            if (user_settings && user_settings->hasKey("machine_width"))
            {
                width = user_settings->vvalue("machine_width").toFloat();
            }
            else {
                width = settings->vvalue("machine_width").toFloat();
            }
        }
        else
        {
            std::vector<trimesh::vec2> size;
            if (user_settings && user_settings->hasKey("printable_area"))
            {
                size = user_settings->point2s("printable_area");
            }
            else {
                size = settings->point2s("printable_area");
            }

            if (size.size() == 4)
                width = size.at(2).x;
        }
        return width;
	}

    float get_machine_depth(us::USettings* settings, us::USettings* user_settings)
    {
        float depth = 500.0f;
        if (isCura())
        {
            if (user_settings && user_settings->hasKey("machine_depth"))
            {
                depth = user_settings->vvalue("machine_depth").toFloat();
            }
            else {
                depth = settings->vvalue("machine_depth").toFloat();
            }
        }
        else {
            std::vector<trimesh::vec2> size;
            if (user_settings && user_settings->hasKey("printable_area"))
            {
                size = user_settings->point2s("printable_area");
            }
            else {
                size = settings->point2s("printable_area");
            }

            if (size.size() == 4)
                depth = size.at(2).x;
        }
        return depth;
    }

    float get_machine_height(us::USettings* settings, us::USettings* user_settings)
    {
        float height = 500.0f;
        if (isCura())
        {
            if (user_settings && user_settings->hasKey("machine_height"))
            {
                height = user_settings->vvalue("machine_height").toFloat();
            }
            else {
                height = settings->vvalue("machine_height").toFloat();
            }
        }
        else {
            if (user_settings && user_settings->hasKey("printable_height"))
            {
                height = user_settings->vvalue("printable_height").toFloat();
            }
            else if(settings->hasKey("printable_height")){
                height = settings->vvalue("printable_height").toFloat();
            }
        }
        return height;
    }

    int get_nozzle(us::USettings* settings)
    {
        if (isCura())
        {
            return settings->vvalue("extruder_nr").toInt();
        }
        else {
            return settings->vvalue("extruder").toInt() - 1;
        }
    }

    void set_nozzle(us::USettings* settings, int nozzle)
    {
        if (isCura())
        {
            settings->add("extruder_nr", QString("%1").arg(nozzle), true);
        }
        else {
            settings->add("extruder", QString("%1").arg(nozzle + 1), true);
        }
    }

    float get_prime_volume(us::USettings* settings, us::USettings* user_settings)
    {
        float prime_volume = 45.0;
        if (!isCura())
        {
            if (user_settings && user_settings->hasKey("prime_volume"))
            {
                prime_volume = user_settings->vvalue("prime_volume").toFloat();
            }
            else {
                prime_volume = settings->vvalue("prime_volume").toFloat();
            }
        }
        return prime_volume;
    }

    float get_prime_tower_width(us::USettings* settings, us::USettings* user_settings)
    {
        float prime_tower_width = 35.0;
        if (!isCura())
        {
            if (user_settings && user_settings->hasKey("prime_tower_width"))
            {
                prime_tower_width = user_settings->vvalue("prime_tower_width").toFloat();
            }
            else {
                prime_tower_width = settings->vvalue("prime_tower_width").toFloat();
            }
        }
        return prime_tower_width;
    }

    float get_layer_height(us::USettings* settings, us::USettings* user_settings)
    {
        float layer_height = 3.0;
        if (user_settings && user_settings->hasKey("layer_height"))
        {
            layer_height = user_settings->vvalue("layer_height").toFloat();
        }
        else {
            layer_height = settings->vvalue("layer_height").toFloat();
        }
        return layer_height;
    }

    float get_initial_layer_print_height(us::USettings* settings, us::USettings* user_settings)
    {
        float initial_layer_print_height = 0.0;
        if (user_settings && user_settings->hasKey("initial_layer_print_height"))
        {
            initial_layer_print_height = user_settings->vvalue("initial_layer_print_height").toFloat();
        }
        else {
            initial_layer_print_height = settings->vvalue("initial_layer_print_height").toFloat();
        }
        return initial_layer_print_height;
    }   

    bool get_enable_prime_tower(us::USettings* settings, us::USettings* user_settings)
    {
        bool enable_prime_tower = false;
        if (user_settings && user_settings->hasKey("enable_prime_tower"))
        {
            enable_prime_tower = user_settings->vvalue("enable_prime_tower").toBool();
        }
        else {
            enable_prime_tower = settings->vvalue("enable_prime_tower").toBool();
        }
        return enable_prime_tower;
    }

    bool get_enable_support(us::USettings* settings, us::USettings* user_settings)
    {
        bool enable_support = false;
        if (user_settings && user_settings->hasKey("enable_support"))
        {
            enable_support = user_settings->vvalue("enable_support").toBool();
        }
        else {
            enable_support = settings->vvalue("enable_support").toBool();
        }
        return enable_support;
    }

    int get_support_filament(us::USettings* settings, us::USettings* user_settings)
    {
        int support_filament;
        if (user_settings && user_settings->hasKey("support_filament"))
        {
            support_filament = user_settings->vvalue("support_filament").toInt();
        }
        else {
            support_filament = settings->vvalue("support_filament").toInt();
        }
        return support_filament;
    }

    int get_support_interface_filament(us::USettings* settings, us::USettings* user_settings)
    {
        int support_interface_filament;
        if (user_settings && user_settings->hasKey("support_interface_filament"))
        {
            support_interface_filament = user_settings->vvalue("support_interface_filament").toInt();
        }
        else {
            support_interface_filament = settings->vvalue("support_interface_filament").toInt();
        }
        return support_interface_filament;;
    }

    float get_extruder_clearance_radius(us::USettings* settings, us::USettings* user_settings)
    {
        float extruder_clearance_radius = 40.0;
        if (!isCura())
        {
            if (user_settings && user_settings->hasKey("extruder_clearance_radius"))
            {
                extruder_clearance_radius = user_settings->vvalue("extruder_clearance_radius").toFloat();
            }
            else {
                extruder_clearance_radius = settings->vvalue("extruder_clearance_radius").toFloat();
            }
        }
        return extruder_clearance_radius;
    }

    float get_extruder_clearance_height_to_lid(us::USettings* settings, us::USettings* user_settings)
    {
        float extruder_clearance_height_to_lid = 35.0;
        if (!isCura())
        {
            if (user_settings && user_settings->hasKey("extruder_clearance_height_to_lid"))
            {
                extruder_clearance_height_to_lid = user_settings->vvalue("extruder_clearance_height_to_lid").toFloat();
            }
            else {
                extruder_clearance_height_to_lid = settings->vvalue("extruder_clearance_height_to_lid").toFloat();
            }
        }
        return extruder_clearance_height_to_lid;
    }

    float get_extruder_clearance_height_to_rod(us::USettings* settings, us::USettings* user_settings)
    {
        float extruder_clearance_height_to_rod = 35.0;
        if (!isCura())
        {
            if (user_settings && user_settings->hasKey("extruder_clearance_height_to_rod"))
            {
                extruder_clearance_height_to_rod = user_settings->vvalue("extruder_clearance_height_to_rod").toFloat();
            }
            else {
                extruder_clearance_height_to_rod = settings->vvalue("extruder_clearance_height_to_rod").toFloat();
            }
        }
        return extruder_clearance_height_to_rod;
    }

    QString get_software_version(us::USettings* settings)
    {
        return settings->vvalue("software_version").toString();
    }

    void set_software_version(us::USettings* settings, const QString& value)
    {
        settings->add("software_version", value, true);
    }
}
