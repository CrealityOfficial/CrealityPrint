#include <QtCore/QtCore>
#include <QtGui/QtGui>

// MacWindow provides a Qt C++ API for creating NSWindows in
// various configuartions. The idea is to make it possible to
// quickly test window configurations from C++ code. This API is not
// complete and shold be used as a starting point for a custom implementation.

Q_FORWARD_DECLARE_OBJC_CLASS(NSWindow);
class MyCppObject;
class MacWindow
{
public:
    MacWindow();
    // Content Windows
    void setContentWindow(QWindow *contentWindow);

    // NSWindow properties
    void setFullSizeContentView(bool enable);
    bool fullSizeContentView() const;
    void setTitlebarAppearsTransparent(bool enable);
    bool titlebarAppearsTransparent() const;
    void setTitleVisibility(bool enable);
    bool titleVisibility() const;
    void setVisible(bool visible);
    void show();
//    void setMacObject(MyCppObject *cppObject);

    MyCppObject *getMacAppObject();
protected:
    void createNSWindow();
    void destroyNSWindow();
    void recreateNSWindow();
    void scheduleRecreateNSWindow();

private:
    QWindow *m_window = nullptr;
    NSWindow *m_nsWindow = nullptr;
    MyCppObject *m_macAppObject;
    QString m_titleText;
    bool m_visible;

    bool m_fullSizeContentView = false;
    bool m_titlebarAppearsTransparent = false;
    bool m_movableByWindowBackground = false;
    bool m_titleVisibility = true;
    bool m_accessoryView = false;
};
