/*******************************************************************************
* Copyright 2016 Intel Corporation
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

#ifndef BATCH_NORMALIZATION_FWD_PD_HPP
#define BATCH_NORMALIZATION_FWD_PD_HPP

#include "mkldnn.h"

#include "c_types_map.hpp"
#include "primitive_desc.hpp"
#include "memory_pd.hpp"

namespace mkldnn {
namespace impl {

struct batch_normalization_fwd_pd_t: public primitive_desc_t {
    typedef batch_normalization_fwd_pd_t base_class;
    typedef batch_normalization_fwd_pd_t hint_class;
    static constexpr auto base_pkind = primitive_kind::batch_normalization;

    batch_normalization_fwd_pd_t(mkldnn::impl::engine_t *engine,
            const batch_normalization_desc_t *adesc,
            const batch_normalization_fwd_pd_t *hint_fwd_pd)
        : primitive_desc_t(engine, primitive_kind::batch_normalization)
        , desc_(*adesc), hint_fwd_pd_(hint_fwd_pd) {}
    virtual ~batch_normalization_fwd_pd_t() {}

    const batch_normalization_desc_t *desc() const { return &desc_; }
    virtual const op_desc_t *op_desc() const override
    { return reinterpret_cast<const op_desc_t *>(this->desc()); }

    virtual const memory_pd_t *input_pd(int index = 0) const override {
        if (index == 0) return src_pd();
        if (stats_is_src()) {
            if (index == 1) return mean_pd();
            if (index == 2) return variance_pd();
        }
        return nullptr;
    }
    virtual const memory_pd_t *output_pd(int index = 0) const override {
        if (index == 0) return dst_pd();
        if (!stats_is_src() && is_training()) {
            if (index == 1) return mean_pd();
            if (index == 2) return variance_pd();
        }
        return nullptr;
    }

    virtual const memory_pd_t *mean_pd() const {
        return stats_is_src() ? src_pd(1) : dst_pd(1); }

    virtual const memory_pd_t *variance_pd() const {
        return stats_is_src() ? src_pd(2) : dst_pd(2); }

    inline bool stats_is_src() const {
        return desc_.flags & mkldnn_use_global_stats; }

    inline bool use_scaleshift() const {
        return desc_.flags & mkldnn_use_scaleshift; }

    inline bool is_training() const {
        return desc_.prop_kind == mkldnn_forward_training
                || desc_.prop_kind == mkldnn_forward;
    }

    virtual int n_inputs() const override {
        return 1 + 2*(stats_is_src()) + use_scaleshift(); }
    virtual int n_outputs() const override {
        return 1 + 2*(!stats_is_src()); }

    virtual status_t query(query_t what, int idx, void *result) const override
    {
        switch (what) {
        case query::batch_normalization_d:
            *(const batch_normalization_desc_t**)result = desc(); break;
        default: return primitive_desc_t::query(what, idx, result);
        }
        return status::success;
    }

    /* common batch_normalization aux functions */

    inline int MB() const { return desc_.data_desc.dims[0]; }
    inline int C() const { return desc_.data_desc.dims[1]; }
    inline int H() const { return desc_.data_desc.dims[2]; }
    inline int W() const { return desc_.data_desc.dims[3]; }

protected:
    batch_normalization_desc_t desc_;
    const batch_normalization_fwd_pd_t *hint_fwd_pd_;
};

struct batch_normalization_bwd_pd_t: public primitive_desc_t {
    typedef batch_normalization_bwd_pd_t base_class;
    typedef batch_normalization_fwd_pd_t hint_class;
    static constexpr auto base_pkind = primitive_kind::batch_normalization;

    batch_normalization_bwd_pd_t(mkldnn::impl::engine_t *engine,
            const batch_normalization_desc_t *adesc,
            const batch_normalization_fwd_pd_t *hint_fwd_pd)
        : primitive_desc_t(engine, primitive_kind::batch_normalization)
        , desc_(*adesc), hint_fwd_pd_(hint_fwd_pd) {}
    virtual ~batch_normalization_bwd_pd_t() {}

    const batch_normalization_desc_t *desc() const { return &desc_; }
    virtual const op_desc_t *op_desc() const override
    { return reinterpret_cast<const op_desc_t *>(this->desc()); }

    virtual const memory_pd_t *input_pd(int index = 0) const override {
        if (index == 0) return src_pd();
        if (index == 1) return src_pd(1);
        if (index == 2) return src_pd(2);
        if (index == 3) return diff_dst_pd();
        if (index == 4) return weights_pd();
        return nullptr;
    }
    virtual const memory_pd_t *output_pd(int index = 0) const override {
        if (index == 0) return diff_src_pd();
        if (index == 1) return diff_weights_pd();
        return nullptr;
    }

    virtual const memory_pd_t *mean_pd() const {
        return stats_is_src() ? src_pd(1) : dst_pd(1); }

    virtual const memory_pd_t *variance_pd() const {
        return stats_is_src() ? src_pd(2) : dst_pd(2); }

    inline bool stats_is_src() const {
        return desc_.flags && mkldnn_use_global_stats; }

    inline bool use_scaleshift() const {
        return desc_.flags & mkldnn_use_scaleshift; }

    inline bool omit_stats() const {
        return desc_.flags && mkldnn_omit_stats; }

    virtual int n_inputs() const override
    { return 4 + use_scaleshift(); }
    virtual int n_outputs() const override
    { return 1 + (diff_weights_pd()->desc()->format != mkldnn_format_undef); }

    virtual status_t query(query_t what, int idx, void *result) const override
    {
        switch (what) {
        case query::batch_normalization_d:
            *(const batch_normalization_desc_t**)result = desc(); break;
        default: return primitive_desc_t::query(what, idx, result);
        }
        return status::success;
    }

    /* common batch_normalization aux functions */

    inline int MB() const { return desc_.data_desc.dims[0]; }
    inline int C() const { return desc_.data_desc.dims[1]; }
    inline int H() const { return desc_.data_desc.dims[2]; }
    inline int W() const { return desc_.data_desc.dims[3]; }

protected:
    batch_normalization_desc_t desc_;
    const batch_normalization_fwd_pd_t *hint_fwd_pd_;
};

}
}

#endif

// vim: et ts=4 sw=4 cindent cino^=l0,\:0,N-s
