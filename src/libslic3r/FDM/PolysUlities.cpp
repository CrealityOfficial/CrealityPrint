#include "PolysUlities.hpp"
#include "libslic3r/ClipperUtils.hpp"

namespace creality
{
    struct PolyContainment
    {
        Slic3r::ExPolygon out;
        Slic3r::Polylines polyline;
    };

    struct RegionContainment
    {
        std::vector<PolyContainment> outs;
    };

    void contains(const std::vector<Slic3r::ExPolygons*>& sources, std::vector<int>& all_indices)
    {
        all_indices.clear();
        int size = (int)sources.size();
        if(size <= 1)
            return;
            
        std::vector<RegionContainment> regions(size);
        for(int i = 0; i < size; i++)
        {
            RegionContainment& region = regions[i];
            Slic3r::ExPolygons* region_polys = sources[i];
            int out_size = (int)region_polys->size();
            if(out_size > 0)
            {
                region.outs.resize(out_size);
                for(int j = 0; j < out_size; ++j)
                {
                    PolyContainment& poly_con = region.outs.at(j);
                    poly_con.out = Slic3r::ExPolygon(region_polys->at(j).contour);
                    poly_con.polyline = Slic3r::to_polylines(region_polys->at(j).contour);
                }
            }
        }

        std::vector<std::pair<int, int>> pairs;
        for(int i = 0; i < size; i++)
        {
            for(int j = i + 1; j < size; j++)
            {
                pairs.push_back(std::make_pair(i, j));
            }
        }

        auto check_in = [](const RegionContainment& region1, const RegionContainment& region2)->bool{
            bool in = true;
            for(const PolyContainment& poly2 : region2.outs)
            {
                bool sub_contain = false;
                for(const PolyContainment& poly1 : region1.outs)
                {
                    if(poly1.out.contains(poly2.polyline))
                    {
                        sub_contain = true;
                        break;
                    }
                }

                if(!sub_contain)
                {
                    in = false;
                    break;
                }
            }

            return in;
        };

        for(const std::pair<int, int>& p : pairs)
        {
            const RegionContainment& region1 = regions[p.first];
            const RegionContainment& region2 = regions[p.second];

            if(check_in(region1, region2))
            {
                all_indices.push_back(p.second);
            }
            else if(check_in(region2, region1))
            {
                all_indices.push_back(p.first);
            }
        }
    }
}