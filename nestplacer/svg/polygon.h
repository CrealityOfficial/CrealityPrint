//Copyright (c) 2018 Ultimaker B.V.
//CuraEngine is released under the terms of the AGPLv3 or higher.

#ifndef CX_POLYGON_H
#define CX_POLYGON_H

#include <vector>
#include <assert.h>
#include <float.h>
#include <fstream>

#include <algorithm>    // std::reverse, fill_n array
#include <limits> // int64_t.min
#include <list>

#include <initializer_list>

#include "svg/intpoint.h"

namespace svg
{
    class PartsView;
    class Polygons;
    class Polygon;
    class PolygonRef;

    class ListPolyIt;

    typedef std::list<Point> ListPolygon; //!< A polygon represented by a linked list instead of a vector
    typedef std::vector<ListPolygon> ListPolygons; //!< Polygons represented by a vector of linked lists instead of a vector of vectors

    const static int clipper_init = (0);
    #define NO_INDEX (std::numeric_limits<unsigned int>::max())

    class ConstPolygonPointer;

    /*!
     * Outer polygons should be counter-clockwise,
     * inner hole polygons should be clockwise.
     * (When negative X is to the left and negative Y is downward.)
     */
    class ConstPolygonRef
    {
        friend class Polygons;
        friend class Polygon;
        friend class PolygonRef;
        friend class ConstPolygonPointer;
    protected:
        Clipper3r::Path* path;
    public:
        ConstPolygonRef(const Clipper3r::Path& polygon)
        : path(const_cast<Clipper3r::Path*>(&polygon))
        {}

        virtual ~ConstPolygonRef()
        {
        }

        bool operator==(ConstPolygonRef& other) const =delete; // polygon comparison is expensive and probably not what you want when you use the equality operator

        ConstPolygonRef& operator=(const ConstPolygonRef& other) =delete; // Cannot assign to a const object

        /*!
         * Gets the number of vertices in this polygon.
         * \return The number of vertices in this polygon.
         */
        size_t size() const;

        /*!
         * Returns whether there are any vertices in this polygon.
         * \return ``true`` if the polygon has no vertices at all, or ``false`` if
         * it does have vertices.
         */
        bool empty() const;

        const Point& operator[] (unsigned int index) const
        {
            return (*path)[index];
        }

        const Clipper3r::Path& operator*() const
        {
            return *path;
        }

        Clipper3r::Path::const_iterator begin() const
        {
            return path->begin();
        }

        Clipper3r::Path::const_iterator end() const
        {
            return path->end();
        }

        Clipper3r::Path::const_reference back() const
        {
            return path->back();
        }

        const void* data() const
        {
            return path->data();
        }
    private:
    };


    class PolygonPointer;

    class PolygonRef : public ConstPolygonRef
    {
        friend class PolygonPointer;
    public:
        PolygonRef(Clipper3r::Path& polygon)
        : ConstPolygonRef(polygon)
        {}

        PolygonRef(const PolygonRef& other)
        : ConstPolygonRef(*other.path)
        {}

        virtual ~PolygonRef()
        {
        }

        void reserve(size_t min_size)
        {
            path->reserve(min_size);
        }

        PolygonRef& operator=(const ConstPolygonRef& other) =delete; // polygon assignment is expensive and probably not what you want when you use the assignment operator

        PolygonRef& operator=(ConstPolygonRef& other) =delete; // polygon assignment is expensive and probably not what you want when you use the assignment operator
    //     { path = other.path; return *this; }

        PolygonRef& operator=(PolygonRef&& other)
        {
            *path = std::move(*other.path);
            return *this;
        }

        Point& operator[] (unsigned int index)
        {
            return (*path)[index];
        }

        Clipper3r::Path::iterator begin()
        {
            return path->begin();
        }

        Clipper3r::Path::iterator end()
        {
            return path->end();
        }

        Clipper3r::Path::reference back()
        {
            return path->back();
        }

        void* data()
        {
            return path->data();
        }

        void add(const Point p)
        {
            path->push_back(p);
        }

