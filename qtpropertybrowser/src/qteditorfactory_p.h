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

#include "qtpropertymanager.h"
#include "qteditorfactory.h"

#include <QSpinBox>
#include <QSlider>
#include <QScrollBar>
#include <QComboBox>
#include <QLineEdit>
#include <QDateTimeEdit>
#include <QLabel>
#include <QToolButton>

#if QT_VERSION >= 0x040400
QT_BEGIN_NAMESPACE
#endif

// ---------- EditorFactoryPrivate :
// Base class for editor factory private classes. Manages mapping of properties to editors and vice versa.

template <class Editor>
class EditorFactoryPrivate
{
public:

    typedef QList<Editor *> EditorList;
    typedef QMap<QtProperty *, EditorList> PropertyToEditorListMap;
    typedef QMap<Editor *, QtProperty *> EditorToPropertyMap;

    Editor *createEditor(QtProperty *property, QWidget *parent);
    void initializeEditor(QtProperty *property, Editor *e);
    void slotEditorDestroyed(QObject *object);

    PropertyToEditorListMap  m_createdEditors;
    EditorToPropertyMap m_editorToProperty;
};

template <class Editor>
Editor *EditorFactoryPrivate<Editor>::createEditor(QtProperty *property, QWidget *parent)
{
    Editor *editor = new Editor(parent);
    initializeEditor(property, editor);
    return editor;
}

template <class Editor>
void EditorFactoryPrivate<Editor>::initializeEditor(QtProperty *property, Editor *editor)
{
    typename PropertyToEditorListMap::iterator it = m_createdEditors.find(property);
    if (it == m_createdEditors.end())
        it = m_createdEditors.insert(property, EditorList());
    it.value().append(editor);
    m_editorToProperty.insert(editor, property);
}

template <class Editor>
void EditorFactoryPrivate<Editor>::slotEditorDestroyed(QObject *object)
{
    const typename EditorToPropertyMap::iterator ecend = m_editorToProperty.end();
    for (typename EditorToPropertyMap::iterator itEditor = m_editorToProperty.begin(); itEditor !=  ecend; ++itEditor) {
        if (itEditor.key() == object) {
            Editor *editor = itEditor.key();
            QtProperty *property = itEditor.value();
            const typename PropertyToEditorListMap::iterator pit = m_createdEditors.find(property);
            if (pit != m_createdEditors.end()) {
                pit.value().removeAll(editor);
                if (pit.value().empty())
                    m_createdEditors.erase(pit);
            }
            m_editorToProperty.erase(itEditor);
            return;
        }
    }
}

class QtSpinBoxFactoryPrivate : public EditorFactoryPrivate<QSpinBox>
{
    QtSpinBoxFactory *q_ptr;
    Q_DECLARE_PUBLIC(QtSpinBoxFactory)
public:

    void slotPropertyChanged(QtProperty *property, int value);
    void slotRangeChanged(QtProperty *property, int min, int max);
    void slotSingleStepChanged(QtProperty *property, int step);
    void slotReadOnlyChanged(QtProperty *property, bool readOnly);
    void slotSetValue(int value);
};

class QtSliderFactoryPrivate : public EditorFactoryPrivate<QSlider>
{
    QtSliderFactory *q_ptr;
    Q_DECLARE_PUBLIC(QtSliderFactory)
public:
    void slotPropertyChanged(QtProperty *property, int value);
    void slotRangeChanged(QtProperty *property, int min, int max);
    void slotSingleStepChanged(QtProperty *property, int step);
    void slotSetValue(int value);
};

class QtScrollBarFactoryPrivate : public  EditorFactoryPrivate<QScrollBar>
{
    QtScrollBarFactory *q_ptr;
    Q_DECLARE_PUBLIC(QtScrollBarFactory)
public:
    void slotPropertyChanged(QtProperty *property, int value);
    void slotRangeChanged(QtProperty *property, int min, int max);
    void slotSingleStepChanged(QtProperty *property, int step);
    void slotSetValue(int value);
};

