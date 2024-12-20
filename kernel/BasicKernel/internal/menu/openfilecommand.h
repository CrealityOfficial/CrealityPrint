#ifndef OPENFILECOMMAND_H
#define OPENFILECOMMAND_H
#include "menu/actioncommand.h"
#include "data/interface.h"
#include "data/kernelenum.h"

namespace creative_kernel
{
    class OpenFileCommand : public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
    public:
        enum OpenType
        {
            All = 0,
            Mesh
        };

        OpenFileCommand();
        virtual ~OpenFileCommand();

        void setFileType(OpenType type);

        void setJoinGroupEnabled(bool enabled);
        void setModelType(ModelNType type);
        Q_INVOKABLE void execute();
    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;

    private:
        OpenType m_openType { All };
        ModelNType m_partType { ModelNType::normal_part };
        bool m_joinGroupEnabled {false};
    };
}

#endif // OPENFILECOMMAND_H
