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

#ifndef MAPNIK_LABEL_COLLISION_DETECTOR_HPP
#define MAPNIK_LABEL_COLLISION_DETECTOR_HPP

// mapnik
#include <mapnik/quad_tree.hpp>
#include <mapnik/util/noncopyable.hpp>
#include <mapnik/value/types.hpp>

#pragma GCC diagnostic push
#include <mapnik/warning_ignore.hpp>
#include <unicode/unistr.h>
#pragma GCC diagnostic pop

namespace mapnik
{

//quad tree based label collision detector so labels dont appear within a given distance
class MAPNIK_DECL label_collision_detector4 : util::noncopyable
{
public:
    struct label
    {
        label(box2d<double> const& b) : box(b), text() {}
        label(box2d<double> const& b, mapnik::value_unicode_string const& t) : box(b), text(t) {}

        box2d<double> box;
        mapnik::value_unicode_string text;
    };

private:
    using tree_t = quad_tree< label >;
    tree_t tree_;

public:
    explicit label_collision_detector4(box2d<double> const& _extent);

    bool has_placement(box2d<double> const& box, double margin = 0);
    bool has_placement(box2d<double> const& box, double margin,
                       value_unicode_string const& text, double repeat_distance);

    void insert(box2d<double> const& box);
    void insert(box2d<double> const& box, value_unicode_string const& text);

    void clear();

    box2d<double> const& extent() const
    {
        return tree_.extent();
    }

    template <typename Fn>
    void for_each(Fn && func)
    {
        tree_.for_each(std::forward<Fn>(func));
    }
};

} // namespace mapnik

#endif // MAPNIK_LABEL_COLLISION_DETECTOR_HPP
