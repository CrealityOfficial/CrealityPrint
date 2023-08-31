#ifndef COMPUTEFUNC_1603848105104_H
#define COMPUTEFUNC_1603848105104_H

#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLExtraFunctions>
#include <functional>

typedef std::function<void(QOpenGLExtraFunctions*)> computeFunc;
#endif // COMPUTEFUNC_1603848105104_H