        Clipper3r::Path& operator*()
        {
            return *path;
        }

        template <typename... Args>
        void emplace_back(Args&&... args)
        {
            path->emplace_back(args...);
        }

        void remove(unsigned int index)
        {
            path->erase(path->begin() + index);
        }

        void clear()
        {
            path->clear();
        }

        void reverse()
        {
            Clipper3r::ReversePath(*path);
        }

        void pop_back()
        { 
            path->pop_back();
        }
    };

    class ConstPolygonPointer
    {
    protected:
        const Clipper3r::Path* path;
    public:
        ConstPolygonPointer()
        : path(nullptr)
        {}
        ConstPolygonPointer(const ConstPolygonRef* ref)
        : path(ref->path)
        {}
        ConstPolygonPointer(const ConstPolygonRef& ref)
        : path(ref.path)
        {}

        ConstPolygonRef operator*() const
        {
            assert(path);
            return ConstPolygonRef(*path);
        }
        const Clipper3r::Path* operator->() const
        {
            assert(path);
            return path;
        }

        operator bool() const
        {
            return path;
        }

        bool operator==(const ConstPolygonPointer& rhs)
        {
            return path == rhs.path;
        }
    };

    class PolygonPointer
    {
    protected:
        Clipper3r::Path* path;
    public:
        PolygonPointer()
        : path(nullptr)
        {}
        PolygonPointer(PolygonRef* ref)
        : path(ref->path)
        {}

        PolygonPointer(PolygonRef& ref)
        : path(ref.path)
        {}

        PolygonRef operator*()
        {
            assert(path);
            return PolygonRef(*path);
        }
        Clipper3r::Path* operator->()
        {
            assert(path);
            return path;
        }

        operator bool() const
        {
            return path;
        }
    };

    class Polygon : public PolygonRef
    {
        Clipper3r::Path poly;
    public:
        Polygon()
        : PolygonRef(poly)
        {
        }

        Polygon(const ConstPolygonRef& other)
        : PolygonRef(poly)
        , poly(*other.path)
        {
        }

        Polygon(const Polygon& other)
        : PolygonRef(poly)
        , poly(*other.path)
        {
        }

        Polygon(Polygon&& moved)
        : PolygonRef(poly)
        , poly(std::move(moved.poly))
        {
        }

        virtual ~Polygon()
        {
        }

        Polygon& operator=(const ConstPolygonRef& other) = delete; // copying a single polygon is generally not what you want
    //     {
    //         path = other.path;
    //         poly = *other.path;
    //         return *this;
    //     }

        Polygon& operator=(Polygon&& other) //!< move assignment
        {
            poly = std::move(other.poly);
            return *this;
        }
    };

    class PolygonsPart;

    class Polygons
    {
        friend class Polygon;
        friend class PolygonRef;
        friend class ConstPolygonRef;
    public:
        Clipper3r::Paths paths;
    public:
        void save(std::fstream& out) const;
        void load(std::fstream& in);

        unsigned int size() const
        {
            return paths.size();
        }

		Clipper3r::Paths getPaths()
		{
			return paths;
		}

		void setPaths(Clipper3r::Paths _paths)
		{
			paths = _paths;
		}

        void CleanPaths()
        {
            Clipper3r::CleanPolygons(paths);
        }

        /*!
         * Convenience function to check if the polygon has no points.
         *
         * \return `true` if the polygon has no points, or `false` if it does.
         */
        bool empty() const;

        unsigned int pointCount() const; //!< Return the amount of points in all polygons

