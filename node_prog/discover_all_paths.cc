/*
 * ===============================================================
 *    Description:  Discover all paths between two vertices
 *    predicated on max path len, node properties, edge
 *    properties. (Note that the other `discover_paths` node
 *    program could ignore some paths while searching,
 *    although it offers better time complexity)
 *
 *         Author:  Ted Yin, ted.sybil@gmail.com
 *
 * Copyright (C) 2015, Cornell University, see the LICENSE file
 *                     for licensing agreement
 * ===============================================================
 */

#define weaver_debug_
#include "common/stl_serialization.h"
#include "node_prog/node_prog_type.h"
#include "node_prog/discover_all_paths.h"

using node_prog::search_type;
using node_prog::discover_all_paths_params;
using node_prog::discover_all_paths_state;
using node_prog::discover_all_paths_substate;
using node_prog::cache_response;
using node_prog::Cache_Value_Base;

// params
discover_all_paths_params :: discover_all_paths_params()
    : path_len(UINT32_MAX)
    , path_hash(0)
    , prev_path_hash(0)
    , returning(false)
{ }

uint64_t
discover_all_paths_params :: size() const
{
    return message::size(dest)
         + message::size(path_len)
         + message::size(node_preds)
         + message::size(edge_preds)
         + message::size(ancestors)
         + message::size(path_id)
         + message::size(path_hash)
         + message::size(prev_path_hash)
         + message::size(paths)
         + message::size(returning)
         + message::size(prev_node)
         ;
}

void
discover_all_paths_params :: pack(e::packer &packer) const
{
    message::pack_buffer(packer, dest);
    message::pack_buffer(packer, path_len);
    message::pack_buffer(packer, node_preds);
    message::pack_buffer(packer, edge_preds);
    message::pack_buffer(packer, ancestors);
    message::pack_buffer(packer, path_id);
    message::pack_buffer(packer, path_hash);
    message::pack_buffer(packer, prev_path_hash);
    message::pack_buffer(packer, paths);
    message::pack_buffer(packer, returning);
    message::pack_buffer(packer, prev_node);
}

void
discover_all_paths_params :: unpack(e::unpacker &unpacker)
{
    message::unpack_buffer(unpacker, dest);
    message::unpack_buffer(unpacker, path_len);
    message::unpack_buffer(unpacker, node_preds);
    message::unpack_buffer(unpacker, edge_preds);
    message::unpack_buffer(unpacker, ancestors);
    message::unpack_buffer(unpacker, path_id);
    message::unpack_buffer(unpacker, path_hash);
    message::unpack_buffer(unpacker, prev_path_hash);
    message::unpack_buffer(unpacker, paths);
    message::unpack_buffer(unpacker, returning);
    message::unpack_buffer(unpacker, prev_node);
}

// state
discover_all_paths_substate :: discover_all_paths_substate()
    : outstanding_count(0)
{ }

uint64_t
discover_all_paths_substate :: size() const
{
    return message::size(outstanding_count)
         + message::size(prev_node)
         + message::size(paths)
         + message::size(path_len)
         + message::size(prev_path_hash)
         + message::size(path_hash)
         + message::size(path_id)
         ;
}

void
discover_all_paths_substate :: pack(e::packer &packer) const
{
    message::pack_buffer(packer, outstanding_count);
    message::pack_buffer(packer, prev_node);
    message::pack_buffer(packer, paths);
    message::pack_buffer(packer, path_len);
    message::pack_buffer(packer, prev_path_hash);
    message::pack_buffer(packer, path_hash);
    message::pack_buffer(packer, path_id);
}

void
discover_all_paths_substate :: unpack(e::unpacker &unpacker)
{
    message::unpack_buffer(unpacker, outstanding_count);
    message::unpack_buffer(unpacker, prev_node);
    message::unpack_buffer(unpacker, paths);
    message::unpack_buffer(unpacker, path_len);
    message::unpack_buffer(unpacker, prev_path_hash);
    message::unpack_buffer(unpacker, path_hash);
    message::unpack_buffer(unpacker, path_id);
}

