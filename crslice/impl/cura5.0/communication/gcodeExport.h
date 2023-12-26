// Copyright (c) 2022 Ultimaker B.V.
// CuraEngine is released under the terms of the AGPLv3 or higher.

#ifndef GCODEEXPORT_H
#define GCODEEXPORT_H

#include <deque> // for extrusionAmountAtPreviousRetractions
#include <sstream> // for stream.str()
#include <fstream>
#include <stdio.h>
#include <memory>

#include "utils/AABB3D.h" //To track the used build volume for the Griffin header.
#include "settings/Settings.h" //For MAX_EXTRUDERS.
#include "settings/wrapper.h"

#include "types/LayerIndex.h"

#include "types/header.h"

#include "utils/IntPoint.h"
#include "tools/NoCopy.h"
#include "utils/smoothspeedacc.h"

namespace cura52
{
    class RetractionConfig;
    struct WipeScriptConfig;
    class SliceContext;
    struct SliceResult;
    class TimeEstimateCalculator;

    struct ExtruderTrainAttributes
    {
        bool is_primed; //!< Whether this extruder has currently already been primed in this print

        bool is_used; //!< Whether this extruder train is actually used during the printing of all meshgroups
        char extruderCharacter;

        double filament_area; //!< in mm^2 for non-volumetric, cylindrical filament
        Velocity max_volumetric_spped;

        double totalFilament; //!< total filament used per extruder in mm^3
        Temperature currentTemperature;
        bool waited_for_temperature; //!< Whether the most recent temperature command has been a heat-and-wait command (M109) or not (M104).
        Temperature initial_temp; //!< Temperature this nozzle needs to be at the start of the print.

        double retraction_e_amount_current; //!< The current retracted amount (in mm or mm^3), or zero(i.e. false) if it is not currently retracted (positive values mean retracted amount, so negative impact on E values)
        double retraction_e_amount_at_e_start; //!< The ExtruderTrainAttributes::retraction_amount_current value at E0, i.e. the offset (in mm or mm^3) from E0 to the situation where the filament is at the tip of the nozzle.

        double prime_volume; //!< Amount of material (in mm^3) to be primed after an unretration (due to oozing and/or coasting)
        Velocity last_retraction_prime_speed; //!< The last prime speed (in mm/s) of the to-be-primed amount

        double last_e_value_after_wipe; //!< The current material amount extruded since last wipe

        unsigned fan_number; // nozzle print cooling fan number

        std::deque<double> extruded_volume_at_previous_n_retractions; // in mm^3

        ExtruderTrainAttributes()
            : is_primed(false)
            , is_used(false)
            , extruderCharacter(0)
            , filament_area(0)
            , totalFilament(0)
            , currentTemperature(0)
            , waited_for_temperature(false)
            , initial_temp(0)
            , retraction_e_amount_current(0.0)
            , retraction_e_amount_at_e_start(0.0)
            , prime_volume(0.0)
            , last_retraction_prime_speed(0.0)
            , fan_number(0)
            , last_e_value_after_wipe(0.0)
        {
        }
    };

    //The GCodeExport class writes the actual GCode. This is the only class that knows how GCode looks and feels.
    //Any customizations on GCodes flavors are done in this class.
    class GCodeExport
    {
    private:
        /*!
        * The gcode file to write to when using CuraEngine as command line tool.
        */
        std::ofstream output_file;
        std::ostream* output_stream;
        
        EGCodeFlavor flavor;
        std::string new_line;
        bool is_volumetric;

        //time
        std::shared_ptr<TimeEstimateCalculator> estimateCalculator;



        ExtruderTrainAttributes extruder_attr[MAX_EXTRUDERS];

        double current_e_value; //!< The last E value written to gcode (in mm or mm^3)
        double e_value_cleaned; //E celaned
        double material_diameter = { 1.75 };
        double material_density = { 1.24 };

