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

#ifndef MAPNIK_MARKERS_PLACEMENT_HPP
#define MAPNIK_MARKERS_PLACEMENT_HPP

#include <mapnik/markers_placements/line.hpp>
#include <mapnik/markers_placements/point.hpp>
#include <mapnik/markers_placements/interior.hpp>
#include <mapnik/markers_placements/vertext_first.hpp>
#include <mapnik/markers_placements/vertext_last.hpp>
#include <mapnik/symbolizer_enumerations.hpp>

namespace mapnik
{

template <typename Locator, typename Detector>
class markers_placement_finder : util::noncopyable
{
public:
    markers_placement_finder(marker_placement_e placement_type,
                             Locator &locator,
                             Detector &detector,
                             markers_placement_params const& params)
        : placement_type_(placement_type)
    {
        dispatch(constructor(), locator, detector, params);
    }

    ~markers_placement_finder()
    {
        dispatch(destructor());
    }

    // Get next point where the marker should be placed. Returns true if a place is found, false if none is found.
    bool get_point(double &x, double &y, double &angle, bool ignore_placement)
    {
        return dispatch(point_getter(), x, y, angle, ignore_placement);
    }

    template <typename Visitor, typename... Args>
    typename Visitor::result_type dispatch(Visitor && visitor, Args && ...args)
    {
        switch (placement_type_)
        {
        default:
        case MARKER_POINT_PLACEMENT:
            return std::forward<Visitor>(visitor)(point_, std::forward<Args>(args)...);
        case MARKER_INTERIOR_PLACEMENT:
            return std::forward<Visitor>(visitor)(interior_, std::forward<Args>(args)...);
        case MARKER_LINE_PLACEMENT:
            return std::forward<Visitor>(visitor)(line_, std::forward<Args>(args)...);
        case MARKER_VERTEX_FIRST_PLACEMENT:
            return std::forward<Visitor>(visitor)(vertex_first_, std::forward<Args>(args)...);
        case MARKER_VERTEX_LAST_PLACEMENT:
            return std::forward<Visitor>(visitor)(vertex_last_, std::forward<Args>(args)...);
        }
    }

private:
    marker_placement_e const placement_type_;

    union
    {
        markers_point_placement<Locator, Detector> point_;
        markers_line_placement<Locator, Detector> line_;
        markers_interior_placement<Locator, Detector> interior_;
        markers_vertex_first_placement<Locator, Detector> vertex_first_;
        markers_vertex_last_placement<Locator, Detector> vertex_last_;
    };

    struct constructor
    {
        using result_type = void;

        template <typename T, typename... Args>
        void operator() (T & that, Args && ...args)
        {
            new(&that) T(std::forward<Args>(args)...);
        }
    };

    struct destructor
    {
        using result_type = void;

        template <typename T>
        void operator() (T & that)
        {
            that.~T();
        }
    };

    struct point_getter
    {
        using result_type = bool;

        template <typename T, typename... Args>
        bool operator() (T & that, Args && ...args)
        {
            return that.get_point(std::forward<Args>(args)...);
        }
    };
};

}

#endif // MAPNIK_MARKERS_PLACEMENT_HPP
