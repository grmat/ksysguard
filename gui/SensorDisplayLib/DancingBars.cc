/*
    KSysGuard, the KDE System Guard
   
	Copyright (c) 1999, 2000, 2001 Chris Schlaeger <cs@kde.org>
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of version 2 of the GNU General Public
    License as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

	KSysGuard is currently maintained by Chris Schlaeger
	<cs@kde.org>. Please do not commit any changes without consulting
	me first. Thanks!

	$Id$
*/

#include <qcheckbox.h>
#include <qdom.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qspinbox.h>
#include <qtooltip.h>

#include <kdebug.h>
#include <klocale.h>
#include <knumvalidator.h>

#include <ksgrd/ColorPicker.h>
#include <ksgrd/SensorManager.h>
#include <ksgrd/StyleEngine.h>

#include "BarGraphSettings.h"
#include "DancingBars.moc"
#include "DancingBarsSettings.h"

DancingBars::DancingBars(QWidget* parent, const char* name,
						 const QString& title, int, int, bool nf)
	: KSGRD::SensorDisplay(parent, name, title)
{
	bars = 0;
	flags = 0;
	noFrame = nf;

	if (noFrame)
		plotter = new BarGraph(this, "signalPlotter");
	else
		plotter = new BarGraph(frame, "signalPlotter");
	Q_CHECK_PTR(plotter);

	setMinimumSize(sizeHint());

	/* All RMB clicks to the plotter widget will be handled by 
	 * SensorDisplay::eventFilter. */
	plotter->installEventFilter(this);

	registerPlotterWidget(plotter);

	setModified(false);
}

DancingBars::~DancingBars()
{
}

void
DancingBars::settings()
{
	dbs = new DancingBarsSettings(this, "DancingBarsSettings", true);
	Q_CHECK_PTR(dbs);

	dbs->title->setText(getTitle());
	dbs->title->setFocus();
	dbs->minVal->setValidator(new KFloatValidator(dbs->minVal));
	dbs->minVal->setText(QString("%1").arg(plotter->getMin()));
	dbs->maxVal->setValidator(new KFloatValidator(dbs->maxVal));
	dbs->maxVal->setText(QString("%1").arg(plotter->getMax()));

	double l, u;
	bool la, ua;
	plotter->getLimits(l, la, u, ua);
	dbs->upperLimitActive->setChecked(ua);
	dbs->upperLimit->setValidator(new KFloatValidator(dbs->upperLimit));
	dbs->upperLimit->setText(QString("%1").arg(u));
	dbs->lowerLimitActive->setChecked(la);
	dbs->lowerLimit->setValidator(new KFloatValidator(dbs->lowerLimit));
	dbs->lowerLimit->setText(QString("%1").arg(l));

	dbs->normalColor->setColor(plotter->normalColor);
	dbs->alarmColor->setColor(plotter->alarmColor);
	dbs->backgroundColor->setColor(plotter->backgroundColor);
	dbs->fontSize->setValue(plotter->fontSize);

	for (uint i = bars - 1; i < bars; i--)
	{
		QString status = sensors.at(i)->ok ? i18n("Ok") : i18n("Error");
		QListViewItem* lvi = new QListViewItem(
			dbs->sensorList,
			sensors.at(i)->hostName,
			KSGRD::SensorMgr->translateSensor(sensors.at(i)->name),
			plotter->footers[i],
			KSGRD::SensorMgr->translateUnit(sensors.at(i)->unit), status);
	}
	connect(dbs->editButton, SIGNAL(clicked()),
			this, SLOT(settingsEdit()));
	connect(dbs->deleteButton, SIGNAL(clicked()),
			this, SLOT(settingsDelete()));
	connect(dbs->sensorList, SIGNAL(selectionChanged(QListViewItem*)),
			this, SLOT(settingsSelectionChanged(QListViewItem*)));

	connect(dbs->applyButton, SIGNAL(clicked()),
			this, SLOT(applySettings()));

	if (dbs->exec())
		applySettings();

	delete dbs;
	dbs = 0;
}