        // flow-rate compensation
        double current_e_offset; //!< Offset to compensate for flow rate (mm or mm^3)
        double max_extrusion_offset; //!< 0 to turn it off, normally 4
        double extrusion_offset_factor; //!< default 1

        Point3 currentPosition; //!< The last build plate coordinates written to gcode (which might be different from actually written gcode coordinates when the extruder offset is encoded in the gcode)
        Velocity currentSpeed; //!< The current speed (F values / 60) in mm/s
        Acceleration current_print_acceleration; //!< The current acceleration (in mm/s^2) used for print moves (and also for travel moves if the gcode flavor doesn't have separate travel acceleration)
        Acceleration current_travel_acceleration; //!< The current acceleration (in mm/s^2) used for travel moves for those gcode flavors that have separate print and travel accelerations
        Velocity current_jerk; //!< The current jerk in the XY direction (in mm/s^3)

        AABB3D total_bounding_box; //!< The bounding box of all g-code.

        /*!
         * The z position to be used on the next xy move, if the head wasn't in the correct z position yet.
         *
         * \see GCodeExport::writeExtrusion(Point, double, double)
         *
         * \note After GCodeExport::writeExtrusion(Point, double, double) has been called currentPosition.z coincides with this value
         */
        coord_t current_layer_z;
        coord_t z_offset = 0;
        coord_t is_z_hopped; //!< The amount by which the print head is currently z hopped, or zero if it is not z hopped. (A z hop is used during travel moves to avoid collision with other layer parts)

        size_t current_extruder;
        double current_cds_fan_speed;
        double current_fan_speed;
        double current_chamber_fan_speed;
        unsigned fan_number; // current print cooling fan number

        std::vector<Duration> total_print_times; //!< The total estimated print time in seconds for each feature
        unsigned int layer_nr; //!< for sending travel data

        Temperature initial_bed_temp; //!< bed temperature at the beginning of the print.
        Temperature bed_temperature; //!< Current build plate temperature.
        size_t m_preFixLen;
    protected:
        /*!
         * Convert an E value to a value in mm (if it wasn't already in mm) for the current extruder.
         *
         * E values are either in mm or in mm^3
         * The current extruder is used to determine the filament area to make the conversion.
         *
         * \param e the value to convert
         * \return the value converted to mm
         */
        double eToMm(double e);

        /*!
         * Convert a volume value to an E value (which might be volumetric as well) for the current extruder.
         *
         * E values are either in mm or in mm^3
         * The current extruder is used to determine the filament area to make the conversion.
         *
         * \param mm3 the value to convert
         * \return the value converted to mm or mm3 depending on whether the E axis is volumetric
         */
        double mm3ToE(double mm3);

        /*!
         * Convert a distance value to an E value (which might be linear/distance based as well) for the current extruder.
         *
         * E values are either in mm or in mm^3
         * The current extruder is used to determine the filament area to make the conversion.
         *
         * \param mm the value to convert
         * \return the value converted to mm or mm3 depending on whether the E axis is volumetric
         */
        double mmToE(double mm);

        /*!
         * Convert an E value to a value in mm3 (if it wasn't already in mm3) for the provided extruder.
         *
         * E values are either in mm or in mm^3
         * The given extruder is used to determine the filament area to make the conversion.
         *
         * \param e the value to convert
         * \param extruder Extruder number
         * \return the value converted to mm3
         */
        double eToMm3(double e, size_t extruder);

    public:
        SliceContext* application = nullptr;

        GCodeExport();
        ~GCodeExport();

        /*!
        * Set the target to write gcode to: to a file.
        *
        * Used when CuraEngine is used as command line tool.
        *
        * \param filename The filename of the file to which to write the gcode.
        */
        bool setTargetFile(const char* filename);
        void setOutputStream(std::ostream* stream);