        PolygonRef operator[] (unsigned int index)
        {
            return paths[index];
        }
        ConstPolygonRef operator[] (unsigned int index) const
        {
            return paths[index];
        }
        Clipper3r::Paths::iterator begin()
        {
            return paths.begin();
        }
        Clipper3r::Paths::const_iterator begin() const
        {
            return paths.begin();
        }
        Clipper3r::Paths::iterator end()
        {
            return paths.end();
        }
        Clipper3r::Paths::const_iterator end() const
        {
            return paths.end();
        }
        /*!
         * Remove a polygon from the list and move the last polygon to its place
         * 
         * \warning changes the order of the polygons!
         */
        void remove(unsigned int index)
        {
            if (index < paths.size() - 1)
            {
                paths[index] = std::move(paths.back());
            }
            paths.resize(paths.size() - 1);
        }
        /*!
         * Remove a range of polygons
         */
        void erase(Clipper3r::Paths::iterator start, Clipper3r::Paths::iterator end)
        {
            paths.erase(start, end);
        }
        void clear()
        {
            paths.clear();
        }
        void deletePaths()
        {
	        Clipper3r::Paths empty;	
            paths.swap(empty);
        }
        void add(ConstPolygonRef& poly)
        {
            paths.push_back(*poly.path);
        }
        void add(const ConstPolygonRef& poly)
        {
            paths.push_back(*poly.path);
        }
        void add(Polygon&& other_poly)
        {
            paths.emplace_back(std::move(*other_poly));
        }
        void add(const Polygons& other)
        {
            std::copy(other.paths.begin(), other.paths.end(), std::back_inserter(paths));
        }
        /*!
         * Add a 'polygon' consisting of two points
         */
        void addLine(const Point from, const Point to)
        {
            paths.emplace_back(Clipper3r::Path{from, to});
        }

        template<typename... Args>
        void emplace_back(Args... args)
        {
            paths.emplace_back(args...);
        }

        PolygonRef newPoly()
        {
            paths.emplace_back();
            return PolygonRef(paths.back());
        }
        PolygonRef back()
        {
            return PolygonRef(paths.back());
        }
        ConstPolygonRef back() const
        {
            return ConstPolygonRef(paths.back());
        }

        Polygons() {}

        Polygons(const Polygons& other) { paths = other.paths; }
        Polygons(Polygons&& other) { paths = std::move(other.paths); }
        Polygons& operator=(const Polygons& other) { paths = other.paths; return *this; }
        Polygons& operator=(Polygons&& other) { paths = std::move(other.paths); return *this; }

        bool operator==(const Polygons& other) const =delete;

        /*!
         * Convert Clipper3r::PolyTree to a Polygons object,
         * which uses Clipper3r::Paths instead of Clipper3r::PolyTree
         */
        static Polygons toPolygons(Clipper3r::PolyTree& poly_tree);

        /*!
         * Split up the polygons into groups according to the even-odd rule.
         * Each PolygonsPart in the result has an outline as first polygon, whereas the rest are holes.
         */
        std::vector<PolygonsPart> splitIntoParts(bool unionAll = false) const;
    private:
    
        Point min() const
        {
            Point ret = Point(POINT_MAX, POINT_MAX);
            for(const Clipper3r::Path& polygon : paths)
            {
                for(Point p : polygon)
                {
                    ret.X = std::min(ret.X, p.X);
                    ret.Y = std::min(ret.Y, p.Y);
                }
            }
            return ret;
        }
    
        Point max() const
        {
            Point ret = Point(POINT_MIN, POINT_MIN);
            for(const Clipper3r::Path& polygon : paths)
            {
                for(Point p : polygon)
                {
                    ret.X = std::max(ret.X, p.X);
                    ret.Y = std::max(ret.Y, p.Y);
                }
            }
            return ret;
        }

        void splitIntoParts_processPolyTreeNode(Clipper3r::PolyNode* node, std::vector<PolygonsPart>& ret) const;
    };

    /*!
     * A single area with holes. The first polygon is the outline, while the rest are holes within this outline.
     * 
     * This class has little more functionality than Polygons, but serves to show that a specific instance is ordered such that the first Polygon is the outline and the rest are holes.
     */
    class PolygonsPart : public Polygons
    {
    public:
        PolygonRef outerPolygon()
        {
            return paths[0];
        }
        ConstPolygonRef outerPolygon() const
        {
            return paths[0];
        }
    };
}//namespace cxutil

#endif//CX_UTILS_POLYGON_H
