#include "external/message/bedtypeinvalidmessage.h"

#include <qtusercore/util/translateutil.h>

namespace creative_kernel {

  BedTypeInvalidMessage::BedTypeInvalidMessage(int            platform_index,
                                               const QString& platform_bed_type,
                                               int            material_index,
                                               const QString& material_name,
                                               QObject*       parent)
      : MessageObject{ parent }
      , platform_index_{ platform_index }
      , platform_bed_type_{ platform_bed_type }
      , material_index_{ material_index }
      , material_name_{ material_name } {}

  auto BedTypeInvalidMessage::codeImpl() -> int {
    return MessageType::BedTypeInvalid;
  }

  auto BedTypeInvalidMessage::levelImpl() -> int {
    return MessageLevel::Error;
  }

  auto BedTypeInvalidMessage::messageImpl() -> QString {
    using namespace cx::literals;
    return u8"bed_type_unvalid_message_format"_tr.arg(
        cx::translate(u8"ParameterComponent", platform_bed_type_),
        QString::number(material_index_ + 1),
        cx::tr(material_name_));
  }

  auto BedTypeInvalidMessage::linkStringImpl() -> QString {
    return {};
  }

  auto BedTypeInvalidMessage::handleImpl() -> void {
    return;
  }

}  // namespace creative_kernel