        /*!
         * Get the gcode file header (e.g. ";FLAVOR:UltiGCode\n")
         *
         * \param extruder_is_used For each extruder whether it is used in the print
         * \param print_time The total print time in seconds of the whole gcode (if known)
         * \param filament_used The total mm^3 filament used for each extruder or a vector of the wrong size of unknown
         * \param mat_ids The material GUIDs for each material.
         * \return The string representing the file header
         */
        std::string getFileHeader(const std::vector<bool>& extruder_is_used,
            const Duration* print_time = nullptr,
            const std::vector<double>& filament_used = std::vector<double>(),
            const std::vector<std::string>& mat_ids = std::vector<std::string>());

        //for cloud result
        SliceResult getFileHeaderC(const std::vector<bool>& extruder_is_used,
            const Duration* print_time = nullptr,
            const std::vector<double>& filament_used = std::vector<double>(),
            const std::vector<std::string>& mat_ids = std::vector<std::string>());

        void setLayerNr(unsigned int layer_nr);

        int getExtruderNum();
        bool getExtruderIsUsed(const int extruder_nr) const; //!< return whether the extruder has been used throughout printing all meshgroup up till now

        Point getGcodePos(const coord_t x, const coord_t y, const int extruder_train) const;

        void setFlavor(EGCodeFlavor flavor);
        EGCodeFlavor getFlavor() const;

        void setZ(int z);
        void setZOffset(int zf);

        void setFlowRateExtrusionSettings(double max_extrusion_offset, double extrusion_offset_factor);

        /*!
         * Add extra amount of material to be primed after an unretraction.
         *
         * \param extra_prime_distance Amount of material in mm.
         */
        void addExtraPrimeAmount(double extra_prime_volume);

        void setMaxVolumetricSpeed(float maxSpeed);

        Point3 getPosition() const;

        Point getPositionXY() const;

        int getPositionZ() const;

        int getExtruderNr() const;

        double getCurrentFanSpeed();
        double getCurrentCdsFanSpeed();

        void setFilamentDiameter(size_t extruder, const coord_t diameter);

        double getCurrentExtrudedVolume() const;

        /*!
         * Get the total extruded volume for a specific extruder in mm^3
         *
         * Retractions and unretractions don't contribute to this.
         *
         * \param extruder_nr The extruder number for which to get the total netto extruded volume
         * \return total filament printed in mm^3
         */
        double getTotalFilamentUsed(size_t extruder_nr);

        /*!
         * Get the total estimated print time in seconds for each feature
         *
         * \return total print time in seconds for each feature
         */
        std::vector<Duration> getTotalPrintTimePerFeature();
        /*!
         * Get the total print time in seconds for the complete print
         *
         * \return total print time in seconds for the complete print
         */
        double getSumTotalPrintTimes();
        void updateTotalPrintTime();
        void reWritePreFixStr(std::string preFix);
        void setPreFixLen(size_t len) { m_preFixLen = len; }
        void resetTotalPrintTimeAndFilament();

        void writeComment(const std::string& comment);
        void writeComment2(const std::string& comment);
        void writeTypeComment(const PrintFeatureType& type);

        /*!
         * Write an M82 (absolute) or M83 (relative)
         *
         * \param set_relative_extrusion_mode If true, write an M83, otherwise write an M82
         */
        void writeExtrusionMode(bool set_relative_extrusion_mode);

        /*!
         * Write a comment saying what (estimated) time has passed up to this point
         *
         * \param time The time passed up till this point
         */
        void writeTimeComment(const Duration time);

        /*!
        * Write a comment saying what (estimated) time has passed up to this point
        *
        * \param time The time passed up till this point
        */
        void writeTimePartsComment(std::ostringstream& prefix);

        /*!
       * Enable the offset of gcode in the z-direction.
       *
       * \
       */
        void writeZoffsetComment(const double zOffset);

        /*!
        * Pressure advance(Klipper) AKA Linear advance(Marlin).
        */
        void writePressureComment(const double length);

