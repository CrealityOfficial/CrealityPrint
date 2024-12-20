#pragma once

#include <set>

#include <qtusercore/module/job.h>

namespace cxkernel {

  class MeshJob;

  class MeshJobObserver {
   public:
    using Methord = void(MeshJobObserver::*)(MeshJob*);
    virtual ~MeshJobObserver() = default;
    virtual auto onFinished(MeshJob* job) -> void {};
  };

  class MeshJob : public qtuser_core::Job {
    Q_OBJECT;

   public:
    explicit MeshJob(QObject* parent = nullptr);
    virtual ~MeshJob() = default;

   public:
    auto attachObserver(MeshJobObserver* observer) -> void;
    auto detachObserver(MeshJobObserver* observer) -> void;

   protected:
    auto notifyObserver(MeshJobObserver::Methord methord) -> void;

   private:
    std::set<MeshJobObserver*> observers_{};
  };

}  // namespace cxkernel
