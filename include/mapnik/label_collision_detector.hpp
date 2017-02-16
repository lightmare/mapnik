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
class label_collision_detector4 : util::noncopyable
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
    explicit label_collision_detector4(box2d<double> const& _extent)
        : tree_(_extent) {}

    bool has_placement(box2d<double> const& box)
    {
        auto overlaps = [&](label const& item)
            {
                return item.box.intersects(box);
            };
        return tree_.find_near(box, overlaps) == false;
    }

    bool has_placement(box2d<double> const& box, double margin)
    {
        box2d<double> const& margin_box = (margin > 0
                                               ? box2d<double>(box.minx() - margin, box.miny() - margin,
                                                               box.maxx() + margin, box.maxy() + margin)
                                               : box);
        return has_placement(margin_box);
    }

    bool has_placement(box2d<double> const& box, double margin, mapnik::value_unicode_string const& text, double repeat_distance)
    {
        // Don't bother with any of the repeat checking unless the repeat distance is greater than the margin
        if (repeat_distance <= margin) {
            return has_placement(box, margin);
        }

        box2d<double> repeat_box(box.minx() - repeat_distance, box.miny() - repeat_distance,
                                 box.maxx() + repeat_distance, box.maxy() + repeat_distance);

        box2d<double> const& margin_box = (margin > 0
                                               ? box2d<double>(box.minx() - margin, box.miny() - margin,
                                                               box.maxx() + margin, box.maxy() + margin)
                                               : box);

        auto overlaps = [&](label const& item)
            {
                if (item.box.intersects(margin_box))
                    return true;
                if (item.box.intersects(repeat_box) && item.text == text)
                    return true;
                return false;
            };
        return tree_.find_near(repeat_box, overlaps) == false;
    }

    void insert(box2d<double> const& box)
    {
        tree_.emplace(box, box);
    }

    void insert(box2d<double> const& box, mapnik::value_unicode_string const& text)
    {
        tree_.emplace(box, box, text);
    }

    void clear()
    {
        tree_.clear();
    }

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
