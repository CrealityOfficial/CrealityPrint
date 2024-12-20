#ifndef PLACER_BOILERPLATE_HPP
#define PLACER_BOILERPLATE_HPP

#include <libnest2d/nester.hpp>

namespace libnest2d { namespace placers {

struct EmptyConfig {};

template<class Subclass, class RawShape, class TBin, class Cfg = EmptyConfig>
class PlacerBoilerplate {
    mutable bool farea_valid_ = false;
    mutable double farea_ = 0.0;
public:
    using ShapeType = RawShape;
    using Item = _Item<RawShape>;
    using Vertex = TPoint<RawShape>;
    using Segment = _Segment<Vertex>;
    using BinType = TBin;
    using Coord = TCoord<Vertex>;
    using Config = Cfg;
    using ItemGroup = _ItemGroup<RawShape>;
    using DefaultIter = typename ItemGroup::const_iterator;

    class PackResult {
        Item *item_ptr_;
        Vertex move_;
        Radians rot_;
        double overfit_;
        friend class PlacerBoilerplate;
        friend Subclass;

        PackResult(Item& item):
            item_ptr_(&item),
            move_(item.translation()),
            rot_(item.rotation()) {}

        PackResult(Item& item, bool pack_out) :
            item_ptr_(nullptr),
            move_(item.translation()),
            rot_(item.rotation()) {}

        PackResult(double overfit = 1.0):
            item_ptr_(nullptr), overfit_(overfit) {}

    public:
        operator bool() { return item_ptr_ != nullptr; }
        double overfit() const { return overfit_; }
    };

    inline PlacerBoilerplate(const BinType& bin, const Config& cfg, unsigned cap = 50)
    {
        configure(cfg);
        bin_ = bin;
        items_.reserve(cap);
    }

    inline const BinType& bin() const BP2D_NOEXCEPT { return bin_; }

    template<class TB> inline void bin(TB&& b) {
        bin_ = std::forward<BinType>(b);
    }

    inline void configure(const Config& config) BP2D_NOEXCEPT {
        config_ = config;
    }

    template<class Range = ConstItemRange<DefaultIter>>
    bool pack(Item& item, const Range& rem = Range()) {
        auto&& r = static_cast<Subclass*>(this)->trypack(item, rem);
        if(r) {
            items_.emplace_back(*(r.item_ptr_));
            farea_valid_ = false;
        }
        if(r.move_.X != 0 || r.move_.Y != 0 || double(r.rot_) == -0.5*Pi)  return true;
        return r;
    }

    void preload(const ItemGroup& packeditems) {
        items_.insert(items_.end(), packeditems.begin(), packeditems.end());
        farea_valid_ = false;
    }

    void accept(PackResult& r) {
        if(r) {
            r.item_ptr_->translation(r.move_);
            r.item_ptr_->rotation(r.rot_);
            items_.emplace_back(*(r.item_ptr_));
            farea_valid_ = false;
        }
    }

    void unpackLast() {
        items_.pop_back();
        farea_valid_ = false;
    }

    void load_unpack(const ItemGroup& items)
    {
        for (const auto& item : items) {
            unpacked_items_.emplace_back(item);
        }
        farea_valid_ = false;
    }
    
    void load_translate(const Vertex& tr)
    {
        trans_ = tr;
    }

    inline const ItemGroup& getItems() const { return items_; }

    inline const ItemGroup& getUnPackItems() const { return unpacked_items_; }

    inline void clearItems() {
        items_.clear();
        farea_valid_ = false;
    }

    inline double filledArea() const {
        if(farea_valid_) return farea_;
        else {
            farea_ = .0;
            std::for_each(items_.begin(), items_.end(),
                          [this] (Item& item) {
                farea_ += item.area();
            });
            farea_valid_ = true;
        }

        return farea_;
    }

protected:
    Vertex bl_;
    BinType bin_;
    Vertex trans_;
    ItemGroup items_;
    ItemGroup unpacked_items_;
    Cfg config_;
};


#define DECLARE_PLACER(Base) \
using Base::bl_;                  \
using Base::bin_;                 \
using Base::trans_;               \
using Base::items_;               \
using Base::unpacked_items_;      \
using Base::config_;              \
public:                           \
using typename Base::ShapeType;   \
using typename Base::Item;        \
using typename Base::ItemGroup;   \
using typename Base::BinType;     \
using typename Base::Config;      \
using typename Base::Vertex;      \
using typename Base::Segment;     \
using typename Base::PackResult;  \
using typename Base::Coord;       \
private:

}
}

#endif // PLACER_BOILERPLATE_HPP
