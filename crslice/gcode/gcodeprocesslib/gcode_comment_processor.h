////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Gcode Processor Library
//
// Tools for parsing gcode and calculating printer state from parsed gcode commands.
//
// Copyright(C) 2021 - Brad Hochgesang
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This program is free software : you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU Affero General Public License for more details.
//
//
// You can contact the author at the following email address: 
// FormerLurker@pm.me
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "position.h"
#define NUM_FEATURE_TYPES 11
static const std::string feature_type_name[NUM_FEATURE_TYPES] = {
		 "unknown_feature", "bridge_feature", "outer_perimeter_feature", "unknown_perimeter_feature", "inner_perimeter_feature", "skirt_feature", "gap_fill_feature", "solid_infill_feature", "ooze_shield_feature", "infill_feature", "prime_pillar_feature"
};
enum feature_type
{
	feature_type_unknown_feature, 
	feature_type_bridge_feature, 
	feature_type_outer_perimeter_feature, 
	feature_type_unknown_perimeter_feature,
	feature_type_inner_perimeter_feature, 
	feature_type_skirt_feature, 
	feature_type_gap_fill_feature, 
	feature_type_solid_infill_feature,
	feature_type_ooze_shield_feature, 
	feature_type_infill_feature, 
	feature_type_prime_pillar_feature
};
enum comment_process_type
{
	comment_process_type_off, 
	comment_process_type_unknown, 
	comment_process_type_slic3r_pe, 
	comment_process_type_cura, 
	comment_process_type_simplify_3d
};
// used for marking slicer sections for cura and simplify 3d
enum section_type
{
	section_type_no_section, 
	section_type_outer_perimeter_section, 
	section_type_inner_perimeter_section, 
	section_type_infill_section,
	section_type_gap_fill_section, 
	section_type_skirt_section, 
	section_type_solid_infill_section, 
	section_type_ooze_shield_section,
	section_type_prime_pillar_section
};

class gcode_comment_processor
{
	
public:
	
	gcode_comment_processor();
	~gcode_comment_processor();
	void update(position& pos);
	void update(std::string & comment);
	comment_process_type get_comment_process_type();

private:
	section_type current_section_;
	comment_process_type processing_type_;
	void update_feature_from_section(position& pos) const;
	bool update_feature_from_section_from_section(position& pos) const;
	bool update_feature_from_section_for_cura(position& pos) const;
	bool update_feature_from_section_for_simplify_3d(position& pos) const;
	bool update_feature_from_section_for_slice3r_pe(position& pos) const;
	void update_feature_for_unknown_slicer_comment(position& pos, std::string &comment);
	bool update_feature_for_slic3r_pe_comment(position& pos, std::string &comment) const;
	void update_unknown_section(std::string & comment);
	bool update_cura_section(std::string &comment);
	bool update_simplify_3d_section(std::string &comment);
	bool update_slic3r_pe_section(std::string &comment);
};

