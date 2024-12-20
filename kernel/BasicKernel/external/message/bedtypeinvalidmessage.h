#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>

#include "basickernelexport.h"
#include "message/messageobject.h"

namespace creative_kernel {

  class BASIC_KERNEL_API BedTypeInvalidMessage : public MessageObject {
    Q_OBJECT;

   public:
    explicit BedTypeInvalidMessage(int            platform_index,
                                   const QString& platform_bed_type,
                                   int            material_index,
                                   const QString& material_name,
                                   QObject*       parent = nullptr);
    virtual ~BedTypeInvalidMessage() override = default;

   protected:
    auto codeImpl() -> int override;
    auto levelImpl() -> int override;
    auto messageImpl() -> QString override;
    auto linkStringImpl() -> QString override;
    auto handleImpl() -> void override;

   private:
    int     platform_index_{ -1 };
    QString platform_bed_type_{};
    int     material_index_{ -1 };
    QString material_name_{};
  };

}  // namespace creative_kernel
