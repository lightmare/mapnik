/*****************************************************************************
 *
 * This file is part of Mapnik (c++ mapping toolkit)
 *
 * Copyright (C) 2016 Artem Pavlenko
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

#ifndef MAPNIK_UTIL_SPATIAL_INDEX_HPP
#define MAPNIK_UTIL_SPATIAL_INDEX_HPP

//mapnik
#include <mapnik/geometry/box2d.hpp>

// stl
#include <cstdint>
#include <cstring>
#include <istream>
#include <type_traits>
#include <vector>

namespace mapnik { namespace util {

inline bool check_spatial_index(std::istream& in)
{
    char header[17]; // mapnik-index
    std::memset(header, 0, 17);
    in.seekg(0, std::ios::beg);
    in.read(header,16);
    return (std::strncmp(header, "mapnik-index",12) == 0);
}

template <typename Value, typename BBox = mapnik::box2d<double>>
class spatial_index
{
public:
    static BBox bounding_box(std::istream& in);

    template <typename Filter>
    static void query(Filter const& filter, std::istream& in, std::vector<Value>& pos);

    template <typename Filter>
    static void query_first_n(Filter const& filter, std::istream& in,
                              std::vector<Value>& pos, std::size_t count);

private:
    spatial_index();
    ~spatial_index();
    spatial_index(spatial_index const&);
    spatial_index& operator=(spatial_index const&);

    template <typename Filter>
    static void query_first_n_impl(Filter const& filter, std::istream& in,
                                   std::vector<Value>& results, std::size_t count);

    template <typename T>
    static T read_raw(std::istream& in);
};

template <typename Value, typename BBox>
BBox spatial_index<Value, BBox>::bounding_box(std::istream& in)
{
    static_assert(std::is_standard_layout<Value>::value, "Values stored in quad-tree must be standard layout type");
    if (!check_spatial_index(in)) throw std::runtime_error("Invalid index file (regenerate with shapeindex)");
    in.seekg(4, std::ios::cur);
    return read_raw<BBox>(in);
}

template <typename Value, typename BBox>
template <typename Filter>
void spatial_index<Value, BBox>::query
    (Filter const& filter, std::istream& in, std::vector<Value>& results)
{
    static_assert(std::is_standard_layout<Value>::value, "Values stored in quad-tree must be standard layout type");
    if (!check_spatial_index(in)) throw std::runtime_error("Invalid index file (regenerate with shapeindex)");
    query_first_n_impl(filter, in, results, std::size_t(-1));
}

template <typename Value, typename BBox>
template <typename Filter>
void spatial_index<Value, BBox>::query_first_n
    (Filter const& filter, std::istream& in, std::vector<Value>& results, std::size_t count)
{
    static_assert(std::is_standard_layout<Value>::value, "Values stored in quad-tree must be standard layout type");
    if (!check_spatial_index(in)) throw std::runtime_error("Invalid index file (regenerate with shapeindex)");
    query_first_n_impl(filter, in, results, count);
}

template <typename Value, typename BBox>
template <typename Filter>
void spatial_index<Value, BBox>::query_first_n_impl
    (Filter const& filter, std::istream& in, std::vector<Value>& results, std::size_t count)
{
    std::size_t prev_results = results.size();
    if (prev_results >= count)
        return;

    auto offset = read_raw<std::uint32_t>(in);
    auto node_ext = read_raw<BBox>(in);
    std::size_t num_shapes = read_raw<std::uint32_t>(in);

    if (!filter.pass(node_ext))
    {
        in.seekg(offset + num_shapes * sizeof(Value) + 4, std::ios::cur);
        return;
    }

    if (num_shapes > count - prev_results)
    {
        num_shapes = count - prev_results;
    }
    if (num_shapes > 0)
    {
        results.resize(prev_results + num_shapes);
        in.read(reinterpret_cast<char*>(results.data() + prev_results),
                num_shapes * sizeof(Value));
        if (results.size() >= count)
            return;
    }

    auto children = read_raw<std::uint32_t>(in);
    for (std::uint32_t j = 0; j < children; ++j)
    {
        query_first_n_impl(filter, in, results, count);
    }
}

template <typename Value, typename BBox>
template <typename T>
T spatial_index<Value, BBox>::read_raw(std::istream& in)
{
    T result;
    in.read(reinterpret_cast<char*>(&result), sizeof(result));
    return result;
}

}} // mapnik/util

#endif // MAPNIK_UTIL_SPATIAL_INDEX_HPP
