#include "dialog.h"
#include <QtWidgets/QDialog>
#include <QtWidgets/QBoxLayout>

namespace qtuser_core
{
	void showModelDialog(QWidget* widget)
	{
		if (!widget)
			return;

		QBoxLayout* layout = new QBoxLayout(QBoxLayout::LeftToRight);
		layout->addWidget(widget);
		QDialog dialog;
		dialog.setWindowTitle(widget->windowTitle());
		dialog.setLayout(layout);
		dialog.exec();
	}
}