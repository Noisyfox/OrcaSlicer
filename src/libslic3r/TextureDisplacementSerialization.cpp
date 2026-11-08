#include "TextureDisplacement.hpp"
#include "Point.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "nlohmann/json.hpp"

namespace Slic3r {

// ---------------------------------------------------------------------------------------------
// Project persistence. See the declarations in TextureDisplacement.hpp for why this is JSON rather
// than the cereal stream next to them.
// ---------------------------------------------------------------------------------------------
namespace {

nlohmann::json vec2_to_json(const Vec2f &v) { return nlohmann::json::array({ v.x(), v.y() }); }
nlohmann::json vec3_to_json(const Vec3f &v) { return nlohmann::json::array({ v.x(), v.y(), v.z() }); }

Vec2f vec2_from_json(const nlohmann::json &j, const Vec2f &fallback)
{
    if (!j.is_array() || j.size() != 2)
        return fallback;
    return Vec2f(j[0].get<float>(), j[1].get<float>());
}
Vec3f vec3_from_json(const nlohmann::json &j, const Vec3f &fallback)
{
    if (!j.is_array() || j.size() != 3)
        return fallback;
    return Vec3f(j[0].get<float>(), j[1].get<float>(), j[2].get<float>());
}

// Reads key `k` into `out` when it is present and of the expected type, leaving `out` alone otherwise.
// That "leave it alone" is the whole point: every member starts at its struct default, so a project
// written by an older build simply keeps the defaults for whatever it did not know about.
template<class T> void read(const nlohmann::json &j, const char *k, T &out)
{
    const auto it = j.find(k);
    if (it == j.end())
        return;
    try {
        out = it->get<T>();
    } catch (...) {
    }
}
void read_enum(const nlohmann::json &j, const char *k, int &out)
{
    const auto it = j.find(k);
    if (it != j.end() && it->is_number_integer())
        out = it->get<int>();
}

} // namespace

std::string texture_displacement_layers_to_json(const std::vector<TextureDisplacementLayer> &layers,
                                                const TextureDisplacementOptions            &options)
{
    nlohmann::json root;
    root["version"] = 1;

    nlohmann::json &opt = root["options"];
    opt["displace_border"]    = options.displace_border;
    opt["smooth_enabled"]     = options.smooth_enabled;
    opt["smooth_strength"]    = options.smooth_strength;
    opt["smooth_iterations"]  = options.smooth_iterations;
    opt["smooth_skip_border"] = options.smooth_skip_border;
    opt["pipeline_v2"]        = options.pipeline_v2;
    opt["v2_refine_mm"]       = options.v2_refine_mm;
    opt["v2_regularize"]      = options.v2_regularize;
    opt["v2_max_triangles_k"] = options.v2_max_triangles_k;
    opt["v2_relocate"]        = options.v2_relocate;
    opt["v2_flip_edges"]      = options.v2_flip_edges;
    opt["color_mix_enabled"]  = options.color_mix_enabled;
    opt["color_despeckle"]    = options.color_despeckle;
    opt["color_mix_count"]    = options.color_mix_count;
    opt["active_slot"]        = options.active_slot;
    opt["panel_mode"]         = options.panel_mode;

    nlohmann::json &arr = root["layers"];
    arr                 = nlohmann::json::array();
    for (const TextureDisplacementLayer &l : layers) {
        nlohmann::json j;
        j["slot"]        = l.slot;
        j["name"]        = l.name;
        j["path"]        = l.path;
        j["path_in_3mf"] = l.path_in_3mf;

        j["depth_mm"]     = l.depth_mm;
        j["tiling_scale"] = l.tiling_scale;
        j["rotation_deg"] = l.rotation_deg;
        j["offset"]       = vec2_to_json(l.offset);
        j["invert"]       = l.invert;
        j["midlevel"]     = l.midlevel;
        j["smoothing"]    = l.smoothing;

        j["edge_smoothing"]        = l.edge_smoothing;
        j["edge_smoothing_amount"] = l.edge_smoothing_amount;
        j["auto_connect_islands"]  = l.auto_connect_islands;

        j["tile_enabled"]      = l.tile_enabled;
        j["tile_method"]       = int(l.tile_method);
        j["projection_method"] = int(l.projection_method);
        j["blend_mode"]        = int(l.blend_mode);
        j["color_enabled"]     = l.color_enabled;

        j["lscm_seam_angle_deg"] = l.lscm_seam_angle_deg;
        j["island_padding_mm"]   = l.island_padding_mm;

        j["view_project_right"]      = vec3_to_json(l.view_project_right);
        j["view_project_up"]         = vec3_to_json(l.view_project_up);
        j["view_project_projective"] = l.view_project_projective;
        j["view_project_matrix"]     = l.view_project_matrix;

        // Flat pairs rather than nested arrays: shorter, and the reader can simply ignore a trailing
        // odd element instead of having to validate every sub-array.
        nlohmann::json &seams = j["lscm_seam_edges"];
        seams                 = nlohmann::json::array();
        for (const auto &[a, b] : l.lscm_seam_edges) {
            seams.push_back(a);
            seams.push_back(b);
        }

        nlohmann::json &islands = j["islands"];
        islands                 = nlohmann::json::array();
        for (const TextureIsland &island : l.islands)
            islands.push_back(nlohmann::json::array(
                { island.offset.x(), island.offset.y(), island.rotation_deg, island.scale }));

        j["island_groups"] = l.island_groups;

        nlohmann::json &overrides = j["lscm_uv_overrides"];
        overrides                 = nlohmann::json::array();
        for (const auto &[key, uv] : l.lscm_uv_overrides)
            overrides.push_back(nlohmann::json::array({ key, uv.x(), uv.y() }));

        arr.push_back(std::move(j));
    }
    return root.dump();
}

bool texture_displacement_layers_from_json(const std::string                     &text,
                                           std::vector<TextureDisplacementLayer> &layers,
                                           TextureDisplacementOptions            &options)
{
    nlohmann::json root;
    try {
        root = nlohmann::json::parse(text);
    } catch (...) {
        return false;
    }
    if (!root.is_object())
        return false;

    TextureDisplacementOptions out_options;
    if (const auto it = root.find("options"); it != root.end() && it->is_object()) {
        const nlohmann::json &opt = *it;
        read(opt, "displace_border", out_options.displace_border);
        read(opt, "smooth_enabled", out_options.smooth_enabled);
        read(opt, "smooth_strength", out_options.smooth_strength);
        read(opt, "smooth_iterations", out_options.smooth_iterations);
        read(opt, "smooth_skip_border", out_options.smooth_skip_border);
        read(opt, "pipeline_v2", out_options.pipeline_v2);
        read(opt, "v2_refine_mm", out_options.v2_refine_mm);
        read(opt, "v2_regularize", out_options.v2_regularize);
        read(opt, "v2_max_triangles_k", out_options.v2_max_triangles_k);
        read(opt, "v2_relocate", out_options.v2_relocate);
        read(opt, "v2_flip_edges", out_options.v2_flip_edges);
        read(opt, "color_mix_enabled", out_options.color_mix_enabled);
        read(opt, "color_despeckle", out_options.color_despeckle);
        read(opt, "color_mix_count", out_options.color_mix_count);
        read(opt, "active_slot", out_options.active_slot);
        read(opt, "panel_mode", out_options.panel_mode);
        out_options.active_slot = std::clamp(out_options.active_slot, 0, int(TEXTURE_DISPLACEMENT_MAX_LAYERS) - 1);
        out_options.panel_mode  = std::clamp(out_options.panel_mode, 0, 1);
    }

    std::vector<TextureDisplacementLayer> out_layers;
    // read() skips a value of the wrong type, but the vector, matrix, seam, island and UV-override arrays
    // below read their numbers with get<>(), which throws on one; that rejects the document, like text that
    // does not parse.
    try {
        if (const auto it = root.find("layers"); it != root.end() && it->is_array()) {
            for (const nlohmann::json &j : *it) {
                if (!j.is_object())
                    continue;
                TextureDisplacementLayer l;
                read(j, "slot", l.slot);
                // The panel and the paint masks are indexed by slot, so a layer without a valid one rejects the
                // document rather than reaching them.
                if (l.slot < 0 || l.slot >= int(TEXTURE_DISPLACEMENT_MAX_LAYERS))
                    return false;
                read(j, "name", l.name);
                read(j, "path", l.path);
                read(j, "path_in_3mf", l.path_in_3mf);

                read(j, "depth_mm", l.depth_mm);
                read(j, "tiling_scale", l.tiling_scale);
                read(j, "rotation_deg", l.rotation_deg);
                if (const auto o = j.find("offset"); o != j.end())
                    l.offset = vec2_from_json(*o, l.offset);
                read(j, "invert", l.invert);
                read(j, "midlevel", l.midlevel);
                read(j, "smoothing", l.smoothing);

                read(j, "edge_smoothing", l.edge_smoothing);
                read(j, "edge_smoothing_amount", l.edge_smoothing_amount);
                read(j, "auto_connect_islands", l.auto_connect_islands);

                read(j, "tile_enabled", l.tile_enabled);
                read(j, "color_enabled", l.color_enabled);
                int tile_method = int(l.tile_method), projection = int(l.projection_method), blend = int(l.blend_mode);
                read_enum(j, "tile_method", tile_method);
                read_enum(j, "projection_method", projection);
                read_enum(j, "blend_mode", blend);
                l.tile_method       = TextureTileMethod(tile_method);
                l.projection_method = TextureProjectionMethod(std::clamp(projection, 0, 4));
                l.blend_mode        = TextureBlendMode(blend);

                read(j, "lscm_seam_angle_deg", l.lscm_seam_angle_deg);
                read(j, "island_padding_mm", l.island_padding_mm);

                if (const auto v = j.find("view_project_right"); v != j.end())
                    l.view_project_right = vec3_from_json(*v, l.view_project_right);
                if (const auto v = j.find("view_project_up"); v != j.end())
                    l.view_project_up = vec3_from_json(*v, l.view_project_up);
                read(j, "view_project_projective", l.view_project_projective);
                if (const auto v = j.find("view_project_matrix"); v != j.end() && v->is_array() && v->size() == 12)
                    for (size_t i = 0; i < 12; ++i)
                        l.view_project_matrix[i] = (*v)[i].get<float>();

                if (const auto v = j.find("lscm_seam_edges"); v != j.end() && v->is_array())
                    for (size_t i = 0; i + 1 < v->size(); i += 2)
                        l.lscm_seam_edges.emplace_back((*v)[i].get<int>(), (*v)[i + 1].get<int>());

                if (const auto v = j.find("islands"); v != j.end() && v->is_array())
                    for (const nlohmann::json &e : *v) {
                        if (!e.is_array() || e.size() != 4)
                            continue;
                        TextureIsland island;
                        island.offset       = Vec2f(e[0].get<float>(), e[1].get<float>());
                        island.rotation_deg = e[2].get<float>();
                        island.scale        = e[3].get<float>();
                        l.islands.push_back(island);
                    }

                read(j, "island_groups", l.island_groups);

                if (const auto v = j.find("lscm_uv_overrides"); v != j.end() && v->is_array())
                    for (const nlohmann::json &e : *v) {
                        if (!e.is_array() || e.size() != 3)
                            continue;
                        l.lscm_uv_overrides.emplace_back(e[0].get<int>(), Vec2f(e[1].get<float>(), e[2].get<float>()));
                    }

                out_layers.push_back(std::move(l));
            }
        }
    } catch (const nlohmann::json::exception &) {
        return false;
    }

    layers  = std::move(out_layers);
    options = out_options;
    return true;
}

} // namespace Slic3r