void
DancingBars::applySettings()
{
	setTitle(dbs->title->text());
	plotter->changeRange(dbs->minVal->text().toDouble(),
						 dbs->maxVal->text().toDouble());

	plotter->setLimits(dbs->lowerLimitActive->isChecked() ?
					   dbs->lowerLimit->text().toDouble() : 0,
					   dbs->lowerLimitActive->isChecked(),
					   dbs->upperLimitActive->isChecked() ?
					   dbs->upperLimit->text().toDouble() : 0,
					   dbs->upperLimitActive->isChecked());

	plotter->normalColor = dbs->normalColor->getColor();
	plotter->alarmColor = dbs->alarmColor->getColor();
	plotter->backgroundColor = dbs->backgroundColor->getColor();
	plotter->fontSize = dbs->fontSize->value();

	QListViewItemIterator it(dbs->sensorList);
	/* Iterate through all items of the listview and reverse iterate
	 * through the registered sensors. */
	for (uint i = 0; i < sensors.count(); i++)
	{
		if (it.current() &&
			it.current()->text(0) == sensors.at(i)->hostName &&
			it.current()->text(1) == 
			KSGRD::SensorMgr->translateSensor(sensors.at(i)->name))
		{
			plotter->footers[i] = it.current()->text(2);
			it++;
		}
		else
		{
			removeSensor(i);
			i--;
		}
	}

	repaint();
	setModified(true);
}

void
DancingBars::applyStyle()
{
	plotter->normalColor = KSGRD::Style->getFgColor1();
	plotter->alarmColor = KSGRD::Style->getAlarmColor();
	plotter->backgroundColor = KSGRD::Style->getBackgroundColor();
	plotter->fontSize = KSGRD::Style->getFontSize();

	repaint();
	setModified(true);
}

void
DancingBars::settingsEdit()
{
	QListViewItem* lvi = dbs->sensorList->currentItem();

	if (!lvi)
		return;

	BarGraphSettings* bgs = new BarGraphSettings(
		this, "BarsGraphSettings", true);
	Q_CHECK_PTR(bgs);

	bgs->label->setText(lvi->text(2));
	if (bgs->exec())
		lvi->setText(2, bgs->label->text());
}

void
DancingBars::settingsDelete()
{
	QListViewItem* lvi = dbs->sensorList->currentItem();

	if (lvi)
	{
		/* Before we delete the currently selected item, we determine a
		 * new item to be selected. That way we can ensure that multiple
		 * items can be deleted without forcing the user to select a new
		 * item between the deletes. If all items are deleted, the buttons
		 * are disabled again. */
		QListViewItem* newSelected = 0;
		if (lvi->itemBelow())
		{
			lvi->itemBelow()->setSelected(true);
			newSelected = lvi->itemBelow();
		}
		else if (lvi->itemAbove())
		{
			lvi->itemAbove()->setSelected(true);
			newSelected = lvi->itemAbove();
		}
		else
			settingsSelectionChanged(0);
			
		delete lvi;

		if (newSelected)
			dbs->sensorList->ensureItemVisible(newSelected);
	}
}

void
DancingBars::settingsSelectionChanged(QListViewItem* lvi)
{
	dbs->editButton->setEnabled(lvi != 0);
	dbs->deleteButton->setEnabled(lvi != 0);
}

bool
DancingBars::addSensor(const QString& hostName, const QString& sensorName, const QString& sensorType,
					   const QString& title)
{
	if (sensorType != "integer" && sensorType != "float")
		return (false);

	if (bars >= 32)
		return (false);

	if (!plotter->addBar(title))
		return (false);

	registerSensor(new KSGRD::SensorProperties(hostName, sensorName, sensorType, title));

	/* To differentiate between answers from value requests and info
	 * requests we add 100 to the beam index for info requests. */
	sendRequest(hostName, sensorName + "?", bars + 100);
	++bars;
	sampleBuf.resize(bars);

	QString tooltip;
	for (uint i = 0; i < bars; ++i)
	{
		if (i == 0)
			tooltip += QString("%1:%2").arg(sensors.at(i)->hostName).arg(sensors.at(i)->name);
		else
			tooltip += QString("\n%1:%2").arg(sensors.at(i)->hostName).arg(sensors.at(i)->name);
	}
	QToolTip::remove(plotter);
	QToolTip::add(plotter, tooltip);

	return (true);
}

bool
DancingBars::removeSensor(uint idx)
{
	if (idx >= bars)
	{
		kdDebug() << "DancingBars::removeSensor: idx out of range ("
				  << idx << ")" << endl;
		return (false);
	}

	plotter->removeBar(idx);
	bars--;
	KSGRD::SensorDisplay::removeSensor(idx);

	QString tooltip;
	for (uint i = 0; i < bars; ++i)
	{
		if (i == 0)
			tooltip += QString("%1:%2").arg(sensors.at(i)->hostName).arg(sensors.at(i)->name);
		else
			tooltip += QString("\n%1:%2").arg(sensors.at(i)->hostName).arg(sensors.at(i)->name);
	}
	QToolTip::remove(plotter);
	QToolTip::add(plotter, tooltip);

	return (true);
}

