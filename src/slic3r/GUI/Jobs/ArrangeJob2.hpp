///|/ Copyright (c) Prusa Research 2023 Tomáš Mészáros @tamasmeszaros
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#ifndef ARRANGEJOB2_HPP
#define ARRANGEJOB2_HPP

#include <optional>

#include "PlaterJob.hpp"

#include "libslic3r/Arrange/Tasks/ArrangeTask.hpp"
#include "libslic3r/Arrange/Tasks/FillBedTask.hpp"
#include "libslic3r/Arrange/Items/ArrangeItem.hpp"
#include "libslic3r/Arrange/SceneBuilder.hpp"
#include "slic3r/GUI/GUI.hpp"

namespace Slic3r {

class Model;
class DynamicPrintConfig;
class ModelInstance;

class Print;
class SLAPrint;

namespace GUI {

class Plater;

enum class ArrangeSelectionMode { SelectionOnly, Full };

arr2::SceneBuilder build_scene(
    Plater &plater, ArrangeSelectionMode mode = ArrangeSelectionMode::Full);

struct ArrCtl : public arr2::ArrangeTaskBase::Ctl
{
    //Job::Ctl &parent_ctl;
    Job &owner;
    int total;
    const std::string &msg;

    ArrCtl(Job &owner, int cnt, const std::string &m)
        : owner{owner}, total{cnt}, msg{m}
    {}

    bool was_canceled() const override
    {
        return owner.was_canceled();
    }

    void update_status(int remaining) override
    {
        if (remaining > 0)
            owner.update_status((total - remaining) * 100 / total, from_u8(msg));
    }
};

template<class ArrangeTaskT>
class ArrangeJob_ : public PlaterJob
{
public:
    using ResultType =
        typename decltype(std::declval<ArrangeTaskT>().process_native(
            std::declval<arr2::ArrangeTaskCtl>()))::element_type;

    // All callbacks are called in the main thread.
    struct Callbacks {
        // Task is prepared but not no processing has been initiated
        std::function<void(ArrangeTaskT &)> on_prepared;

        // Task has been completed but the result is not yet written (inside finalize)
        std::function<void(ArrangeTaskT &)> on_processed;

        // Task result has been written
        std::function<void(ResultType &)> on_finished;
    };

private:
    arr2::Scene m_scene;
    std::unique_ptr<ArrangeTaskT> m_task;
    std::unique_ptr<ResultType>   m_result;
    Callbacks  m_cbs;
    std::string m_task_msg;

protected:
    void prepare() override
    {
        m_task = ArrangeTaskT::create(m_scene);
        m_result.reset();
        if (m_task && m_cbs.on_prepared)
            m_cbs.on_prepared(*m_task);
    }

public:
    void process() override
    {
        if (!m_task)
            return;

        auto count = m_task->item_count_to_process();

        if (count == 0) // Should be taken care of by plater, but doesn't hurt
            return;

        update_status(0, from_u8(m_task_msg));

        auto taskctl = ArrCtl{*this, count, m_task_msg};
        m_result = m_task->process_native(taskctl);

        update_status(100, from_u8(m_task_msg));
    }

    void finalize() override
    {
        if (was_canceled())
            return;

        if (!m_result)
            return;

        if (m_task && m_cbs.on_processed)
            m_cbs.on_processed(*m_task);

        m_result->apply_on(m_scene.model());

        if (m_task && m_cbs.on_finished)
            m_cbs.on_finished(*m_result);
        
        Job::finalize();
    }

    explicit ArrangeJob_(std::shared_ptr<ProgressIndicator> pri,
                         Plater *plater,
                         arr2::Scene &&scene,
                         std::string task_msg,
                         const Callbacks &cbs = {})
        : PlaterJob{std::move(pri), plater},
          m_scene{std::move(scene)}, m_cbs{cbs}, m_task_msg{std::move(task_msg)}
    {
        // Orca: Use this for single plate arrangement only
        only_on_partplate = true;
    }
};

class ArrangeJob2: public ArrangeJob_<arr2::ArrangeTask<arr2::ArrangeItem>>
{
    using Base = ArrangeJob_<arr2::ArrangeTask<arr2::ArrangeItem>>;
public:
    ArrangeJob2(std::shared_ptr<ProgressIndicator> pri, Plater *plater, arr2::Scene &&scene, const Callbacks &cbs = {});
};

class FillBedJob2: public ArrangeJob_<arr2::FillBedTask<arr2::ArrangeItem>>
{
    using Base =  ArrangeJob_<arr2::FillBedTask<arr2::ArrangeItem>>;
public:
    FillBedJob2(std::shared_ptr<ProgressIndicator> pri, Plater *plater, arr2::Scene &&scene, const Callbacks &cbs = {});
};

} // namespace GUI
} // namespace Slic3r

#endif // ARRANGEJOB2_HPP
