/*****************************************************************************
 *
 * This file is part of Mapnik (c++ mapping toolkit)
 *
 * Copyright (C) 2015 Artem Pavlenko
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *****************************************************************************/

#include "catch.hpp"

#include <mapnik/geom_util.hpp>
#include <mapnik/quad_tree.hpp>
#include <mapnik/util/spatial_index.hpp>

TEST_CASE("spatial_index")
{
    SECTION("mapnik::quad_tree<T>")
    {
        // value_type must have standard layout (http://en.cppreference.com/w/cpp/types/is_standard_layout)
        using value_type = std::int32_t;
        using mapnik::filter_in_box;
        mapnik::box2d<double> extent(0,0,100,100);
        mapnik::quad_tree<value_type> tree(extent);
        REQUIRE(tree.extent() == extent);
        // insert some items
        tree.insert({10,10,20,20}, 1);
        tree.insert({30,30,40,40}, 2);
        tree.insert({30,10,40,20}, 3);
        tree.insert({1,1,2,2}, 4);

        CHECK(tree.count_nodes() == 5);
        CHECK(tree.count_items() == 4);

        // serialise
        std::ostringstream out(std::ios::binary);
        tree.write(out);
        out.flush();

        CHECK(out.str().length() == 252);
        CHECK(out.str().at(0) == 'm');

        // read bounding box
        std::istringstream in(out.str(), std::ios::binary);
        auto box = mapnik::util::spatial_index<value_type>::bounding_box(in);
        CHECK(tree.extent().contains(box));

        // bounding box query
        std::vector<value_type> results;
        filter_in_box filter(box);
        mapnik::util::spatial_index<value_type>::query(filter, in, results);

        CHECK(results.size() == 4);
        CHECKED_IF(results.size() >= 4)
        {
            CHECK(results[0] == 1);
            CHECK(results[1] == 4);
            CHECK(results[2] == 3);
            CHECK(results[3] == 2);
        }

        // query first N elements interface
        results.clear();
        in.seekg(0, std::ios::beg);
        mapnik::util::spatial_index<value_type>::query_first_n(filter, in, results, 2);
        CHECK(results.size() == 2);
        CHECKED_IF(results.size() >= 2)
        {
            CHECK(results[0] == 1);
            CHECK(results[1] == 4);
        }
        results.clear();
        in.seekg(0, std::ios::beg);
        mapnik::util::spatial_index<value_type>::query_first_n(filter, in, results, 5);
        CHECK(results.size() == 4);
        CHECKED_IF(results.size() >= 4)
        {
            CHECK(results[0] == 1);
            CHECK(results[1] == 4);
            CHECK(results[2] == 3);
            CHECK(results[3] == 2);
        }
    }
}
