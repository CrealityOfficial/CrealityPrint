#include "lettercommand.h"
#include "letterop.h"

#include "interface/visualsceneinterface.h"
#include "interface/selectorinterface.h"
#include "interface/modelinterface.h"
#include <QCoreApplication>
#include <QDirIterator>

#include <QStandardPaths>
#include "kernel/kernel.h"
#include "kernel/kernelui.h"
#include "interface/uiinterface.h"
#include "qtusercore/string/resourcesfinder.h"
#include "interface/commandinterface.h"

LetterCommand::LetterCommand(QObject* parent) : ToolCommand(parent), m_pOp(nullptr)
{
	
}

LetterCommand::~LetterCommand()
{
	if (m_pOp)
	{
		delete m_pOp;
	}
}

void LetterCommand::slotMouseLeftClicked()
{
	message();
}

bool LetterCommand::message()
{
	if (m_pOp->getMessage())
	{
		creative_kernel::requestQmlDialog(this, "deleteSupportDlg");
	}

	return true;
}

void LetterCommand::setMessage(bool isRemove)
{
	m_pOp->setMessage(isRemove);
}

void LetterCommand::execute()
{
	if (!m_pOp)
	{
		m_pOp = new LetterOp(this);
		connect(m_pOp, SIGNAL(mouseLeftClicked()), this, SLOT(slotMouseLeftClicked()));
	}

	// find fonts
	QStringList search_paths;
	search_paths.append(QCoreApplication::applicationDirPath());
	search_paths.append(QCoreApplication::applicationDirPath() + "/resources");
	int index = QCoreApplication::applicationDirPath().lastIndexOf("/");
	search_paths.append(QCoreApplication::applicationDirPath().left(index) + "/Resources/resources");
	search_paths.append(qtuser_core::getOrCreateAppDataLocation("") + "/resources");

	m_listFonts.clear();

	for (QString& dir : search_paths)
	{
		QDirIterator iter(dir + "/fonts", QDirIterator::IteratorFlag::NoIteratorFlags);
		while (iter.hasNext())
		{
			iter.next();

			QFileInfo fileinfo = iter.fileInfo();
			if (fileinfo.isFile() && fileinfo.suffix() == "ttf")
			{
				m_listFonts.append(fileinfo.baseName());
			}
		}
	}
	m_listFonts = m_listFonts.toSet().toList();

	creative_kernel::setVisOperationMode(nullptr);
	creative_kernel::sensorAnlyticsTrace("Model Editing & Layout", "Letter");
}

void LetterCommand::startLetter()
{
	creative_kernel::setVisOperationMode(m_pOp);
}

void LetterCommand::endLetter()
{
	creative_kernel::setVisOperationMode(nullptr);
	getKernelUI()->switchPickMode();
}


bool LetterCommand::checkSelect()
{
	return creative_kernel::selectionms().size() > 0;
}


void LetterCommand::loadFont()
{
}

QStringList LetterCommand::getFontList()
{
	return m_listFonts;
}

int LetterCommand::getCurFontIndex()
{
	int index = -1;

	for (size_t i = 0; i < m_listFonts.size(); i++)
	{
		if (m_pOp && m_pOp->GetFont() == m_listFonts[i])
		{
			index = i;
			break;
		}
	}

	return index;
}

void LetterCommand::setCurFontIndex(int cur_font_index)
{
	if (m_pOp && 0 < cur_font_index && cur_font_index < m_listFonts.size())
	{
		m_pOp->SetFont(m_listFonts[cur_font_index]);
	}
}

float LetterCommand::getHeight()
{
	if (m_pOp)
	{
		return m_pOp->GetHeight();
	}
	return 0.0f;
}

void LetterCommand::setHeight(float the_height)
{
	if (m_pOp)
	{
		m_pOp->SetHeight(the_height);
	}
}

float LetterCommand::getThickness()
{
	if (m_pOp)
	{
		return m_pOp->GetThickness();
	}
	return 0.0f;
}

void LetterCommand::setThickness(float the_thickness)
{
	if (m_pOp)
	{
		m_pOp->SetThickness(the_thickness);
	}
}

QString LetterCommand::getText()
{
	if (m_pOp)
	{
		return m_pOp->GetText();
	}
	return "";
}

void LetterCommand::setText(QString the_text)
{
	if (m_pOp)
	{
		m_pOp->SetText(the_text);
	}
}

int LetterCommand::getTextSide()
{
	if (m_pOp)
	{
		return m_pOp->GetTextSide();
	}
	return 0;
}

void LetterCommand::setTextSide(int the_text_side)
{
	if (m_pOp)
	{
		m_pOp->SetTextSide(the_text_side);
	}
}

void LetterCommand::accept()
{
	setMessage(true);
}

void LetterCommand::cancel()
{
	setMessage(false);
	getKernelUI()->switchPickMode();
}