void
DancingBars::resizeEvent(QResizeEvent*)
{
	if (noFrame)
		plotter->setGeometry(0, 0, width(), height());
	else
		frame->setGeometry(0, 0, width(), height());
}

QSize
DancingBars::sizeHint(void)
{
	if (noFrame)
		return (plotter->sizeHint());
	else
		return (frame->sizeHint());
}

void
DancingBars::answerReceived(int id, const QString& answer)
{
	/* We received something, so the sensor is probably ok. */
	sensorError(id, false);
	
	if (id < 100)
	{
		sampleBuf[id] = answer.toDouble();
		if (flags & (1 << id))
		{
			kdDebug() << "ERROR: DancingBars lost sample (" << flags
					  << ", " << bars << ")" << endl;
			sensorError(id, true);
		}
		flags |= 1 << id;

		if (flags == (uint) ((1 << bars) - 1))
		{
			plotter->updateSamples(sampleBuf);
			flags = 0;
		}
	}
	else if (id >= 100)
	{
		KSGRD::SensorIntegerInfo info(answer);
		if (id == 100)
			if (plotter->getMin() == 0.0 && plotter->getMax() == 0.0)
			{
				/* We only use this information from the sensor when the
				 * display is still using the default values. If the
				 * sensor has been restored we don't touch the already set
				 * values. */
				plotter->changeRange(info.getMin(), info.getMax());
			}

		sensors.at(id - 100)->unit = info.getUnit();
	}
}

bool
DancingBars::createFromDOM(QDomElement& element)
{
	plotter->changeRange(element.attribute("min", "0").toDouble(),
						element.attribute("max", "0").toDouble());

	plotter->setLimits(element.attribute("lowlimit", "0").toDouble(),
					element.attribute("lowlimitactive", "0").toInt(),
					element.attribute("uplimit", "0").toDouble(),
					element.attribute("uplimitactive", "0").toInt());

	plotter->normalColor = restoreColorFromDOM(element, "normalColor",
											   KSGRD::Style->getFgColor1());
	plotter->alarmColor = restoreColorFromDOM(element, "alarmColor",
											  KSGRD::Style->getAlarmColor());
	plotter->backgroundColor = restoreColorFromDOM(
		element, "backgroundColor", KSGRD::Style->getBackgroundColor());
	plotter->fontSize = element.attribute(
		"fontSize", QString("%1").arg(KSGRD::Style->getFontSize())).toInt();

	QDomNodeList dnList = element.elementsByTagName("beam");
	for (uint i = 0; i < dnList.count(); ++i)
	{
		QDomElement el = dnList.item(i).toElement();
		addSensor(el.attribute("hostName"), el.attribute("sensorName"), (el.attribute("sensorType").isEmpty() ? "integer" : el.attribute("sensorType")), el.attribute("sensorDescr"));
	}

	internCreateFromDOM(element);

	setModified(false);

	return (true);
}

bool
DancingBars::addToDOM(QDomDocument& doc, QDomElement& element, bool save)
{
	element.setAttribute("min", plotter->getMin());
	element.setAttribute("max", plotter->getMax());
	double l, u;
	bool la, ua;
	plotter->getLimits(l, la, u, ua);
	element.setAttribute("lowlimit", l);
	element.setAttribute("lowlimitactive", la);
	element.setAttribute("uplimit", u);
	element.setAttribute("uplimitactive", ua);

	addColorToDOM(element, "normalColor", plotter->normalColor);
	addColorToDOM(element, "alarmColor", plotter->alarmColor);
	addColorToDOM(element, "backgroundColor", plotter->backgroundColor);
	element.setAttribute("fontSize", plotter->fontSize);

	for (uint i = 0; i < bars; ++i)
	{
		QDomElement beam = doc.createElement("beam");
		element.appendChild(beam);
		beam.setAttribute("hostName", sensors.at(i)->hostName);
		beam.setAttribute("sensorName", sensors.at(i)->name);
		beam.setAttribute("sensorType", sensors.at(i)->type);
		beam.setAttribute("sensorDescr", plotter->footers[i]);
	}

	internAddToDOM(doc, element);

	if (save)
		setModified(false);

	return (true);
}