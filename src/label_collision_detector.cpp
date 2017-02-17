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

#include <mapnik/label_collision_detector.hpp>

namespace mapnik {

label_collision_detector4::label_collision_detector4(box2d<double> const& _extent)
    : tree_(_extent)
{
}

void label_collision_detector4::clear()
{
    tree_.clear();
}

bool label_collision_detector4::has_placement(box2d<double> const& box,
                                              double margin)
{
    auto margin_box = margin > 0 ? box + margin : box;
    auto overlaps = [&](label const& item)
        {
            return item.box.intersects(margin_box);
        };
    return tree_.find_near(margin_box, overlaps) == false;
}

bool label_collision_detector4::has_placement(box2d<double> const& box,
                                              double margin,
                                              value_unicode_string const& text,
                                              double repeat_distance)
{
    // Don't bother with any of the repeat checking
    // unless the repeat distance is greater than the margin
    if (repeat_distance <= margin)
    {
        return has_placement(box, margin);
    }

    auto margin_box = margin > 0 ? box + margin : box;
    auto repeat_box = box + repeat_distance;
    auto overlaps = [&](label const& item)
        {
            // Note that while this may be called many times, it will
            // return true at most once. By testing the larger box first,
            // reaching the more likely result (no overlap) involves less
            // work on average (1 or 3 tests) than if we started with the
            // smaller box (2 or 3 tests).
            if (item.box.intersects(repeat_box))
            {
                if (item.box.intersects(margin_box) || item.text == text)
                    return true;
            }
            return false;
        };
    return tree_.find_near(repeat_box, overlaps) == false;
}

void label_collision_detector4::insert(box2d<double> const& box)
{
    tree_.emplace(box, box);
}

void label_collision_detector4::insert(box2d<double> const& box,
                                       value_unicode_string const& text)
{
    tree_.emplace(box, box, text);
}

} // namespace mapnik
