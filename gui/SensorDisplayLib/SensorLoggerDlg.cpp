/* This file is part of the KDE project
   Copyright ( C ) 2003 Nadeem Hasan <nhasan@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "SensorLoggerDlg.h"
#include "ui_SensorLoggerDlgWidget.h"

#include <KLocalizedString>

SensorLoggerDlg::SensorLoggerDlg( QWidget *parent, const char *name )
    : QDialog( parent )
{
  setObjectName( name );
  setModal( true );
  setWindowTitle( i18n( "Sensor Logger" ) );

  QWidget *main = new QWidget( this );

  m_loggerWidget = new Ui_SensorLoggerDlgWidget;
  m_loggerWidget->setupUi( main );
  m_loggerWidget->m_fileName->setMode(KFile::File|KFile::LocalOnly);
  connect(m_loggerWidget->m_fileName, &KUrlRequester::textChanged, this, &SensorLoggerDlg::fileNameTextChanged);

  fileNameTextChanged();

  connect(m_loggerWidget->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(m_loggerWidget->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

  QVBoxLayout *vlayout = new QVBoxLayout(this);
  vlayout->addWidget(main);
  setLayout(vlayout);
}

SensorLoggerDlg::~SensorLoggerDlg()
{
  delete m_loggerWidget;
}

void SensorLoggerDlg::fileNameTextChanged()
{
  m_loggerWidget->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(m_loggerWidget->m_fileName->url().isValid() && !m_loggerWidget->m_fileName->url().isRelative());
}

QString SensorLoggerDlg::fileName() const
{
  return m_loggerWidget->m_fileName->url().path();
}

int SensorLoggerDlg::timerInterval() const
{
  return m_loggerWidget->m_timerInterval->value();
}

bool SensorLoggerDlg::lowerLimitActive() const
{
  return m_loggerWidget->m_lowerLimitActive->isChecked();
}

bool SensorLoggerDlg::upperLimitActive() const
{
  return m_loggerWidget->m_upperLimitActive->isChecked();
}

double SensorLoggerDlg::lowerLimit() const
{
  return m_loggerWidget->m_lowerLimit->text().toDouble();
}

double SensorLoggerDlg::upperLimit() const
{
  return m_loggerWidget->m_upperLimit->text().toDouble();
}

void SensorLoggerDlg::setFileName( const QString &url )
{
  m_loggerWidget->m_fileName->setUrl( QUrl::fromLocalFile(url) );
}

void SensorLoggerDlg::setTimerInterval( int i )
{
  m_loggerWidget->m_timerInterval->setValue( i );
}

void SensorLoggerDlg::setLowerLimitActive( bool b )
{
  m_loggerWidget->m_lowerLimitActive->setChecked( b );
}

void SensorLoggerDlg::setUpperLimitActive( bool b )
{
  m_loggerWidget->m_upperLimitActive->setChecked( b );
}

void SensorLoggerDlg::setLowerLimit( double limit )
{
  m_loggerWidget->m_lowerLimit->setText( QString::number( limit ) );
}

void SensorLoggerDlg::setUpperLimit( double limit )
{
  m_loggerWidget->m_upperLimit->setText( QString::number( limit ) );
}



/* vim: et sw=2 ts=2
*/

