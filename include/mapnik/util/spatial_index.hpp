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
#include <type_traits>
#include <vector>

namespace mapnik { namespace util {


template <typename InputStream>
bool check_spatial_index(InputStream& in)
{
    char header[17]; // mapnik-index
    std::memset(header, 0, 17);
    in.seekg(0, std::ios::beg);
    in.read(header,16);
    return (std::strncmp(header, "mapnik-index",12) == 0);
}

template <typename Value, typename Filter, typename InputStream,
          typename BBox = mapnik::box2d<double>>
class spatial_index
{
    using bbox_type = BBox;
public:
    static void query(Filter const& filter, InputStream& in,std::vector<Value>& pos);
    static bbox_type bounding_box( InputStream& in );
    static void query_first_n(Filter const& filter, InputStream & in, std::vector<Value>& pos, std::size_t count);
private:
    spatial_index();
    ~spatial_index();
    spatial_index(spatial_index const&);
    spatial_index& operator=(spatial_index const&);
    template <typename T> static T read_raw(InputStream& in);
    static void query_node(Filter const& filter, InputStream& in, std::vector<Value> & results);
    static void query_first_n_impl(Filter const& filter, InputStream& in, std::vector<Value> & results, std::size_t count);
};

template <typename Value, typename Filter, typename InputStream, typename BBox>
BBox spatial_index<Value, Filter, InputStream, BBox>::bounding_box(InputStream& in)
{
    static_assert(std::is_standard_layout<Value>::value, "Values stored in quad-tree must be standard layout type");
    if (!check_spatial_index(in)) throw std::runtime_error("Invalid index file (regenerate with shapeindex)");
    in.seekg(4, std::ios::cur);
    return read_raw<BBox>(in);
}

template <typename Value, typename Filter, typename InputStream, typename BBox>
void spatial_index<Value, Filter, InputStream, BBox>::query(Filter const& filter, InputStream& in, std::vector<Value>& results)
{
    static_assert(std::is_standard_layout<Value>::value, "Values stored in quad-tree must be standard layout type");
    if (!check_spatial_index(in)) throw std::runtime_error("Invalid index file (regenerate with shapeindex)");
    query_node(filter, in, results);
}

template <typename Value, typename Filter, typename InputStream, typename BBox>
void spatial_index<Value, Filter, InputStream, BBox>::query_node(Filter const& filter, InputStream& in, std::vector<Value>& results)
{
    auto offset = read_raw<std::uint32_t>(in);
    auto node_ext = read_raw<BBox>(in);
    auto num_shapes = read_raw<std::uint32_t>(in);
    if (!filter.pass(node_ext))
    {
        in.seekg(offset + num_shapes * sizeof(Value) + 4, std::ios::cur);
        return;
    }

    for (std::uint32_t i = 0; i < num_shapes; ++i)
    {
        Value item;
        in.read(reinterpret_cast<char*>(&item), sizeof(Value));
        results.push_back(std::move(item));
    }

    auto children = read_raw<std::uint32_t>(in);
    for (std::uint32_t j = 0; j < children; ++j)
    {
        query_node(filter, in, results);
    }
}

template <typename Value, typename Filter, typename InputStream, typename BBox>
void spatial_index<Value, Filter, InputStream, BBox>::query_first_n(Filter const& filter, InputStream& in, std::vector<Value>& results, std::size_t count)
{
    static_assert(std::is_standard_layout<Value>::value, "Values stored in quad-tree must be standard layout type");
    if (!check_spatial_index(in)) throw std::runtime_error("Invalid index file (regenerate with shapeindex)");
    query_first_n_impl(filter, in, results, count);
}

template <typename Value, typename Filter, typename InputStream, typename BBox>
void spatial_index<Value, Filter, InputStream, BBox>::query_first_n_impl(Filter const& filter, InputStream& in, std::vector<Value>& results, std::size_t count)
{
    if (results.size() == count) return;
    auto offset = read_raw<std::uint32_t>(in);
    auto node_ext = read_raw<BBox>(in);
    auto num_shapes = read_raw<std::uint32_t>(in);
    if (!filter.pass(node_ext))
    {
        in.seekg(offset + num_shapes * sizeof(Value) + 4, std::ios::cur);
        return;
    }

    for (std::uint32_t i = 0; i < num_shapes; ++i)
    {
        Value item;
        in.read(reinterpret_cast<char*>(&item), sizeof(Value));
        if (results.size() < count) results.push_back(std::move(item));
    }
    auto children = read_raw<std::uint32_t>(in);
    for (std::uint32_t j = 0; j < children; ++j)
    {
        query_first_n_impl(filter, in, results, count);
    }
}

template <typename Value, typename Filter, typename InputStream, typename BBox>
template <typename T>
T spatial_index<Value, Filter, InputStream, BBox>::read_raw(InputStream& in)
{
    T result;
    in.read(reinterpret_cast<char*>(&result), sizeof(result));
    return result;
}

}} // mapnik/util

#endif // MAPNIK_UTIL_SPATIAL_INDEX_HPP