void
discover_all_paths_substate::get_prev_substate_identifier(discover_all_paths_params &params)
{
    params.path_hash = prev_path_hash;
    /* copy path handle */
    params.path_id = path_id;
    params.path_id.pop();
}

discover_all_paths_state :: discover_all_paths_state()
{ }

uint64_t
discover_all_paths_state :: size() const
{
    return message::size(vmap);
}

void
discover_all_paths_state :: pack(e::packer &packer) const
{
    message::pack_buffer(packer, vmap);
}

void
discover_all_paths_state :: unpack(e::unpacker &unpacker)
{
    message::unpack_buffer(unpacker, vmap);
}

static uint32_t incremental_bkdr_hash(uint32_t hv, const node_handle_t &node)
{
    static const uint32_t seed = 131;
    for (char ch: node)
        hv = hv * seed + ch;
    hv = hv * seed + '\0';
    return hv;
}

discover_all_paths_substate *discover_all_paths_state::\
                            get_substate(const discover_all_paths_params &params,
                                        bool create = false)
{
    auto iter = vmap.find(params.path_hash);
    if (create)
    {
        std::vector<discover_all_paths_substate> *substates;
        if (iter == vmap.end())
        {
            substates = &(vmap.emplace(params.path_hash, std::vector<discover_all_paths_substate>()).first->second);
        }
        else
            substates = &(iter->second);
        substates->emplace_back();
        discover_all_paths_substate *substate = &*substates->rbegin();
        return substate;
    }
    else
    {
        if (iter == vmap.end())
            return nullptr;
        for (auto &substate: iter->second) {
            if (substate.path_id == substate.path_id)
            return &substate;
        }
        return nullptr;
    }
}

static void
_state_paths_to_params_paths(const std::unordered_map<node_handle_t, node_prog::edge_set> &state_paths,
    std::unordered_map<node_handle_t, std::vector<cl::edge>> &params_paths)
{
    params_paths.clear();
    for (const auto &p: state_paths) {
        std::vector<cl::edge> &evec = params_paths[p.first];
        evec.reserve(p.second.size());
        for (const cl::edge &e: p.second) {
            evec.emplace_back(e);
        }
    }
}

void node_prog::path_handle::pop() {
    if (!nodes.empty())
    {
        nodes.pop_back();
        edges.pop_back();
    }
}

uint64_t node_prog::path_handle::size() const {
    return message::size(nodes) + message::size(edges);
}

void node_prog::path_handle::pack(e::packer &packer) const {
    message::pack_buffer(packer, nodes);
    message::pack_buffer(packer, edges);
}

void node_prog::path_handle::unpack(e::unpacker &unpacker) {
    message::unpack_buffer(unpacker, nodes);
    message::unpack_buffer(unpacker, edges);
}

bool node_prog::path_handle::operator==(const path_handle &b) const {
    if (nodes.size() != b.nodes.size())
        return false;
    for (size_t i = 0; i < nodes.size(); i++)
        if (nodes[i] != b.nodes[i] || edges[i] != b.edges[i])
            return false;
    return true;
}

