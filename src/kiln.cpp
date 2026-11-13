#include "skopin/kiln.hpp"
#include "skopin/measures.hpp"

#include <stdexcept>

namespace skopin {

Kiln::Kiln() {
    cone_ = Cone::Cold;
    door_ = Door::Closed;
    stokes_ = 0;
}

Cone Kiln::cone() const {
    return cone_;
}

Door Kiln::door() const {
    return door_;
}

int Kiln::stokes() const {
    return stokes_;
}

std::vector<Ware> Kiln::stack() const {
    return stack_;
}

void Kiln::open() {
    if (door_ == Door::Open) {
        throw KilnError("Kiln::open: дверца уже открыта");
    }
    if (cone_ == Cone::Glaze) {
        throw KilnError("Kiln::open: на поливном конусе дверцу не открыть");
    }
    door_ = Door::Open;
}

void Kiln::close() {
    if (door_ == Door::Closed) {
        throw KilnError("Kiln::close: дверца уже закрыта");
    }
    door_ = Door::Closed;
}

void Kiln::fire() {
    if (door_ == Door::Open) {
        throw KilnError("Kiln::fire: дверца открыта");
    }
    if (cone_ == Cone::Glaze) {
        throw KilnError("Kiln::fire: выше некуда");
    }

    for (size_t i = 0; i < stack_.size(); i++) {
        if (stack_[i].thickness_mm > kCrackThicknessMm) {
            throw KilnError("Kiln::fire: изделие слишком толстое");
        }
    }

    if (cone_ == Cone::Bisque) {
        for (size_t i = 0; i < stack_.size(); i++) {
            if (stack_[i].program == Program::Bisque) {
                throw KilnError("Kiln::fire: утиль треснет на поливном конусе");
            }
        }
    }

    if (cone_ == Cone::Cold) {
        cone_ = Cone::Bisque;
    } else {
        cone_ = Cone::Glaze;
        for (size_t i = 0; i < glazed_.size(); i++) {
            glazed_[i] = true;
        }
    }
    stokes_++;
}

void Kiln::cool() {
    if (door_ == Door::Open) {
        throw KilnError("Kiln::cool: дверца открыта");
    }
    if (cone_ == Cone::Cold) {
        throw KilnError("Kiln::cool: ниже некуда");
    }

    if (cone_ == Cone::Bisque) {
        for (size_t i = 0; i < stack_.size(); i++) {
            if (stack_[i].program == Program::Glaze and not glazed_[i]) {
                throw KilnError("Kiln::cool: полива ещё не была на поливном конусе");
            }
        }
        cone_ = Cone::Cold;
    } else {
        cone_ = Cone::Bisque;
    }
    stokes_++;
}

void Kiln::load(const Ware& ware) {
    if (door_ != Door::Open) {
        throw KilnError("Kiln::load: дверца закрыта");
    }

    if (cone_ == Cone::Glaze) {
        throw KilnError("Kiln::load: на поливной конус не сажают");
    }
    if (cone_ == Cone::Bisque and ware.program != Program::Glaze) {
        throw KilnError("Kiln::load: на утильный конус сажают только поливу");
    }

    if (validateWare(ware).has_value()) {
        throw KilnError("Kiln::load: плохая запись об изделии");
    }

    for (size_t i = 0; i < stack_.size(); i++) {
        if (stack_[i].mark == ware.mark) {
            throw KilnError("Kiln::load: такое клеймо уже внутри");
        }
    }

    if (ware.kind == WareKind::Sagger and stack_.size() > 0) {
        throw KilnError("Kiln::load: капсель входит только в пустой горн");
    }
    if (stack_.size() > 0 and stack_[0].kind == WareKind::Sagger) {
        throw KilnError("Kiln::load: к капселю никого не ставят");
    }

    if (not fitsInKiln(stack_, ware)) {
        throw KilnError("Kiln::load: не влезает по высоте");
    }

    stack_.push_back(ware);
    glazed_.push_back(false);
}

void Kiln::unload(const std::string& mark) {
    if (door_ != Door::Open) {
        throw KilnError("Kiln::unload: дверца закрыта");
    }
    if (stack_.size() == 0) {
        throw KilnError("Kiln::unload: горн пуст");
    }

    size_t top = stack_.size() - 1;
    if (stack_[top].mark != mark) {
        throw KilnError("Kiln::unload: наверху другое клеймо");
    }
    if (stack_[top].program == Program::Glaze and cone_ != Cone::Cold) {
        throw KilnError("Kiln::unload: поливу снимают только с холодного конуса");
    }

    stack_.pop_back();
    glazed_.pop_back();
}

}  // namespace skopin
