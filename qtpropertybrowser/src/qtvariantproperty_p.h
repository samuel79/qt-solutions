/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Solutions component.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#pragma once

#include "qtpropertybrowser.h"
#include "qtpropertymanager.h"
#include "qteditorfactory.h"

#if QT_VERSION >= 0x040400
QT_BEGIN_NAMESPACE
#endif

class QtVariantPropertyPrivate
{
    QtVariantProperty *q_ptr;
public:
    QtVariantPropertyPrivate(QtVariantPropertyManager *m) : manager(m) {}

    QtVariantPropertyManager *manager;
};

class QtVariantPropertyManagerPrivate
{
    QtVariantPropertyManager *q_ptr;
    Q_DECLARE_PUBLIC(QtVariantPropertyManager)
public:
    QtVariantPropertyManagerPrivate();

    bool m_creatingProperty;
    bool m_creatingSubProperties;
    bool m_destroyingSubProperties;
    int m_propertyType;

    void slotValueChanged(QtProperty *property, int val);
    void slotRangeChanged(QtProperty *property, int min, int max);
    void slotSingleStepChanged(QtProperty *property, int step);
    void slotValueChanged(QtProperty *property, double val);
    void slotRangeChanged(QtProperty *property, double min, double max);
    void slotSingleStepChanged(QtProperty *property, double step);
    void slotDecimalsChanged(QtProperty *property, int prec);
    void slotValueChanged(QtProperty *property, bool val);
    void slotValueChanged(QtProperty *property, const QString &val);
    void slotRegExpChanged(QtProperty *property, const QRegExp &regExp);
    void slotEchoModeChanged(QtProperty *property, int);
    void slotValueChanged(QtProperty *property, const QDate &val);
    void slotRangeChanged(QtProperty *property, const QDate &min, const QDate &max);
    void slotValueChanged(QtProperty *property, const QTime &val);
    void slotValueChanged(QtProperty *property, const QDateTime &val);
    void slotValueChanged(QtProperty *property, const QKeySequence &val);
    void slotValueChanged(QtProperty *property, const QChar &val);
    void slotValueChanged(QtProperty *property, const QLocale &val);
    void slotValueChanged(QtProperty *property, const QPoint &val);
    void slotValueChanged(QtProperty *property, const QPointF &val);
    void slotValueChanged(QtProperty *property, const QSize &val);
    void slotRangeChanged(QtProperty *property, const QSize &min, const QSize &max);
    void slotValueChanged(QtProperty *property, const QSizeF &val);
    void slotRangeChanged(QtProperty *property, const QSizeF &min, const QSizeF &max);
    void slotValueChanged(QtProperty *property, const QRect &val);
    void slotConstraintChanged(QtProperty *property, const QRect &val);
    void slotValueChanged(QtProperty *property, const QRectF &val);
    void slotConstraintChanged(QtProperty *property, const QRectF &val);
    void slotValueChanged(QtProperty *property, const QColor &val);
    void slotEnumChanged(QtProperty *property, int val);
    void slotEnumNamesChanged(QtProperty *property, const QStringList &enumNames);
    void slotEnumIconsChanged(QtProperty *property, const QMap<int, QIcon> &enumIcons);
    void slotValueChanged(QtProperty *property, const QSizePolicy &val);
    void slotValueChanged(QtProperty *property, const QFont &val);
    void slotValueChanged(QtProperty *property, const QCursor &val);
    void slotFlagChanged(QtProperty *property, int val);
    void slotFlagNamesChanged(QtProperty *property, const QStringList &flagNames);
    void slotReadOnlyChanged(QtProperty *property, bool readOnly);
    void slotTextVisibleChanged(QtProperty *property, bool textVisible);
    void slotPropertyInserted(QtProperty *property, QtProperty *parent, QtProperty *after);
    void slotPropertyRemoved(QtProperty *property, QtProperty *parent);

    void valueChanged(QtProperty *property, const QVariant &val);

    int internalPropertyToType(QtProperty *property) const;
    QtVariantProperty *createSubProperty(QtVariantProperty *parent, QtVariantProperty *after,
            QtProperty *internal);
    void removeSubProperty(QtVariantProperty *property);

    QMap<int, QtAbstractPropertyManager *> m_typeToPropertyManager;
    QMap<int, QMap<QString, int> > m_typeToAttributeToAttributeType;

    QMap<const QtProperty *, QPair<QtVariantProperty *, int> > m_propertyToType;

    QMap<int, int> m_typeToValueType;


    QMap<QtProperty *, QtVariantProperty *> m_internalToProperty;

    const QString m_constraintAttribute;
    const QString m_singleStepAttribute;
    const QString m_decimalsAttribute;
    const QString m_enumIconsAttribute;
    const QString m_enumNamesAttribute;
    const QString m_flagNamesAttribute;
    const QString m_maximumAttribute;
    const QString m_minimumAttribute;
    const QString m_regExpAttribute;
    const QString m_echoModeAttribute;
    const QString m_readOnlyAttribute;
    const QString m_textVisibleAttribute;
};

class QtVariantEditorFactoryPrivate
{
    QtVariantEditorFactory *q_ptr;
    Q_DECLARE_PUBLIC(QtVariantEditorFactory)
public:

    QtSpinBoxFactory           *m_spinBoxFactory;
    QtDoubleSpinBoxFactory     *m_doubleSpinBoxFactory;
    QtCheckBoxFactory          *m_checkBoxFactory;
    QtLineEditFactory          *m_lineEditFactory;
    QtDateEditFactory          *m_dateEditFactory;
    QtTimeEditFactory          *m_timeEditFactory;
    QtDateTimeEditFactory      *m_dateTimeEditFactory;
    QtKeySequenceEditorFactory *m_keySequenceEditorFactory;
    QtCharEditorFactory        *m_charEditorFactory;
    QtEnumEditorFactory        *m_comboBoxFactory;
    QtCursorEditorFactory      *m_cursorEditorFactory;
    QtColorEditorFactory       *m_colorEditorFactory;
    QtFontEditorFactory        *m_fontEditorFactory;

    QMap<QtAbstractEditorFactoryBase *, int> m_factoryToType;
    QMap<int, QtAbstractEditorFactoryBase *> m_typeToFactory;
};

#if QT_VERSION >= 0x040400
QT_END_NAMESPACE
#endif