        /*!
         * Write a comment saying that we're starting a certain layer.
         */
        void writeLayerComment(const LayerIndex layer_nr);

        /*!
         * Write a comment saying that the print has a certain number of layers.
         */
        void writeLayerCountComment(const size_t layer_count);

        void writeLine(const char* line);

        /*!
         * Reset the current_e_value to prevent too high E values.
         *
         * The current extruded volume is added to the current extruder_attr.
         */
        void resetExtrusionValue();

        void writeDelay(const Duration& time_amount);

        /*!
         * Coordinates are build plate coordinates, which might be offsetted when extruder offsets are encoded in the gcode.
         *
         * \param p location to go to
         * \param speed movement speed
         */
        void writeTravel(const Point& p, const Velocity& speed);

        void writeArcSatrt(const Point& p);

        /*!
         * Coordinates are build plate coordinates, which might be offsetted when extruder offsets are encoded in the gcode.
         *
         * \param p location to go to
         * \param speed movement speed
         * \param feature the feature that's currently printing
         * \param update_extrusion_offset whether to update the extrusion offset to match the current flow rate
         */
        void writeExtrusion(const Point& p, const Velocity& speed, double extrusion_mm3_per_mm, PrintFeatureType feature, bool update_extrusion_offset = false);

        void writeExtrusionG2G3(const Point& pointend, const Point& center_offset, double arc_length, const Velocity& speed, double extrusion_mm3_per_mm, PrintFeatureType feature, bool update_extrusion_offset = false, bool is_ccw = true);

        /*!
         * Go to a X/Y location with the z-hopped Z value
         * Coordinates are build plate coordinates, which might be offsetted when extruder offsets are encoded in the gcode.
         *
         * \param p location to go to
         * \param speed movement speed
         */
        void writeTravel(const Point3& p, const Velocity& speed);

        /*!
         * Go to a X/Y location with the extrusion Z
         * Perform un-z-hop
         * Perform unretraction
         *
         * Coordinates are build plate coordinates, which might be offsetted when extruder offsets are encoded in the gcode.
         *
         * \param p location to go to
         * \param speed movement speed
         * \param feature the feature that's currently printing
         * \param update_extrusion_offset whether to update the extrusion offset to match the current flow rate
         */
        void writeExtrusion(const Point3& p, const Velocity& speed, double extrusion_mm3_per_mm, PrintFeatureType feature, bool update_extrusion_offset = false);

        void writeExtrusionG2G3(const Point3& p, const Point& center_offset, double arc_length, const Velocity& speed, double extrusion_mm3_per_mm, PrintFeatureType feature, bool update_extrusion_offset = false, bool is_ccw = true);

        double getDensity();
        double getDiameter();
        double getEvalue();
        void   setDensity(double density);
        void   setDiameter(double diameter);
        Acceleration get_current_travel_acceleration();
        Acceleration get_current_print_acceleration();

        float get_current_layer_z();
        std::shared_ptr<SmoothSpeedAcc> smoothSpeedAcc;
    private:
        /*!
         * Coordinates are build plate coordinates, which might be offsetted when extruder offsets are encoded in the gcode.
         *
         * \param x build plate x
         * \param y build plate y
         * \param z build plate z
         * \param speed movement speed
         */
        void writeTravel(const coord_t x, const coord_t y, const coord_t z, const Velocity& speed);

        /*!
         * Perform un-z-hop
         * Perform unretract
         * Write extrusion move
         * Coordinates are build plate coordinates, which might be offsetted when extruder offsets are encoded in the gcode.
         *
         * \param x build plate x
         * \param y build plate y
         * \param z build plate z
         * \param speed movement speed
         * \param extrusion_mm3_per_mm flow
         * \param feature the print feature that's currently printing
         * \param update_extrusion_offset whether to update the extrusion offset to match the current flow rate
         */
        void writeExtrusion(const coord_t x, const coord_t y, const coord_t z,
            const Velocity& speed, const double extrusion_mm3_per_mm,
            const PrintFeatureType& feature, const bool update_extrusion_offset = false);