class QtCheckBoxFactoryPrivate : public EditorFactoryPrivate<QtBoolEdit>
{
    QtCheckBoxFactory *q_ptr;
    Q_DECLARE_PUBLIC(QtCheckBoxFactory)
public:
    void slotPropertyChanged(QtProperty *property, bool value);
    void slotTextVisibleChanged(QtProperty *property, bool textVisible);
    void slotSetValue(bool value);
};

class QtDoubleSpinBoxFactoryPrivate : public EditorFactoryPrivate<QDoubleSpinBox>
{
    QtDoubleSpinBoxFactory *q_ptr;
    Q_DECLARE_PUBLIC(QtDoubleSpinBoxFactory)
public:

    void slotPropertyChanged(QtProperty *property, double value);
    void slotRangeChanged(QtProperty *property, double min, double max);
    void slotSingleStepChanged(QtProperty *property, double step);
    void slotDecimalsChanged(QtProperty *property, int prec);
    void slotReadOnlyChanged(QtProperty *property, bool readOnly);
    void slotSetValue(double value);
};

class QtLineEditFactoryPrivate : public EditorFactoryPrivate<QLineEdit>
{
    QtLineEditFactory *q_ptr;
    Q_DECLARE_PUBLIC(QtLineEditFactory)
public:

    void slotPropertyChanged(QtProperty *property, const QString &value);
    void slotRegExpChanged(QtProperty *property, const QRegExp &regExp);
    void slotSetValue(const QString &value);
    void slotEchoModeChanged(QtProperty *, int);
    void slotReadOnlyChanged(QtProperty *, bool);
};

class QtDateEditFactoryPrivate : public EditorFactoryPrivate<QDateEdit>
{
    QtDateEditFactory *q_ptr;
    Q_DECLARE_PUBLIC(QtDateEditFactory)
public:

    void slotPropertyChanged(QtProperty *property, const QDate &value);
    void slotRangeChanged(QtProperty *property, const QDate &min, const QDate &max);
    void slotSetValue(const QDate &value);
};

class QtTimeEditFactoryPrivate : public EditorFactoryPrivate<QTimeEdit>
{
    QtTimeEditFactory *q_ptr;
    Q_DECLARE_PUBLIC(QtTimeEditFactory)
public:

    void slotPropertyChanged(QtProperty *property, const QTime &value);
    void slotSetValue(const QTime &value);
};

class QtDateTimeEditFactoryPrivate : public EditorFactoryPrivate<QDateTimeEdit>
{
    QtDateTimeEditFactory *q_ptr;
    Q_DECLARE_PUBLIC(QtDateTimeEditFactory)
public:

    void slotPropertyChanged(QtProperty *property, const QDateTime &value);
    void slotSetValue(const QDateTime &value);

};

class QtKeySequenceEditorFactoryPrivate : public EditorFactoryPrivate<QtKeySequenceEdit>
{
    QtKeySequenceEditorFactory *q_ptr;
    Q_DECLARE_PUBLIC(QtKeySequenceEditorFactory)
public:

    void slotPropertyChanged(QtProperty *property, const QKeySequence &value);
    void slotSetValue(const QKeySequence &value);
};

class QtCharEdit : public QWidget
{
    Q_OBJECT
public:
    QtCharEdit(QWidget *parent = 0);

    QChar value() const;
    bool eventFilter(QObject *o, QEvent *e);
public Q_SLOTS:
    void setValue(const QChar &value);
Q_SIGNALS:
    void valueChanged(const QChar &value);
protected:
    void focusInEvent(QFocusEvent *e);
    void focusOutEvent(QFocusEvent *e);
    void keyPressEvent(QKeyEvent *e);
    void keyReleaseEvent(QKeyEvent *e);
    void paintEvent(QPaintEvent *);
    bool event(QEvent *e);
private slots:
    void slotClearChar();
private:
    void handleKeyEvent(QKeyEvent *e);

