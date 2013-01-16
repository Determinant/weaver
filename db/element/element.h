/*
 * ===============================================================
 *    Description:  Graph element (edges and vertices)
 *
 *        Created:  Thursday 11 October 2012 11:15:20  EDT
 *
 *         Author:  Ayush Dubey, dubey@cs.cornell.edu
 *
 * Copyright (C) 2013, Cornell University, see the LICENSE file
 *                     for licensing agreement
 * ===============================================================
 */

#ifndef __ELEMENT__
#define __ELEMENT__

#include <limits.h>
#include <stdint.h>
#include <vector>
#include <algorithm>
#include <string.h>

#include "property.h"
#include "meta_element.h"

namespace db
{
namespace element
{
    class element
    {
        public:
            element(po6::net::location server, uint64_t time, void* mem_addr);
            
        protected:
            std::vector<property> properties;
            po6::net::location myloc;
            uint64_t t_u;
            uint64_t t_d;
            void* elem_addr;

        public:
            void add_property(property prop);
            void remove_property(property prop);
            //int get_prop_value (char* _key, char** _value);
            bool has_property(property prop);
            void update_t_d(uint64_t del_time);
            meta_element get_meta_element();
            uint64_t get_t_u();
            uint64_t get_t_d();
            
        public:
            //Testing functions
            int num_properties();
    };

    inline
    element :: element(po6::net::location server, uint64_t time, void* mem_addr)
        : properties(0)
        , myloc(server)
        , t_u(time)
        , t_d(UINT_MAX)
        , elem_addr(mem_addr)
    {
    }

    inline void
    element :: add_property(property prop)
    {
        properties.push_back(prop);
    }

    inline void
    element :: remove_property(property prop)
    {
        std::vector<property>::iterator iter = std::remove(properties.begin(),
                                                            properties.end(),
                                                            prop);
        properties.erase(iter, properties.end());
    }

    /*
    int
    element :: get_prop_value (char* _key, char** _value)
    {
        std::vector<property>::iterator iter;
        for (iter = properties.begin(); iter < properties.end(); iter++)
        {
            if (strcmp (iter->key, _key) == 0)
            {
                *_value = (char *) malloc(strlen (iter->value));
                strncpy (*_value, iter->value, strlen (iter->value));
                return 0;
            }
        }
        return -1;
    }
    */

    bool
    element :: has_property(property prop)
    {
        std::vector<property>::iterator iter;
        int i = 0;
        for (iter = properties.begin(); iter<properties.end(); iter++)
        {
            if (prop == *iter) 
            {
                return true;
            }
        }
        return false;
    }

    inline int
    element :: num_properties()
    {
        return properties.size();
    }

    inline void
    element :: update_t_d(uint64_t del_time)
    {
        t_d = del_time;
    }

    inline meta_element
    element :: get_meta_element()
    {
        return meta_element(myloc, t_u, t_d, elem_addr);
    }

    inline uint64_t
    element :: get_t_u()
    {
        return t_u;
    }

    inline uint64_t
    element :: get_t_d()
    {
        return t_d;
    }
}
}

#endif //__ELEMENT__