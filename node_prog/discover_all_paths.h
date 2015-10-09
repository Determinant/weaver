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

#ifndef weaver_node_prog_discover_all_paths_h_
#define weaver_node_prog_discover_all_paths_h_

#include <string>

#include "common/property_predicate.h"
#include "db/remote_node.h"
#include "node_prog/node.h"
#include "node_prog/edge.h"
#include "node_prog/base_classes.h"
#include "node_prog/cache_response.h"

namespace node_prog
{
    using edge_set = std::unordered_set<cl::edge, cl::hash_edge, cl::equals_edge>;
    using anc_set = std::unordered_set<node_handle_t>;

    /* the identifier of paths (demonstrate how to create
     * additional data type for storing local information
     * for each node) */
    struct path_handle: public virtual Node_State_Base {
        std::vector<node_handle_t> nodes;
        std::vector<edge_handle_t> edges;

        void pop();
        uint64_t size() const;
        void pack(e::packer&) const;
        void unpack(e::unpacker&);
        bool operator==(const path_handle &b) const;
    };

    /* the parameters passed to a node */
    struct discover_all_paths_params: public virtual Node_Parameters_Base
    {
        node_handle_t dest;
        /* the remaining path length */
        uint32_t path_len;
        /* node & edge predicates */
        std::vector<predicate::prop_predicate> node_preds;
        std::vector<predicate::prop_predicate> edge_preds;
        /* for checking cycles */
        anc_set ancestors;
        /* for identifying different path through a node */
        path_handle path_id;
        uint32_t path_hash;
        uint32_t prev_path_hash;
        /* path results */
        std::unordered_map<node_handle_t, std::vector<cl::edge>> paths;
        /* whether the search is in returning phase */
        bool returning;
        /* the node that propagates to the current node */
        db::remote_node prev_node;

        discover_all_paths_params();
        ~discover_all_paths_params() { }
        uint64_t size() const;
        void pack(e::packer&) const;
        void unpack(e::unpacker&);

        // no caching
        bool search_cache() { return false; }
        cache_key_t cache_key() { return cache_key_t(); }
    };

    /* the substate describing a conceptual searching path */
    struct discover_all_paths_substate: public virtual Node_State_Base
    {   
        uint32_t outstanding_count;
        db::remote_node prev_node;
        /* result accumulator */
        std::unordered_map<node_handle_t, edge_set> paths;
        uint32_t path_len;
        uint32_t prev_path_hash;
        uint32_t path_hash;
        path_handle path_id;

        void get_prev_substate_identifier(discover_all_paths_params &params);
        discover_all_paths_substate();
        ~discover_all_paths_substate() { }
        uint64_t size() const;
        void pack(e::packer&) const;
        void unpack(e::unpacker&);
    };

    /* overall state of a node */
    struct discover_all_paths_state: public virtual Node_State_Base
    {
        /* maps remaining path len to a set of substates,
         * each of which represents a conceptual searching path */
        std::unordered_map<uint32_t, std::vector<discover_all_paths_substate>> vmap;

        discover_all_paths_state();
        ~discover_all_paths_state() { }
        uint64_t size() const;
        void pack(e::packer&) const;
        void unpack(e::unpacker&);
        discover_all_paths_substate *get_substate(const discover_all_paths_params &params, bool create);
    };

   std::pair<search_type, std::vector<std::pair<db::remote_node, discover_all_paths_params>>>
   discover_all_paths_node_program(node &n,   /* current node */
       db::remote_node &rn,                   /* current node for access remotely */
       discover_all_paths_params &params,     /* passed parameters */
       std::function<discover_all_paths_state&()> state_getter,   /* the function to get state */
       std::function<void(std::shared_ptr<Cache_Value_Base>,
                    std::shared_ptr<std::vector<db::remote_node>>, cache_key_t)>&,
                                            /* cache for each node */
       cache_response<Cache_Value_Base>*);
}

#endif