    QChar m_value;
    QLineEdit *m_lineEdit;
};

class QtCharEditorFactoryPrivate : public EditorFactoryPrivate<QtCharEdit>
{
    QtCharEditorFactory *q_ptr;
    Q_DECLARE_PUBLIC(QtCharEditorFactory)
public:

    void slotPropertyChanged(QtProperty *property, const QChar &value);
    void slotSetValue(const QChar &value);

};

class QtEnumEditorFactoryPrivate : public EditorFactoryPrivate<QComboBox>
{
    QtEnumEditorFactory *q_ptr;
    Q_DECLARE_PUBLIC(QtEnumEditorFactory)
public:

    void slotPropertyChanged(QtProperty *property, int value);
    void slotEnumNamesChanged(QtProperty *property, const QStringList &);
    void slotEnumIconsChanged(QtProperty *property, const QMap<int, QIcon> &);
    void slotSetValue(int value);
};

class QtCursorEditorFactoryPrivate
{
    QtCursorEditorFactory *q_ptr;
    Q_DECLARE_PUBLIC(QtCursorEditorFactory)
public:
    QtCursorEditorFactoryPrivate();

    void slotPropertyChanged(QtProperty *property, const QCursor &cursor);
    void slotEnumChanged(QtProperty *property, int value);
    void slotEditorDestroyed(QObject *object);

    QtEnumEditorFactory *m_enumEditorFactory;
    QtEnumPropertyManager *m_enumPropertyManager;

    QMap<QtProperty *, QtProperty *> m_propertyToEnum;
    QMap<QtProperty *, QtProperty *> m_enumToProperty;
    QMap<QtProperty *, QList<QWidget *> > m_enumToEditors;
    QMap<QWidget *, QtProperty *> m_editorToEnum;
    bool m_updatingEnum;
};

class QtColorEditWidget : public QWidget {
    Q_OBJECT

public:
    QtColorEditWidget(QWidget *parent);

    bool eventFilter(QObject *obj, QEvent *ev);

public Q_SLOTS:
    void setValue(const QColor &value);

Q_SIGNALS:
    void valueChanged(const QColor &value);

protected:
    void paintEvent(QPaintEvent *);

private Q_SLOTS:
    void buttonClicked();

private:
    QColor m_color;
    QLabel *m_pixmapLabel;
    QLabel *m_label;
    QToolButton *m_button;
};

class QtColorEditorFactoryPrivate : public EditorFactoryPrivate<QtColorEditWidget>
{
    QtColorEditorFactory *q_ptr;
    Q_DECLARE_PUBLIC(QtColorEditorFactory)
public:

    void slotPropertyChanged(QtProperty *property, const QColor &value);
    void slotSetValue(const QColor &value);
};


class QtFontEditWidget : public QWidget {
    Q_OBJECT

public:
    QtFontEditWidget(QWidget *parent);

    bool eventFilter(QObject *obj, QEvent *ev);

public Q_SLOTS:
    void setValue(const QFont &value);

Q_SIGNALS:
    void valueChanged(const QFont &value);

protected:
    void paintEvent(QPaintEvent *);

private Q_SLOTS:
    void buttonClicked();

private:
    QFont m_font;
    QLabel *m_pixmapLabel;
    QLabel *m_label;
    QToolButton *m_button;
};


class QtFontEditorFactoryPrivate : public EditorFactoryPrivate<QtFontEditWidget>
{
    QtFontEditorFactory *q_ptr;
    Q_DECLARE_PUBLIC(QtFontEditorFactory)
public:

    void slotPropertyChanged(QtProperty *property, const QFont &value);
    void slotSetValue(const QFont &value);
};

#if QT_VERSION >= 0x040400
QT_END_NAMESPACE
#endif




