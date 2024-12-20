#ifndef LAYOUT_ITEM_EX_202404301124_H
#define LAYOUT_ITEM_EX_202404301124_H

#include "layoutmanager.h"

namespace creative_kernel
{
    class LayoutItemEx : public QObject
    {
    public:
        LayoutItemEx(int groupId, bool layoutConcave = false, QObject* parent = nullptr);
        virtual ~LayoutItemEx();

        std::vector<trimesh::vec3> outLine();

    public:
        int modelGroupId;
        creative_kernel::LayoutNestResultEx nestResult;
        bool m_outlineConcave;
    };

}

#endif // LAYOUT_ITEM_EX_202404301124_H
