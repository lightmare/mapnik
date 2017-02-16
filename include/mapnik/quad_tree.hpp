/*****************************************************************************
 *
 * This file is part of Mapnik (c++ mapping toolkit)
 *
 * Copyright (C) 2017 Artem Pavlenko
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

#ifndef MAPNIK_QUAD_TREE_HPP
#define MAPNIK_QUAD_TREE_HPP

// mapnik
#include <mapnik/geometry/box2d.hpp>
#include <mapnik/util/noncopyable.hpp>

// stl
#include <vector>
#include <type_traits>
#include <utility>

#include <cstdint>
#include <cstring>

namespace mapnik
{
template <typename ValueType, typename BBoxType = box2d<double>>
class quad_tree : util::noncopyable
{
public:
    using value_type = ValueType;
    using bbox_type = BBoxType;
    using coord_type = typename bbox_type::value_type;

    explicit quad_tree(bbox_type const& ext,
                       unsigned int max_depth = 8,
                       double ratio = 0.55)
        : max_depth_(max_depth),
          ratio_(ratio),
          extent_(ext)
    {
        nodes_.reserve(4 * max_depth);
        nodes_.emplace_back(ext);
    }

    template <typename... Args>
    void emplace(bbox_type const& box, Args &&... args)
    {
        if (extent_.intersects(box))
        {
            auto & node = where(box, extent_, max_depth_, 0);
            node->emplace_back(std::forward<Args>(args)...);
        }
    }

    template <typename Fn>
    bool find_near(bbox_type const& box, Fn && pred)
    {
        if (extent_.intersects(box))
            return find_near<Fn>(box, pred, nodes_[0]);
        else
            return false;
    }

    template <typename Fn>
    void for_each(Fn && func)
    {
        for (auto & node : nodes_)
        {
            for (auto & item : node.items_)
            {
                std::forward<Fn>(func)(item);
            }
        }
    }

    void insert(bbox_type const& box)
    {
        emplace(box);
    }

    void insert(bbox_type const& box, value_type const& data)
    {
        emplace(box, data);
    }

    void insert(bbox_type const& box, value_type && data)
    {
        emplace(box, std::move(data));
    }

    void clear()
    {
        nodes_.clear();
        nodes_.emplace_back(extent_);
    }

    bbox_type const& extent() const
    {
        return extent_;
    }

    std::size_t count_items() const
    {
        std::size_t total = 0;
        for (auto & node : nodes_)
        {
            total += node->size();
        }
        return total;
    }

    std::size_t count_nodes() const
    {
        std::size_t total = 0;
        for (auto & node : nodes_)
        {
            total += !(node.count_children() == 1 && node->empty());
        }
        return total;
    }

    template <typename OutputStream>
    void write(OutputStream & out)
    {
        static_assert(std::is_standard_layout<value_type>::value,
                      "Values stored in quad-tree must be standard layout types to allow serialisation");
        char header[16];
        std::memset(header,0,16);
        std::strcpy(header,"mapnik-index");
        out.write(header,16);
        offsets_vec ofs(nodes_.size());
        calc_offsets(ofs, 0);
        write_node(out, ofs, 0);
    }

private:
    struct node_type;
    using items_vec = std::vector<value_type>;
    using nodes_vec = std::vector<node_type>;
    using offsets_vec = std::vector<std::uint32_t>;

    struct node_type
    {
        std::size_t children_[4] = {};
        bbox_type ext_;
        items_vec items_;

        explicit node_type(bbox_type const& ext)
            : ext_(ext) {}

        items_vec const* operator-> () const
        {
            return &items_;
        }

        items_vec * operator-> ()
        {
            return &items_;
        }

        std::size_t count_children() const
        {
            std::size_t total = 0;
            for (auto nj : children_)
            {
                total += nj ? 1 : 0;
            }
            return total;
        }
    };

    template <typename Fn>
    bool find_near(bbox_type const& box, Fn & pred, node_type & node)
    {
        for (auto & item : node.items_)
        {
            if (std::forward<Fn>(pred)(item))
                return true;
        }
        if (auto p = get_child(node, 0))
        {
            if (box.minx() <= p->ext_.maxx() &&
                box.miny() <= p->ext_.maxy() &&
                find_near<Fn>(box, pred, *p))
                return true;
        }
        if (auto p = get_child(node, 1))
        {
            if (box.maxx() >= p->ext_.minx() &&
                box.miny() <= p->ext_.maxy() &&
                find_near<Fn>(box, pred, *p))
                return true;
        }
        if (auto p = get_child(node, 2))
        {
            if (box.minx() <= p->ext_.maxx() &&
                box.maxy() >= p->ext_.miny() &&
                find_near<Fn>(box, pred, *p))
                return true;
        }
        if (auto p = get_child(node, 3))
        {
            if (box.maxx() >= p->ext_.minx() &&
                box.maxy() >= p->ext_.miny() &&
                find_near<Fn>(box, pred, *p))
                return true;
        }
        return false;
    }

    node_type * get_child(node_type & node, unsigned q)
    {
        std::size_t nj = node.children_[q];
        return nj ? &nodes_[nj] : nullptr;
    }

    // Find the node into which the given box belongs, constructing
    // nonexistent nodes along the way.
    node_type & where(bbox_type const& box, bbox_type const& ext,
                      unsigned height, std::size_t ni)
    {
        if (height-- > 0)
        {
            auto x0 = ext.minx();
            auto x3 = ext.maxx();
            auto x2 = x0 + static_cast<coord_type>((x3 - x0) * ratio_);
            auto x1 = x3 - static_cast<coord_type>((x3 - x0) * ratio_);
            auto y0 = ext.miny();
            auto y3 = ext.maxy();
            auto y2 = y0 + static_cast<coord_type>((y3 - y0) * ratio_);
            auto y1 = y3 - static_cast<coord_type>((y3 - y0) * ratio_);

            if (box.maxy() <= y2) // box is fully within bottom half
            {
                if (box.maxx() <= x2) // bottom-left quad
                {
                    return where(box, {x0, y0, x2, y2}, height, ni, 0);
                }
                if (x1 <= box.minx()) // bottom-right quad
                {
                    return where(box, {x1, y0, x3, y2}, height, ni, 1);
                }
            }
            if (y1 <= box.miny()) // box is fully within upper half
            {
                if (box.maxx() <= x2) // upper-left quad
                {
                    return where(box, {x0, y1, x2, y3}, height, ni, 2);
                }
                if (x1 <= box.minx()) // upper-right quad
                {
                    return where(box, {x1, y1, x3, y3}, height, ni, 3);
                }
            }
        }
        return nodes_[ni];
    }

    node_type & where(bbox_type const& box, bbox_type const& ext,
                      unsigned height, std::size_t ni, unsigned q)
    {
        auto nj = nodes_[ni].children_[q];
        if (!nj)
        {
            nj = nodes_.size();
            nodes_.emplace_back(ext); // might move all nodes
            nodes_[ni].children_[q] = nj;
        }
        return where(box, ext, height, nj);
    }

    // Calculate sibling node offsets, or in other words, serialized
    // sub-tree sizes. Nodes with a single child and no items of their own
    // are transparent, they're never written to file; their sub-tree size
    // is equal to their child's sub-tree size, but the value stored in
    // `ofs` will be zero. For all other nodes, the value in `ofs` will be
    // one plus the sub-tree size (adding one in order to distinguish leaf
    // nodes from transparent nodes).
    std::uint32_t calc_offsets(offsets_vec & ofs, std::size_t ni) const
    {
        std::uint32_t num_children = 0;
        std::uint32_t num_sub_items = 0;
        std::uint32_t offset = 0;
        auto & node = nodes_[ni];

        for (auto nj : node.children_)
        {
            if (nj)
            {
                num_children += 1;
                num_sub_items += nodes_[nj]->size();
                offset += calc_offsets(ofs, nj);
            }
        }

        if (num_children == 1 && node->empty())
        {
            ofs[ni] = 0; // transparent node
        }
        else
        {
            offset += num_children * sizeof(std::uint32_t); // offset
            offset += num_children * sizeof(bbox_type); // extent
            offset += num_children * sizeof(std::uint32_t); // num_shapes
            offset += num_sub_items * sizeof(value_type);
            offset += num_children * sizeof(std::uint32_t); // num_children
            ofs[ni] = 1 + offset;
        }
        return offset;
    }

    template <typename OutputStream>
    void write_node(OutputStream & out, offsets_vec const& ofs,
                    std::size_t ni) const
    {
        auto & node = nodes_[ni];
        auto num_children = static_cast<std::uint32_t>(node.count_children());
        auto num_shapes = static_cast<std::uint32_t>(node->size());

        if (auto offset = ofs[ni])
        {
            write_raw(out, &--offset);
            write_raw(out, &node.ext_);
            write_raw(out, &num_shapes);
            if (num_shapes > 0)
            {
                write_raw(out, node->data(), num_shapes);
            }
            write_raw(out, &num_children);
        }

        for (auto nj : node.children_)
        {
            if (nj)
            {
                write_node(out, ofs, nj);
            }
        }
    }

    template <typename OutputStream, typename Elem>
    static void write_raw(OutputStream & out, Elem const* data,
                          std::size_t count = 1)
    {
        out.write(reinterpret_cast<char const*>(data),
                  sizeof(Elem) * count);
    }

    const unsigned int max_depth_;
    const double ratio_;
    const bbox_type extent_;
    nodes_vec nodes_;
};

} // namespace mapnik

#endif // MAPNIK_QUAD_TREE_HPP
