///|/ Copyright (c) Prusa Research 2018 - 2023 Tomáš Mészáros @tamasmeszaros, Vojtěch Bubník @bubnikv, Lukáš Matěna @lukasmatena, Enrico Turri @enricoturri1966
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#ifndef MODELARRANGE_HPP
#define MODELARRANGE_HPP

#include <libslic3r/Arrange.hpp>
#include "libslic3r/PrintConfig.hpp"
#include "libslic3r/Model.hpp"
#include <libslic3r/Arrange/Scene.hpp>

namespace Slic3r {
using ModelInstancePtrs = std::vector<ModelInstance*>;

using arrangement::ArrangePolygon;
using arrangement::ArrangePolygons;
using arrangement::ArrangeParams;
using arrangement::InfiniteBed;
using arrangement::CircleBed;

// Do something with ArrangePolygons in virtual beds
using VirtualBedFn = std::function<void(arrangement::ArrangePolygon&)>;

[[noreturn]] inline void throw_if_out_of_bed(arrangement::ArrangePolygon&) 
{
    throw Slic3r::RuntimeError("Objects could not fit on the bed");
}

ArrangePolygons get_arrange_polys(const Model &model, ModelInstancePtrs &instances);
ArrangePolygon  get_arrange_poly(const Model &model);
bool apply_arrange_polys(ArrangePolygons &polys, ModelInstancePtrs &instances, VirtualBedFn);

//void duplicate(Model &model, ArrangePolygons &copies, VirtualBedFn);
void duplicate_objects(Model &model, size_t copies_num);

template<class TBed>
bool arrange_objects(Model &              model,
                     const TBed &         bed,
                     const ArrangeParams &params,
                     VirtualBedFn         vfn = throw_if_out_of_bed)
{
    ModelInstancePtrs instances;
    auto&& input = get_arrange_polys(model, instances);
    arrangement::arrange(input, bed, params);
    
    return apply_arrange_polys(input, instances, vfn);
}
bool arrange_objects(Model &model,
                     const arr2::ArrangeBed &bed,
                     const ArrangeParams &settings);

void duplicate_objects(Model &              model,
                       size_t               copies_num,
                       const arr2::ArrangeBed &bed,
                       const ArrangeParams &settings);

void duplicate(Model &              model,
               size_t               copies_num,
               const arr2::ArrangeBed &bed,
               const ArrangeParams &settings);

template<class T> struct PtrWrapper
{
    T* ptr;

    explicit PtrWrapper(T* p) : ptr{ p } {}

    arrangement::ArrangePolygon get_arrange_polygon(const Slic3r::DynamicPrintConfig &config = Slic3r::DynamicPrintConfig()) const
    {
        arrangement::ArrangePolygon ap;
        ptr->get_arrange_polygon(&ap, config);
        return ap;
    }

    void apply_arrange_result(const Vec2d& t, double rot, int item_id)
    {
        ptr->apply_arrange_result(t, rot);
        ptr->arrange_order = item_id;
    }
};

template<class T>
arrangement::ArrangePolygon get_arrange_poly(T obj, const DynamicPrintConfig &config = DynamicPrintConfig());

template<>
arrangement::ArrangePolygon get_arrange_poly(ModelInstance* inst, const DynamicPrintConfig& config);

ArrangePolygon get_instance_arrange_poly(ModelInstance* instance, const DynamicPrintConfig& config);
} // namespace Slic3r

#endif // MODELARRANGE_HPP
