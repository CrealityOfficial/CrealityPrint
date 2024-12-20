#include "adaptiveslice.h"

#include "crslice2/crslice.h"

#include "kernel/kernel.h"
#include "internal/parameter/parametermanager.h"
#include "internal/parameter/printmachine.h"
#include "interface/machineinterface.h"

namespace creative_kernel
{
    crslice2::SettingsPtr getSliceParams()
    {
        auto m_setting = createCurrentGlobalSettings();
        crslice2::SettingsPtr settings(new crslice2::Settings());
        const QHash<QString, us::USetting*>& G = m_setting->settings();
        for (QHash<QString, us::USetting*>::const_iterator it = G.constBegin(); it != G.constEnd(); ++it)
        {
            settings->add(it.key().toStdString(), it.value()->str().toStdString());
        }
        return settings;
    }

    std::vector<double> getLayerHeightProfileAdaptive(std::vector<TriMeshPtr> triMesh, float quality)
    {
        return crslice2::getLayerHeightProfileAdaptive(getSliceParams(), triMesh, quality);
    }

    std::vector<double> smoothHeightProfile(const std::vector<double>& profile, std::vector<TriMeshPtr> triMesh, unsigned int radius, bool keep_min)
    {
        return crslice2::smooth_height_profile(getSliceParams(), triMesh, profile, radius, keep_min);
    }

    BASIC_KERNEL_API std::vector<double> generateObjectLayers(const std::vector<double>& profile, std::vector<TriMeshPtr> triMesh)
    {
        if (profile.empty())
        {
            auto m_profile = crslice2::updateObjectLayers(getSliceParams(), triMesh, profile);
            return crslice2::generateObjectLayers(getSliceParams(), triMesh, m_profile);
        }
        return crslice2::generateObjectLayers(getSliceParams(), triMesh, profile);
    }
}