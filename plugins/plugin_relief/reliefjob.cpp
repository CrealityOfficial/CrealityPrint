#include "reliefjob.h"

ReliefJob::ReliefJob(QObject* parent) : qtuser_core::Job{ parent } {}

auto ReliefJob::name() -> QString {
  return QStringLiteral("ReliefJob");
}

auto ReliefJob::description() -> QString {
  return name();
}

auto ReliefJob::work(qtuser_core::Progressor* progressor) -> void {

}

auto ReliefJob::successed(qtuser_core::Progressor* progressor) -> void {

}

auto ReliefJob::failed() -> void {

}