        void writeExtrusionG2G3(const coord_t x, const coord_t y, const coord_t z,
            const coord_t i, const coord_t j, double arc_length,
            const Velocity& speed, const double extrusion_mm3_per_mm,
            const PrintFeatureType& feature,
            const bool update_extrusion_offset = false, const bool is_ccw = true);
        /*!
         * Write the F, X, Y, Z and E value (if they are not different from the last)
         *
         * convenience function called from writeExtrusion and writeTravel
         *
         * This function also applies the gcode offset by calling \ref GCodeExport::getGcodePos
         * This function updates the \ref GCodeExport::total_bounding_box
         * It estimates the time in \ref GCodeExport::estimateCalculator for the correct feature
         * It updates \ref GCodeExport::currentPosition, \ref GCodeExport::current_e_value and \ref GCodeExport::currentSpeed
         */
        void writeFXYZE(const Velocity& speed, const coord_t x, const coord_t y, const coord_t z, const double e, const PrintFeatureType& feature);

        void writeFXYZIJE(const Velocity& speed, const coord_t x, const coord_t y, const coord_t z, const coord_t i, const coord_t j, const double e, const PrintFeatureType& feature);
        /*!
         * The writeTravel and/or writeExtrusion when flavor == BFB
         * \param x build plate x
         * \param y build plate y
         * \param z build plate z
         * \param speed movement speed
         * \param extrusion_mm3_per_mm flow
         * \param feature print feature to track print time for
         */
        void writeMoveBFB(const int x, const int y, const int z, const Velocity& speed, double extrusion_mm3_per_mm, PrintFeatureType feature);
    public:
        /*!
         * Get ready for extrusion moves:
         * - unretract (G11 or G1 E.)
         * - prime blob (G1 E)
         *
         * It estimates the time in \ref GCodeExport::estimateCalculator
         * It updates \ref GCodeExport::current_e_value and \ref GCodeExport::currentSpeed
         */
        void writeUnretractionAndPrime();
        void writeRetraction(const RetractionConfig& config, bool force = false, bool extruder_switch = false);
        void writeExtrusionG1(const Velocity& speed, Point point, const double e, const PrintFeatureType& feature);
        coord_t writeCircle(const Velocity& speed, Point endPoint, coord_t z_hop_height = 0);
        coord_t writeTrapezoidalLeft(const Velocity& speed, Point endPoint, coord_t z_hop_height);
        /*!
         * Start a z hop with the given \p hop_height.
         *
         * \param hop_height The height to move above the current layer.
         * \param speed The speed used for moving.
         */
        void writeZhopStart(const coord_t hop_height, Velocity speed = 0);

        /*!
         * End a z hop: go back to the layer height
         *
         * \param speed The speed used for moving.
         */
        void writeZhopEnd(Velocity speed = 0);

        /*!
         * Start the new_extruder:
         * - set new extruder
         * - zero E value
         * - write extruder start gcode
         *
         * \param new_extruder The extruder to start with
         */
        void startExtruder(const size_t new_extruder);

        /*!
         * Switch to the new_extruder:
         * - perform neccessary retractions
         * - fiddle with E-values
         * - write extruder end gcode
         * - set new extruder
         * - write extruder start gcode
         *
         * \param new_extruder The extruder to switch to
         * \param retraction_config_old_extruder The extruder switch retraction config of the old extruder, to perform the extruder switch retraction with.
         * \param perform_z_hop The amount by which the print head should be z hopped during extruder switch, or zero if it should not z hop.
         */
        void switchExtruder(size_t new_extruder, const RetractionConfig& retraction_config_old_extruder, coord_t perform_z_hop = 0);

        void writeCode(const char* str);

