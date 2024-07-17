#include "cxkernel/utils/meshjob.h"

namespace cxkernel {

  MeshJob:: MeshJob(QObject* parent) : Job{ parent } {}

  auto MeshJob::attachObserver(MeshJobObserver* observer) -> void {
    observers_.emplace(observer);
  }

  auto MeshJob::detachObserver(MeshJobObserver* observer) -> void {
    observers_.erase(observer);
  }

  auto MeshJob::notifyObserver(MeshJobObserver::Methord methord) -> void {
    for (auto* observer : observers_) {
      if (observer) {
        std::bind(methord, observer, this)();
      }
    }
  }

}  // namespace cxkernel
