#ifndef SELECTION_BOILERPLATE_HPP
#define SELECTION_BOILERPLATE_HPP

#include <atomic>
#include <libnest2d/nester.hpp>

namespace libnest2d { namespace selections {

template<class RawShape>
class SelectionBoilerplate {
public:
    using ShapeType = RawShape;
    using Item = _Item<RawShape>;
    using ItemGroup = _ItemGroup<RawShape>;
    using PackGroup = _PackGroup<RawShape>;

    inline const PackGroup& getResult() const {
        return packed_bins_;
    }

    inline void progressIndicator(ProgressFunction fn) { progress_ = fn; }

    inline void stopCondition(StopCondition cond) { stopcond_ = cond; }

    inline void clear() { packed_bins_.clear(); }

protected:

    template<class Placer, class Container, class Bin, class PCfg>
    void remove_unpackable_items(Container &c, const Bin &bin, const PCfg& pcfg)
    {
        // Safety test: try to pack each item into an empty bin. If it fails
        // then it should be removed from the list
        auto it = c.begin();
        Placer placer{ bin };
        ItemGroup items;
        const auto& d = pcfg.binItemGap - pcfg.minItemGap / 2.0;
        const auto& w = bin.width();
        const auto& h = bin.height();
        Bin cbin{ bin };
        if (d < std::min(w, h) / 2.0 && packed_bins_.empty()) {
            auto& p1 = cbin.minCorner();
            auto& p2 = cbin.maxCorner();
            const auto& center = cbin.center();
            const auto& x0 = getX(center);
            const auto& y0 = getY(center);
            setX(p1, x0 - w / 2.0 + d);
            setY(p1, y0 - h / 2.0 + d);
            setX(p2, x0 + w / 2.0 - d);
            setY(p2, y0 + h / 2.0 - d);
        }
        while (it != c.end() && !stopcond_()) {

            // WARNING: The copy of itm needs to be created before Placer.
            // Placer is working with references and its destructor still
            // manipulates the item this is why the order of stack creation
            // matters here.
            const Item& itm = *it;
            Item cpy{itm};

            Placer p{cbin};
            p.configure(pcfg);
            if (itm.area() <= 0 || !p.pack(cpy)) {
                static_cast<Item&>(*it).binId(BIN_ID_UNSET);
                items.emplace_back(std::ref(*it));
                it = c.erase(it);
            }
            else it++;
        }
        placer.load_unpack(items);
    }

    PackGroup packed_bins_;
    ProgressFunction progress_ = [](unsigned){};
    StopCondition stopcond_ = [](){ return false; };
};

}
}

#endif // SELECTION_BOILERPLATE_HPP