        bool substitution(std::string& str, const size_t start_extruder_nr);

        void resetExtruderToPrimed(const size_t extruder, const double initial_retraction);

        /*!
         * Write the gcode for priming the current extruder train so that it can be used.
         *
         * \param travel_speed The travel speed when priming involves a movement
         */
        void writePrimeTrain(const Velocity& travel_speed);

        /*!
         * Set the print cooling fan number (used as P parameter to M10[67]) for the specified extruder
         *
         * \param extruder The current extruder
         */
        void setExtruderFanNumber(int extruder);

        void writeFanCommand(double speed, double cds_speed = 0.0);
        void writeCdsFanCommand(double cds_speed);
        void writeChamberFanCommand(double chamber_speed = 0.0);

        void writeTemperatureCommand(const size_t extruder, const Temperature& temperature, const bool wait = false);
        void writeTopHeaterCommand(const size_t extruder, const Temperature& temperature, const bool wait = false);
        void writeBedTemperatureCommand(const Temperature& temperature, const bool wait = false);
        void writeBuildVolumeTemperatureCommand(const Temperature& temperature, const bool wait = false);

        /*!
         * Write the command for setting the acceleration for print moves to a specific value
         */
        void writePrintAcceleration(const Acceleration& acceleration, bool acceleration_breaking_enable, float acceleration_percent);

        /*!
         * Write the command for setting the acceleration for travel moves to a specific value
         */
        void writeTravelAcceleration(const Acceleration& acceleration, bool acceleration_breaking_enable, float acceleration_percent);

        /*!
         * Write the command for setting the jerk to a specific value
         */
        void writeJerk(const Velocity& jerk);

        /*!
         * Set member variables using the settings in \p settings.
         */
        void preSetup(const size_t start_extruder);

        /*!
         * Handle the initial (bed/nozzle) temperatures before any gcode is processed.
         * These temperatures are set in the pre-print setup in the firmware.
         *
         * See FffGcodeWriter::processStartingCode
         * \param start_extruder_nr The extruder with which to start this print
         */
        void setInitialAndBuildVolumeTemps(const unsigned int start_extruder_nr);

        /*!
         * Override or set an initial nozzle temperature as written by GCodeExport::setInitialTemps
         * This is used primarily during better specification of temperatures in LayerPlanBuffer::insertPreheatCommand
         *
         * \warning This function must be called before any of the layers in the meshgroup are written to file!
         * That's because it sets the current temperature in the gcode!
         *
         * \param extruder_nr The extruder number for which to better specify the temp
         * \param temp The temp at which the nozzle should be at startup
         */
        void setInitialTemp(int extruder_nr, double temp);

        /*!
         * Finish the gcode: turn fans off, write end gcode and flush all gcode left in the buffer.
         *
         * \param endCode The end gcode to be appended at the very end.
         */
        void finalize(const std::string& end_gcode);
        void finalize();

        /*!
         * Get amount of material extruded since last wipe script was inserted.
         *
         * \param extruder Extruder number to check.
         */
        double getExtrudedVolumeAfterLastWipe(size_t extruder);
        void initExtruderAttr(size_t extruder);

        /*!
         *  Reset the last_e_value_after_wipe.
         *
         * \param extruder Extruder number which last_e_value_after_wipe value to reset.
         */
        void ResetLastEValueAfterWipe(size_t extruder);

        /*!
         *  Generate g-code for wiping current nozzle using provided config.
         *
         * \param wipe_config Config with wipe script settings.
         */
        void insertWipeScript(const WipeScriptConfig& wipe_config);

        void writeGcodeHead();

        void writeMashineConfig();
        void writeProfileConfig();
        void writeShellConfig();
        void writeSupportConfig();
        void writeSpeedAndTravelConfig();
        void writeSpecialModelAndMeshConfig();
        void travelToSafePosition();
    };

}

#endif//GCODEEXPORT_H