std::pair<search_type, std::vector<std::pair<db::remote_node, discover_all_paths_params>>>
node_prog :: discover_all_paths_node_program(node_prog::node &n,
   db::remote_node &rn,
   discover_all_paths_params &params,
   std::function<discover_all_paths_state&()> state_getter,
   std::function<void(std::shared_ptr<Cache_Value_Base>, std::shared_ptr<std::vector<db::remote_node>>, cache_key_t)>&,
   cache_response<Cache_Value_Base>*)
{
    discover_all_paths_state &state = state_getter();
    /* node progs to trigger next */
    std::vector<std::pair<db::remote_node, discover_all_paths_params>> next;
    node_handle_t cur_handle = n.get_handle();

    if (!params.returning) {
        // request spreading out

        discover_all_paths_substate *ret = state.get_substate(params);
        if (ret != nullptr) {
            /* node with the same substate already visited */
            assert(0 && "impossible");
        } else {
            // visit this node now
            discover_all_paths_substate &substate = *state.get_substate(params, true);
            substate.path_len = params.path_len;
            substate.path_hash = params.path_hash;
            substate.prev_path_hash = params.prev_path_hash;
            substate.path_id = params.path_id;
            substate.prev_node = params.prev_node;

            if (!n.has_all_predicates(params.node_preds)) {
                // node does not have all required properties, return immediately
                params.returning = true;
                substate.get_prev_substate_identifier(params);
                assert(params.paths.empty());
                next.emplace_back(std::make_pair(params.prev_node, params));
            } else {
                /* already reaches the target */
                if (params.dest == cur_handle || n.is_alias(params.dest)) {
                    params.returning = true;
                    substate.get_prev_substate_identifier(params);
                    substate.paths[cur_handle] = edge_set();
                    _state_paths_to_params_paths(substate.paths, params.paths);
                    next.emplace_back(std::make_pair(params.prev_node, params));
                } else if (params.path_len) {
                    uint32_t prev_path_len = params.path_len;
                    discover_all_paths_params params0 = params;
                    params.prev_node = rn;
                    params.ancestors.emplace(cur_handle);
                    params.path_id.nodes.emplace_back(cur_handle);
                    params.prev_path_hash = params.path_hash;
                    double path_hash = incremental_bkdr_hash(params.path_hash,
                                                    cur_handle);
                    bool has_child = false;

                    for (edge &e: n.get_edges()) {
                        const db::remote_node &nbr = e.get_neighbor();
                        if (params.ancestors.find(nbr.handle) == params.ancestors.end()
                            && e.has_all_predicates(params.edge_preds)) {
                            params.path_len = prev_path_len - 1;
                            params.path_id.edges.emplace_back(e.get_handle());
                            params.path_hash = incremental_bkdr_hash(path_hash, e.get_handle());
                            next.emplace_back(std::make_pair(nbr, params));
                            params.path_id.edges.pop_back(); /* revert to original edges */
                            substate.outstanding_count++;
                            has_child = true;
                        }
                    }

                    if (!has_child) {
                        params0.returning = true;
                        substate.get_prev_substate_identifier(params0);
                        next.emplace_back(std::make_pair(params0.prev_node, params0));
                    }
                } else { /* run out of path length */
                    params.returning = true;
                    substate.get_prev_substate_identifier(params);
                    assert(params.paths.empty());
                    next.emplace_back(std::make_pair(params.prev_node, params));
                }
            }
        }

    } else {
        // request returning to start node

        discover_all_paths_substate *ret = state.get_substate(params);
        assert(ret != nullptr);
        discover_all_paths_substate &substate = *ret;
        std::unordered_map<node_handle_t, edge_set> new_paths;
        auto &spaths = substate.paths;
        auto &ppaths = params.paths;
        /* merge results from child node */
        if (!params.paths.empty()) {
            for (const auto &p: ppaths) {
                if (spaths.find(p.first) == spaths.end())
                    spaths.emplace(p.first, edge_set());
                edge_set &eset = spaths[p.first];
                for (const cl::edge &cl_e: p.second)
                    eset.emplace(cl_e);
            }
            /* add edges to children */
            edge_set &eset = spaths[cur_handle];
            for (edge &e: n.get_edges()) {
                node_handle_t nbr = e.get_neighbor().handle;
                if (e.has_all_predicates(params.edge_preds) && spaths.find(nbr) != spaths.end()) {
                    cl::edge cl_e;
                    e.get_client_edge(n.get_handle(), cl_e);
                    eset.emplace(cl_e);
                }
            }
        }

        if (--substate.outstanding_count == 0) {
            substate.get_prev_substate_identifier(params);
            _state_paths_to_params_paths(spaths, ppaths);
            next.emplace_back(std::make_pair(substate.prev_node, params));
        }
    }

    return std::make_pair(search_type::BREADTH_FIRST, next);
}
