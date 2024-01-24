#ifndef SRC_LIBSLIC3R_SUPPORTABLEISSUESSEARCH_HPP_
#define SRC_LIBSLIC3R_SUPPORTABLEISSUESSEARCH_HPP_

#include "Layer.hpp"
#include "Line.hpp"
#include "PrintBase.hpp"
#include "PrintConfig.hpp"
#include <boost/log/trivial.hpp>
#include <cstddef>
#include <vector>


namespace Slic3r {

namespace SupportSpotsGenerator {

struct Params
{
    Params(
        const std::vector<std::string> &filament_types, float max_acceleration, int raft_layers_count, BrimType brim_type, float brim_width)
        : max_acceleration(max_acceleration), raft_layers_count(raft_layers_count), brim_type(brim_type), brim_width(brim_width)
    {
        if (filament_types.size() > 1) {
            BOOST_LOG_TRIVIAL(warning)
                << "SupportSpotsGenerator does not currently handle different materials properly, only first will be used";
        }
        if (filament_types.empty() || filament_types[0].empty()) {
            BOOST_LOG_TRIVIAL(error) << "SupportSpotsGenerator error: empty filament_type";
            filament_type = std::string("PLA");
        } else {
            filament_type = filament_types[0];
            BOOST_LOG_TRIVIAL(debug) << "SupportSpotsGenerator: applying filament type: " << filament_type;
        }
    }

    // the algorithm should use the following units for all computations: distance [mm], mass [g], time [s], force [g*mm/s^2]
    const float bridge_distance = 16.0f; // mm
    const float max_acceleration; // mm/s^2 ; max acceleration of object in XY -- should be applicable only to printers with bed slinger, 
                                  // however we do not have such info yet. The force is usually small anyway, so not such a big deal to include it everytime
    const int raft_layers_count;
    std::string filament_type;

    BrimType brim_type;
    const float brim_width;

    const std::pair<float,float> malformation_distance_factors = std::pair<float, float> { 0.2, 1.1 };
    const float max_curled_height_factor = 10.0f;
    const float curled_distance_expansion = 1.0f; // controls the spread of the area where slow down for curled overhangs is applied
    const float curling_tolerance_limit = 0.1f;

    const float min_distance_between_support_points = 3.0f; //mm
    const float support_points_interface_radius = 1.5f; // mm
    const float min_distance_to_allow_local_supports = 1.0f; //mm

    const float gravity_constant = 9806.65f; // mm/s^2; gravity acceleration on Earth's surface, algorithm assumes that printer is in upwards position.
    const double filament_density = 1.25e-3f; // g/mm^3  ; Common filaments are very lightweight, so precise number is not that important
    const double material_yield_strength = 33.0f * 1e6f; // (g*mm/s^2)/mm^2; 33 MPa is yield strength of ABS, which has the lowest yield strength from common materials.
    const float standard_extruder_conflict_force = 10.0f * gravity_constant; // force that can occasionally push the model due to various factors (filament leaks, small curling, ... );
    const float malformations_additive_conflict_extruder_force = 65.0f * gravity_constant; // for areas with possible high layered curled filaments

    // MPa * 1e^6 = (g*mm/s^2)/mm^2 = g/(mm*s^2); yield strength of the bed surface
    double get_bed_adhesion_yield_strength() const {
        if (raft_layers_count > 0) {
            return get_support_spots_adhesion_strength() * 2.0;
        }

        if (filament_type == "PLA") {
            return 0.02 * 1e6;
        } else if (filament_type == "PET" || filament_type == "PETG") {
            return 0.3 * 1e6;
        } else if (filament_type == "ABS" || filament_type == "ASA") {
            return 0.1 * 1e6; //TODO do measurements
        } else { //PLA default value - defensive approach, PLA has quite low adhesion
            return 0.02 * 1e6;
        }
    }

    double get_support_spots_adhesion_strength() const {
         return 0.016f * 1e6; 
    }
};

struct SupportPoint {
    SupportPoint(const Vec3f &position, float force, float spot_radius, const Vec3f &direction);
    Vec3f position;
    float force;
    float spot_radius;
    Vec3f direction;
};

using SupportPoints = std::vector<SupportPoint>;
struct Issues {
    SupportPoints support_points;
};

struct Malformations {
    std::vector<Lines> layers; //for each layer
};

std::tuple<Issues, Malformations> full_search(const PrintObject *po, const Params &params);

void estimate_malformations(LayerPtrs &layers, const Params &params);

}} // namespace Slic3r::SupportSpotsGenerator

#endif /* SRC_LIBSLIC3R_SUPPORTABLEISSUESSEARCH_HPP_ */
